/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiaBufferManager.h"

#include "BufferAllocator/BufferAllocatorWrapper.h"
#include "MiaSettings.h"

static BufferAllocator bufferAllocator;

namespace mialgo2 {

static int ionMemAlloc(struct IonBufHandle *handle, uint32_t size, bool cached)
{
    int32_t ret = 0;
    int32_t dataFd = -1;
    unsigned char *virtualAddr = NULL;
    uint32_t len = (size + 4095) & (~4095);
    uint32_t legacy_align = 4096;
    uint32_t legacy_flags = 0;
    struct dma_buf_sync bufSync;

    dataFd =
        DmabufHeapAllocSystem(&bufferAllocator, cached /* cpu_access */, len, legacy_flags,
                              legacy_align); //! cpu_access_needed kDmabufSystemUncachedHeapName
    if (dataFd < 0) {
        MLOGE(Mia2LogGroupCore, "DmabufHeapAllocSystem() failed: %d cpu_access: %d\n", dataFd,
              cached);
        return -1;
    }

    virtualAddr = (unsigned char *)mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, dataFd, 0);
    if (virtualAddr == MAP_FAILED) {
        MLOGE(Mia2LogGroupCore, "DmabufHeap map failed (%s)\n", strerror(errno));
        close(dataFd);
        dataFd = -1;
        return -1;
    }

    ret = DmabufHeapCpuSyncStart(&bufferAllocator, dataFd,
                                 kSyncReadWrite /*SyncType default = kSyncRead*/, NULL, NULL);
    if (ret) {
        MLOGE(Mia2LogGroupCore, "DmabufHeapCpuSyncStart failed: %d\n", ret);
        munmap(virtualAddr, len);
        virtualAddr = NULL;
        close(dataFd);
        dataFd = -1;
        return -1;
    }

    handle->data_fd = dataFd;
    handle->buffer = virtualAddr;
    handle->buffer_len = size;
    return ret;
}

static int ionMemFree(struct IonBufHandle *handle)
{
    uint32_t len = (handle->buffer_len + 4095) & (~4095);
    int ret = 0;

    ret = DmabufHeapCpuSyncEnd(&bufferAllocator, handle->data_fd, kSyncReadWrite, NULL, NULL);
    if (ret) {
        MLOGE(Mia2LogGroupCore, "DmabufHeapCpuSyncEnd failed: %d\n", ret);
    }

    if (handle->buffer) {
        munmap(handle->buffer, len);
        handle->buffer = NULL;
    }

    if (handle->data_fd >= 0) {
        close(handle->data_fd);
        handle->data_fd = -1;
    }
    // No close for heap fd
    return ret;
}

MiaBufferManager::~MiaBufferManager()
{
    m_width = 0;
    m_height = 0;
    m_format = 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MiaBufferManager::create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
MiaBufferManager *MiaBufferManager::create(BufferManagerCreateData *createData)
{
    CDKResult result = MIAS_OK;
    MiaBufferManager *bufferManager = new MiaBufferManager();

    result = bufferManager->initialize(createData);

    if (MIAS_OK != result) {
        bufferManager->destroy();
        delete bufferManager;
        bufferManager = NULL;
    }

    return bufferManager;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MiaBufferManager::destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MiaBufferManager::destroy()
{
    // check of we have buffer in-flight
    if (!mBufferOutList.empty()) {
        MLOGW(Mia2LogGroupCore,
              "destroy when some buffer(%zu) still in-flight, this may cause problems",
              mBufferOutList.size());
    }
    freeAllBuffers();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MiaBufferManager::initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult MiaBufferManager::initialize(BufferManagerCreateData *createData)
{
    CDKResult result = MIAS_OK;
    mNumBuffers = 0;
    mBufferList.clear();
    mBufferOutList.clear();
    mType = createData->bufferType;

    mTotalIonMemLock.lock();
    mTotalIonMem = 0;
    mTotalIonBufferNums = 0;
    mTotalIonMemLock.unlock();

    if (mType == GRALLOC_TYPE) {
    }

    if (MIAS_OK == result) {
        result = allocateBuffers(createData->numBuffers, createData->width, createData->height,
                                 createData->format, createData->usage);
    }

    m_width = createData->width;
    m_height = createData->height;
    m_format = createData->format;
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Allocate a number of buffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult MiaBufferManager::allocateMoreBuffers(uint32_t numBuffers)
{
    CDKResult result = MIAS_OK;

    uint64_t usage = 0x3;
    result = allocateBuffers(numBuffers, m_width, m_height, m_format, usage);

    MLOGI(Mia2LogGroupCore, "Allocate more buffers, now numBuffers: %d", mNumBuffers);
    return result;
}

CDKResult MiaBufferManager::allocateBuffers(uint32_t numBuffers, uint32_t width, uint32_t height,
                                            uint32_t format, uint64_t usage)
{
    CDKResult result = MIAS_OK;
    uint32_t allocedBufferNumThisTime = 0;
    for (uint32_t i = 0; i < numBuffers; i++) {
        MiaBufferHandle *buffer = (MiaBufferHandle *)malloc(sizeof(MiaBufferHandle));
        if (buffer == NULL) {
            continue;
        }
        if (mType == GRALLOC_TYPE) {
            memset(&buffer->grallocHandle, 0x0, sizeof(GrallocBufHandle));
            GrallocUtils::allocateGrallocBuffer(width, height, format, usage,
                                                &buffer->grallocHandle, &mBufferStride);
            buffer->type = mType;
            onbuffersAllocated(*buffer);
            mBufferList.push_back(buffer);

            allocedBufferNumThisTime++;
            Mutex::Autolock autoLock(bufferNumLock);
            mNumBuffers++;
        } else {
            memset(&buffer->ionHandle, 0x0, sizeof(IonBufHandle));
            // This time allocateIonBuffer may fail
            result = allocateIonBuffer(width, height, format, &buffer->ionHandle);

            if (result == MIAS_OK) {
                buffer->type = mType;
                onbuffersAllocated(*buffer);
                mBufferList.push_back(buffer);

                allocedBufferNumThisTime++;
                Mutex::Autolock autoLock(bufferNumLock);
                mNumBuffers++;
            } else {
                free(buffer);
            }
        }
        MLOGI(Mia2LogGroupCore,
              "totalBufferNums %d, thisBufferNums:%d, width %d, height %d stride %d, mType: %d",
              mNumBuffers, allocedBufferNumThisTime, width, height, mBufferStride, mType);
    }

    if (allocedBufferNumThisTime > 0) {
        result = MIAS_OK;
    } else {
        result = MIAS_NO_MEM;
    }
    return result;
}

int MiaBufferManager::allocateIonBuffer(uint32_t width, uint32_t height, uint32_t format,
                                        IonBufHandle *allocatedBuffer)
{
    LenOffsetInfo mOffsetInfo;
    PaddingInfo mPadInfo;
    MiaImageFormat mFormat;
    unsigned int frameLen = 0;

    mPadInfo.width_padding = MI_PAD_TO_64;
    mPadInfo.height_padding = MI_PAD_TO_64;
    mPadInfo.plane_padding = MI_PAD_TO_64;

    mFormat.width = width;
    mFormat.height = height;
    mFormat.format = format;

    CDKResult rc = MiaUtil::calcOffset(mFormat, &mPadInfo, &mOffsetInfo);
    if (MIAS_OK != rc) {
        MLOGE(Mia2LogGroupCore, "Fail to new input  image: Fail to calculate offset\n");
        return rc;
    }

    frameLen = mOffsetInfo.frame_len;
    MLOGD(Mia2LogGroupCore, "ionMemAlloc %d bytes for frame %d*%d of format 0x%x\n", frameLen,
          width, height, mFormat.format);

    int ret = ionMemAlloc(allocatedBuffer, frameLen, true /*cached*/);
    if (ret != 0) {
        MLOGE(Mia2LogGroupCore, "malloc failed!");
        return MIAS_NO_MEM;
    }

    MLOGD(Mia2LogGroupCore, "success, malloc buffer %p, size: %d dataFd: %d",
          allocatedBuffer->buffer, allocatedBuffer->buffer_len, allocatedBuffer->data_fd);

    return MIAS_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// memory reclaim
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MiaBufferManager::releaseImageBuffer(MiaBufferHandle *bufferHandle)
{
    {
        Mutex::Autolock autoLock(mLock);
        BufferList_t::iterator it = mBufferOutList.begin();
        for (; it != mBufferOutList.end(); it++) {
            if (*it == bufferHandle) {
                mBufferOutList.erase(it);
                break;
            }
        }
    }

    if (bufferHandle != NULL) {
        MLOGD(Mia2LogGroupCore, "release buffer:%p, numBuffers: %d", bufferHandle, mNumBuffers - 1);
        freeOneBuffer(bufferHandle);
        free(bufferHandle);
        bufferHandle = NULL;
        {
            Mutex::Autolock autoLock(bufferNumLock);
            mNumBuffers--;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Free one buffer
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MiaBufferManager::freeOneBuffer(MiaBufferHandle *buffer)
{
    if (buffer->type == GRALLOC_TYPE) {
        onbuffersReleased(*buffer);
        GrallocUtils::freeGrallocBuffer(&buffer->grallocHandle);
    } else if (buffer->type == ION_TYPE) {
        onbuffersReleased(*buffer);
        ionMemFree(&buffer->ionHandle);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Free all buffers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MiaBufferManager::freeAllBuffers()
{
    while (!mBufferList.empty()) {
        BufferList_t::iterator it = mBufferList.begin();
        MiaBufferHandle *buffer = *it;
        if (buffer->type == GRALLOC_TYPE) {
            onbuffersReleased(*buffer);
            GrallocUtils::freeGrallocBuffer(&buffer->grallocHandle);
        } else if (buffer->type == ION_TYPE) {
            onbuffersReleased(*buffer);
            ionMemFree(&buffer->ionHandle);
        }
        mBufferList.erase(it);
        free(buffer);
        buffer = NULL;
    }
}

uint64_t MiaBufferManager::getBufferSize(buffer_handle_t bufferHandle)
{
    uint64_t size = 0;
    GrallocUtils::getGrallocBufferSize(bufferHandle, &size);
    return size;
}

void *MiaBufferManager::getCPUAddress(buffer_handle_t bufferHandle, int size)
{
    void *virtualAddress = NULL;
    if (NULL != bufferHandle) {
        int fd = 0;
        if (MiaInternalProp::getInstance()->getInt(Soc) == midebug::SocType::QCOM) {
            fd = reinterpret_cast<const native_handle_t *>(bufferHandle)->data[0];
        } else {
            fd = reinterpret_cast<const native_handle_t *>(bufferHandle)->data[1];
        }
        // bufferHandle is a gralloc handle
        MLOGD(Mia2LogGroupCore, "bufferHandle = %p, fd = %d", bufferHandle, fd);

        virtualAddress = mmap(NULL, size, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, 0);

        if ((NULL == virtualAddress) || (reinterpret_cast<void *>(-1) == virtualAddress)) {
            MLOGW(Mia2LogGroupCore,
                  "MIA Failed in getting virtual address, bufferHandle=%p, virtualAddress = %p",
                  bufferHandle, virtualAddress);

            // in case of mmap failures, it will return -1, not NULL. So, set to NULL so that
            // callers just validate on NULL
            virtualAddress = NULL;
        }
    } else {
        MLOGE(Mia2LogGroupCore, "Buffer handle is NULL, bufferHandle=%p", bufferHandle);
    }

    return virtualAddress;
}

void MiaBufferManager::putCPUAddress(buffer_handle_t bufferHandle, int size, void *virtualAddress)
{
    if ((NULL != bufferHandle) && (NULL != virtualAddress)) {
        munmap(virtualAddress, size);
    } else {
        MLOGE(Mia2LogGroupCore, "Buffer handle is NULL, bufferHandle=%p", bufferHandle);
    }
}

void MiaBufferManager::onbuffersAllocated(MiaBufferHandle buffer)
{
    mTotalIonMemLock.lock();
    if (buffer.type == GRALLOC_TYPE) {
        uint64_t memoryTemp = getBufferSize(buffer.grallocHandle.nativeHandle);
        mTotalIonMem += memoryTemp;
        ++mTotalIonBufferNums;
        MLOGI(
            Mia2LogGroupCore,
            " mialgo2_Gralloc_mem: buffer num is %llu ,one buffer size is %llu M,total used %llu M",
            mTotalIonBufferNums, memoryTemp / 1024 / 1024, mTotalIonMem / 1024 / 1024);
    } else {
        uint64_t memoryTemp = buffer.ionHandle.buffer_len;
        mTotalIonMem += memoryTemp;
        ++mTotalIonBufferNums;
        MLOGI(Mia2LogGroupCore,
              " mialgo2_Ion_mem: buffer num is %llu ,one buffer size is %llu M,total used %llu M",
              mTotalIonBufferNums, memoryTemp / 1024 / 1024, mTotalIonMem / 1024 / 1024);
    }
    mTotalIonMemLock.unlock();
    MICAM_TRACE_INT64_F(MialgoTraceMemProfile, mTotalIonMem, "%s", "mialgo2_Ion_mem");
}

void MiaBufferManager::onbuffersReleased(MiaBufferHandle buffer)
{
    mTotalIonMemLock.lock();
    if (buffer.type == GRALLOC_TYPE) {
        uint64_t memoryTemp = getBufferSize(buffer.grallocHandle.nativeHandle);
        mTotalIonMem -= memoryTemp;
        --mTotalIonBufferNums;
        MLOGI(
            Mia2LogGroupCore,
            " mialgo2_Gralloc_mem: buffer num is %llu ,one buffer size is %lu M,total used %llu M",
            mTotalIonBufferNums, memoryTemp / 1024 / 1024, mTotalIonMem / 1024 / 1024);
    } else {
        uint64_t memoryTemp = buffer.ionHandle.buffer_len;
        mTotalIonMem -= memoryTemp;
        --mTotalIonBufferNums;
        MLOGI(Mia2LogGroupCore,
              " mialgo2_Ion_mem: buffer num is %llu ,one buffer size is %lu M,total used %llu M",
              mTotalIonBufferNums, memoryTemp / 1024 / 1024, mTotalIonMem / 1024 / 1024);
    }
    mTotalIonMemLock.unlock();
    MICAM_TRACE_INT64_F(MialgoTraceMemProfile, mTotalIonMem, "%s", "mialgo2_Ion_mem");
}

} // end namespace mialgo2
