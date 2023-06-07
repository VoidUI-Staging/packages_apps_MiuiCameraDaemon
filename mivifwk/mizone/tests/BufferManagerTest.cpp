#include <cutils/properties.h>

#include "MiBufferManager.h"

using namespace midebug;
using namespace mizone;

#define MI_BUFFER_TEST_FWK_STREAM_ID  (20011) // TODO, support vendor stream only now
#define MI_BUFFER_TEST_VND_STREAM_ID  (20012)
#define MI_BUFFER_TEST_VND2_STREAM_ID (20013)
#define MI_BUFFER_TEST_IMAGE_WITH     (4000)
#define MI_BUFFER_TEST_IMAGE_HEIGHT   (3000)
#define MI_BUFFER_TEST_FILL_VALUE     (6)

#define MI_BUFFER_TEST_REQUEST_PARAMS     (1)
#define MI_BUFFER_TEST_REQUEST_CAPREQUEST (2)

void memoryCallback(MiBufferManager::MemoryInfoMap &memoryInfo)
{
    for (auto &it : memoryInfo) {
        MLOGI(LogGroupHAL, "stream %d, memory %u bytes", it.first, it.second);
    }
}

std::shared_ptr<MiBufferManager> testCreateMgr(void)
{
    StreamConfiguration fwkCfg;
    StreamConfiguration vndCfg;
    std::vector<Stream> streams;
    streams.clear();

    Stream stream;
    stream.id = MI_BUFFER_TEST_FWK_STREAM_ID;
    stream.format = mizone::EImageFormat::eImgFmt_NV21;
    stream.width = MI_BUFFER_TEST_IMAGE_WITH;
    stream.height = MI_BUFFER_TEST_IMAGE_HEIGHT;

    streams.push_back(stream);
    fwkCfg.streams = streams;
    fwkCfg.operationMode = StreamConfigurationMode::NORMAL_MODE;
    fwkCfg.streamConfigCounter = 0;

    streams.clear();
    stream.id = MI_BUFFER_TEST_VND_STREAM_ID;
    stream.format = mizone::EImageFormat::eImgFmt_RAW16;
    streams.push_back(stream);
    stream.id = MI_BUFFER_TEST_VND2_STREAM_ID;
    stream.format = mizone::EImageFormat::eImgFmt_RAW16;
    streams.push_back(stream);
    vndCfg = fwkCfg;
    vndCfg.streams = streams;

    MiBufferManager::BufferMgrCreateInfo createInfo;
    createInfo.fwkConfig = fwkCfg;
    createInfo.vendorConfig = vndCfg;
    createInfo.callback = memoryCallback;

    return MiBufferManager::create(createInfo);
}

bool testRequestBuffer(const std::shared_ptr<MiBufferManager> spMgr,
                       std::map<int32_t, std::vector<StreamBuffer>> &bufferMap, int testType)
{
    bool ret = false;

    std::vector<MiBufferManager::MiBufferRequestParam> params;
    MiBufferManager::MiBufferRequestParam param;
    param.streamId = MI_BUFFER_TEST_FWK_STREAM_ID;
    param.bufferNum = ::property_get_int32("vendor.debug.camera.mizone.testMgrBufNum", 6);
    params.push_back(param);
    param.streamId = MI_BUFFER_TEST_VND_STREAM_ID;
    params.push_back(param);

    std::map<int32_t, std::vector<StreamBuffer>> tmpBufferMap;
    bufferMap.clear();
    switch (testType) {
    case MI_BUFFER_TEST_REQUEST_PARAMS:
        ret = spMgr->requestBuffers(params, bufferMap);
        break;
    default:
        MLOGE(LogGroupHAL, "invalid testType %d", testType);
    }
    if (ret == false) {
        MLOGE(LogGroupHAL, "request testType %d failed", testType);
        return false;
    } else {
        MLOGI(LogGroupHAL, "request success testType %d, map size %lu", testType, bufferMap.size());
        return true;
    }
}

void testReleaseBuffer(const std::shared_ptr<MiBufferManager> spMgr,
                       std::map<int32_t, std::vector<StreamBuffer>> &bufferMap)
{
    for (auto &item : bufferMap) {
        MLOGI(LogGroupHAL, " release buffer for stream %d", item.first);
        spMgr->releaseBuffers(item.second);
    }
    bufferMap.clear();
}

bool pullBuffer(unsigned char *src, size_t size)
{
    // MLOGI(LogGroupHAL, "pull %p with size %lu", src, size);
    for (int i = 0; i < size; i++) {
        if (*src++ != MI_BUFFER_TEST_FILL_VALUE) {
            MLOGE(LogGroupHAL, "pullBuffer failed");
            return false;
        }
    }
    // MLOGI(LogGroupHAL, "pull done");
    return true;
}

bool testReadBuffer(std::map<int32_t, std::vector<StreamBuffer>> &bufferMap)
{
    bool ret = true;

    for (auto &it : bufferMap) {
        std::vector<StreamBuffer> buffers = it.second;
        for (auto item = buffers.begin(); item != buffers.end(); ++item) {
            std::shared_ptr<MiImageBuffer> pImgBuffer = item->buffer;
            if (pImgBuffer->lockBuf("test", eBUFFER_USAGE_SW_READ_OFTEN) != 0) {
                unsigned char *addr = pImgBuffer->getBufVA(0);
                if (addr != nullptr) {
                    ret = pullBuffer(addr, pImgBuffer->getBufSize());
                } else {
                    MLOGE(LogGroupHAL, "addr null");
                    ret = false;
                }
                pImgBuffer->unlockBuf("test");
            } else {
                MLOGE(LogGroupHAL, "lock failed");
                ret = false;
            }
        }
    }

    return ret;
}

bool fillBuffer(unsigned char *dst, size_t size)
{
    // MLOGI(LogGroupHAL, "fill %p with size %lu", dst, size);
    for (int i = 0; i < size; i++) {
        *dst++ = MI_BUFFER_TEST_FILL_VALUE;
    }
    // MLOGI(LogGroupHAL, "fill done");
    return true;
}

bool testWriteBuffer(std::map<int32_t, std::vector<StreamBuffer>> &bufferMap)
{
    bool ret = true;

    for (auto &it : bufferMap) {
        std::vector<StreamBuffer> buffers = it.second;
        for (auto item = buffers.begin(); item != buffers.end(); ++item) {
            std::shared_ptr<MiImageBuffer> pImgBuffer = item->buffer;
            if (pImgBuffer->lockBuf("test", eBUFFER_USAGE_SW_WRITE_OFTEN) != 0) {
                unsigned char *addr = pImgBuffer->getBufVA(0);
                if (addr != nullptr) {
                    ret = fillBuffer(addr, pImgBuffer->getBufSize());
                } else {
                    MLOGE(LogGroupHAL, "addr null");
                    ret = false;
                }
                pImgBuffer->unlockBuf("test");
            } else {
                MLOGE(LogGroupHAL, "lock failed");
                ret = false;
            }
        }
    }

    return ret;
}

bool testBufferManagerImpl(int testType)
{
    bool ret = false;
    std::map<int32_t, std::vector<StreamBuffer>> bufferMap;
    std::shared_ptr<MiBufferManager> spManager;
    std::vector<int32_t> usedStreams{};

    // step 1
    spManager = testCreateMgr();
    if (spManager == nullptr) {
        MLOGE(LogGroupHAL, "testCreateMgr failed");
        return false;
    } else {
        MLOGI(LogGroupHAL, "create mgr done");
    }

    // step 2
    ret = testRequestBuffer(spManager, bufferMap, testType);
    if (ret == true && bufferMap.size() != 0) {
        MLOGI(LogGroupHAL, "testRequestBuffer success");
    } else {
        MLOGE(LogGroupHAL, "testRequestBuffer failed");
        return false;
    }

    // check buffer used
    for (auto &pair : bufferMap) {
        MLOGI(LogGroupHAL, "buffer used %u bytes for stream %d",
              spManager->queryMemoryUsedByStream(pair.first), pair.first);
        usedStreams.push_back(pair.first);
    }

    // step 3
    ret = testWriteBuffer(bufferMap);
    if (ret == false) {
        MLOGE(LogGroupHAL, "testWriteBuffer failed");
    }

    // step 4
    ret = testReadBuffer(bufferMap);
    if (ret == false) {
        MLOGE(LogGroupHAL, "testReadBuffer failed");
    }

    // check buffer used
    for (auto &stream : usedStreams) {
        MLOGI(LogGroupHAL, "buffer used %u bytes for stream %d",
              spManager->queryMemoryUsedByStream(stream), stream);
    }

    // step 5
    testReleaseBuffer(spManager, bufferMap);

    // step 6
    if (testType == MI_BUFFER_TEST_REQUEST_PARAMS) {
        // test cached buffer
        ret = testRequestBuffer(spManager, bufferMap, testType);
        if (ret == true && bufferMap.size() != 0) {
            MLOGI(LogGroupHAL, "testRequestBuffer success");
        } else {
            MLOGE(LogGroupHAL, "testRequestBuffer failed");
            return false;
        }

        testReleaseBuffer(spManager, bufferMap);
    }

    // check buffer used
    for (auto &stream : usedStreams) {
        MLOGI(LogGroupHAL, "buffer used %u bytes for stream %d",
              spManager->queryMemoryUsedByStream(stream), stream);
    }

    MLOGI(LogGroupHAL, "all test done");
    return true;
}

int main(int argc, char **argv)
{
    int testTimes = ::property_get_int32("vendor.debug.camera.mizone.testMgrTimes", 1);
    int testType = MI_BUFFER_TEST_REQUEST_PARAMS;
    if (argc > 1) {
        testType = atoi(argv[1]);
    }
    for (int i = 0; i < testTimes; i++) {
        if (!testBufferManagerImpl(testType)) {
            MLOGE(LogGroupHAL, " test %d failed", i);
            return 0;
        } else {
            MLOGI(LogGroupHAL, " test %d done", i);
        }
    }
    return 0;
}