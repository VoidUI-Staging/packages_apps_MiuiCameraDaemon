/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef __MI_BUFFERMANAGER__
#define __MI_BUFFERMANAGER__

#include <MiCamTrace.h>

#include <mutex>

#include "BufferAllocator/BufferAllocator.h"
#include "GrallocUtils.h"
#include "MiaUtil.h"

#define MINIMUM_BUFFER_QUEUE_DEPTH   1
#define DEFAULT_BUFFER_QUEUE_DEPTH   2
#define THRESHOLD_BUFFER_QUEUE_DEPTH 4
#define MAX_BUFFER_QUEUE_DEPTH       20

namespace mialgo2 {

struct IonBufHandle
{
    unsigned char *buffer;
    uint32_t buffer_len;
    int data_fd;
};

struct MiaBufferHandle
{
    buffer_type type;
    union {
        GrallocBufHandle grallocHandle;
        IonBufHandle ionHandle;
    };
};

typedef List<MiaBufferHandle *> BufferList_t;

/// @brief BufferManager create data structure
struct BufferManagerCreateData
{
    uint32_t width;         ///< Buffer width
    uint32_t height;        ///< Buffer height
    uint32_t format;        ///< Buffer format
    uint64_t usage;         ///< Buffer gralloc flags
    uint32_t numBuffers;    ///< Number of buffers to allcoate
    buffer_type bufferType; ///< Buffer alloc type
};

// TODO: this is not a singleton object. Remove the static "create" method
class MiaBufferManager : public virtual RefBase
{
public:
    /// creates an instance of this class
    static MiaBufferManager *create(BufferManagerCreateData *createData);
    ~MiaBufferManager();

    /// destroy an instance of the class
    void destroy();

    CDKResult allocateMoreBuffers(uint32_t numBuffers);

    uint32_t getAllocatedBufferNum() { return mNumBuffers; }

    /// Get a buffer
    MiaBufferHandle *getBuffer()
    {
        Mutex::Autolock autoLock(mLock);
        if (!mBufferList.empty()) {
            BufferList_t::iterator it = mBufferList.begin();
            MiaBufferHandle *buffer = *it;
            mBufferOutList.push_back(buffer);
            mBufferList.erase(it);
            return buffer;
        } else {
            return NULL;
        }
    }

    /// get the in-flight list
    BufferList_t getBuffersInFlight() { return mBufferOutList; }
    BufferList_t getBuffersNoFlight() { return mBufferList; }

    /// release the free buffer from out-flight list
    void releaseImageBuffer(MiaBufferHandle *bufferHandle);

    static uint64_t getBufferSize(buffer_handle_t bufferHandle);

    static void *getCPUAddress(buffer_handle_t bufferHandle, int size);

    static void putCPUAddress(buffer_handle_t bufferHandle, int size, void *virtualAddress);

    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_format;

    void onbuffersAllocated(MiaBufferHandle mBuffer);
    void onbuffersReleased(MiaBufferHandle mBuffer);

private:
    /// initialize the instance
    CDKResult initialize(BufferManagerCreateData *createData);

    MiaBufferManager() = default;

    // Do not support the copy constructor or assignment operator
    MiaBufferManager(const MiaBufferManager &) = delete;
    MiaBufferManager &operator=(const MiaBufferManager &) = delete;

    // Pre allocate the max number of buffers the buffer manager needs to manage
    CDKResult allocateBuffers(uint32_t numBuffers, uint32_t width, uint32_t height, uint32_t format,
                              uint64_t usage);

    // Allocate one buffer
    int allocateIonBuffer(uint32_t width, uint32_t height, uint32_t format,
                          IonBufHandle *allocatedBuffer);

    // Free all buffers
    void freeAllBuffers();
    void freeOneBuffer(MiaBufferHandle *buffer);

    mutable Mutex mLock;
    BufferList_t mBufferList;
    BufferList_t mBufferOutList;
    uint32_t mNumBuffers;   ///< Num of Buffers
    uint32_t mBufferStride; ///< Buffer stride
    // uint32_t            m_nextFreeBuffer;               ///< Next free ZSL buffer to be used in
    // the preview pipeline
    buffer_type mType;

    uint64_t mTotalIonMem;
    uint64_t mTotalIonBufferNums;
    std::mutex mTotalIonMemLock;

    Mutex bufferNumLock;
};

} // namespace mialgo2
#endif // END define __MI_BUFFERMANAGER__
