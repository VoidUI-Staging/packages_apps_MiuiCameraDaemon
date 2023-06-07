#ifndef MIPLUGINADAPTOR_H
#define MIPLUGINADAPTOR_H

#include <iostream>
#include <memory>

#include "MiPostProcDevice.h"
#include "MiPostProcDeviceUtil.h"
#include "MiZoneTypes.h"
#include "MiaPluginWraper.h"

namespace mizone {
namespace mipostprocinterface {

struct StreamInfos
{
    StreamType streamType;
    uint32_t width;
    uint32_t height;
    Stream stream;
    PostProcParams *pPostProcParams;
};

std::atomic<int32_t> rStreamIdStart = 2000;
std::atomic<int32_t> rBufferIdx = 3000;

class MiPluginAdaptor : public MiPostProcAdaptorCb
{
public:
    static auto create(CreationInfos &rInfos) -> std::shared_ptr<MiPluginAdaptor>;
    int process(ProcessInfos &rInfos, ProcessRequestInfo *pluginInfo);
    int flush();
    int destroy();
    void handleNotify(const std::vector<NotifyMsg> &msgs) override;
    void handleResult(const std::vector<CaptureResult> &results) override;

public:
    ~MiPluginAdaptor();
    MiPluginAdaptor(CreationInfos &rInfos);

private:
    void composeStream(const ImageParams &phBuffer, std::vector<StreamBuffer> &streamBuffers,
                       StreamInfos &rInfos);
    void composeStreamBuffer(const ImageParams &phParams, std::vector<StreamBuffer> &streamBuffer,
                             StreamInfos &rInfos);
    void composeMetadata(const ImageParams &phParams, CaptureRequest &request, StreamInfos &rInfos);
    void composeMetadata(const ImageParams &phParams, SubRequest &request, StreamInfos &rInfos);
    void convertRequestInfos(AdaptorParams &rParams, ProcessRequestInfo *pluginInfo);
    void configureIfNeed(AdaptorParams &rParams, ProcessRequestInfo *pluginInfo);
    void processReqNo(const uint32_t &requestNo);
    void processReqNo(const uint32_t &requestNo, CaptureResult &res);

    int32_t mCamId = 0;

    std::shared_ptr<MiPostProcDevice> mPostProcDevice;
    int32_t multiFrameCount;
    MiPostProcPluginCb *mPluginCb;
    ProcessRequestInfo *mPluginInfo;
    CallbackInfos callbackInfos;
    int32_t mPluginReqNo;
    // <buffer format, StreamInfos>
    std::multimap<EImageFormat, StreamInfos> mMapStream;
    bool mIsNeedRecfg = false; // true: need; false: do not need(default)

    std::condition_variable cond;
    mutable std::mutex mx;
    // <reqNo, outputBuffer index>
    std::multimap<uint32_t, uint16_t> mMapReqNum;
    // <vndReqNo, settings>
    std::map<uint32_t, IMetadata> mMapReqSettings;
};

} // namespace mipostprocinterface
} // namespace mizone

#endif