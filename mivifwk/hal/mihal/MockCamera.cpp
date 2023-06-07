#include "MockCamera.h"

#include "Camera3Plus.h"
#include "Metadata.h"
#include "MiviSettings.h"
#include "Session.h"

namespace mihal {

MockCamera::MockCamera(int cameraId, const camera_info *info, const Metadata *defaultSettings,
                       CameraModule *module)
    : CameraDevice{std::to_string(cameraId)},
      mInfo{info},
      mDefaultSettings{defaultSettings},
      mSessionManager{SessionManager::getInstance()}
{
    mHal3Device.common.version = CAMERA_DEVICE_API_VERSION_CURRENT;

    MiviSettings::getVal("MockCameraFwkStreamMaxBufferQueueSize", mFwkStreamMaxBufferQueueSize, 8);
    mFlushed.store(false);
}

int MockCamera::open()
{
    MLOGI(LogGroupHAL, "[MiHalOp]%s: open", getLogPrefix().c_str());
    return 0;
}

int MockCamera::close()
{
    MLOGI(LogGroupHAL, "[MiHalOp]%s: Begin close", getLogPrefix().c_str());

    if (!mFlushed.load()) {
        flush();
    }

    mConfig = nullptr;

    MLOGI(LogGroupHAL, "[MiHalOp]%s: End close", getLogPrefix().c_str());
    return 0;
}

int MockCamera::getCameraInfo(camera_info *info) const
{
    if (!info)
        return -EINVAL;

    *info = *mInfo;

    return 0;
}

const camera_metadata *MockCamera::defaultSettings(int) const
{
    MLOGI(LogGroupHAL, "[MiHalOp]%s: get default setting", getLogPrefix().c_str());
    return mDefaultSettings->peek();
}

int MockCamera::checkIsConfigValid(std::shared_ptr<StreamConfig> config)
{
    // NOTE: for VTS test where the test script will send many strange stream config
    int ret = 0;
    config->forEachStream([&](const camera3_stream *stream) {
        if (stream->width == 0 || stream->height == 0 || stream->format == -1) {
            MLOGE(LogGroupHAL, "invalid parameter for stream: %p", stream);
            ret = -EINVAL;
            return -EINTR;
        }

        return 0;
    });

    return ret;
}

int MockCamera::configureStreams(std::shared_ptr<StreamConfig> config)
{
    MLOGI(LogGroupHAL, "[MiHalOp]%s[Config]: config:\n\t%s", getLogPrefix().c_str(),
          config->toString().c_str());

    auto checkResult = checkIsConfigValid(config);
    if (checkResult) {
        MLOGE(LogGroupHAL, "invalid stream config");
        return checkResult;
    }

    std::string dump = config->getMeta()->toString();
    MLLOGV(LogGroupHAL, dump, "[MiHalOp]%s[Config]: dump config meta:", getLogPrefix().c_str());

    // TODO: introduce some new methods to set these attributes
    camera3_stream_configuration *rawConfig = config->toRaw();
    for (int i = 0; i < rawConfig->num_streams; i++) {
        rawConfig->streams[i]->max_buffers = mFwkStreamMaxBufferQueueSize;
        MLOGI(LogGroupRT,
              "[MiHalOp][Config][MockInvitations]invited configureStreams size:%uX%u, format: 0x%x",
              rawConfig->streams[i]->width, rawConfig->streams[i]->height,
              rawConfig->streams[i]->format);
    }

    // NOTE: mConfig.reset here is important to hold the registry if new is the same with the old
    mConfig.reset(nullptr);
    StreamConfig *hold = new LocalConfig{*config};
    MASSERT(hold);
    mConfig = std::unique_ptr<StreamConfig, std::function<void(StreamConfig *)>>{
        hold, [](StreamConfig *cfg) {
            cfg->forEachStream([](std::shared_ptr<Stream> stream) {
                stream->setRetired();
                return 0;
            });
            delete cfg;
        }};

    mFlushed.store(false);
    MLOGI(LogGroupHAL, "[MiHalOp][Config] End %s", getLogPrefix().c_str());

    return 0;
}

int MockCamera::processCaptureRequest(std::shared_ptr<CaptureRequest> request)
{
    auto frameEntry = request->findInMeta(MI_MIVI_FG_FRAME);
    uint64_t fwkFrame = -1;
    if (frameEntry.count) {
        fwkFrame = frameEntry.data.i32[0];
    } else {
        MLOGE(LogGroupHAL,
              "[Mock][ProcessRequest]: unable to anchor foreground camera frame number");
        MASSERT(false);
    }
    MLOGI(LogGroupHAL, "%s: [Fulfill]: mock request for fwkFrame:%u mock frame :%d arrived",
          getLogPrefix().c_str(), fwkFrame, request->getFrameNumber());

    uint32_t sessionId = -1;
    auto entry = request->findInMeta(MI_MIVI_SESSION_ID);
    if (entry.count) {
        sessionId = entry.data.i32[0];
    }

    std::vector<std::shared_ptr<StreamBuffer>> outBuffers;
    for (int i = 0; i < request->getOutBufferNum(); i++) {
        auto buffer = request->getStreamBuffer(i);
        outBuffers.push_back(buffer);
    }

    auto localRequest = std::make_shared<LocalRequest>(
        request->getCamera(), request->getFrameNumber(), outBuffers, *request->getMeta());

    {
        std::lock_guard<std::recursive_mutex> lock{mMutex};
        mPendingMockRecords[localRequest->getFrameNumber()] =
            std::make_unique<MockRequestRecord>(localRequest);

        for (auto &iter : mPendingMockRecords) {
            MLOGI(LogGroupHAL, " mPendingMockRecords mock frame :%d ", iter.first);
        }
    }

    DARKROOM.processOutputRequest(localRequest, fwkFrame << 32 | sessionId);
    return 0;
}

void MockCamera::processMockPendingFrameImpl(uint32_t frame)
{
    auto it = mPendingMockRecords.find(frame);
    if (it == mPendingMockRecords.end()) {
        MLOGD(LogGroupHAL, "%s: not found in pending record", getLogPrefixWithFrame(frame).c_str());
        return;
    }

    auto &record = it->second;
    record->flush();

    mPendingMockRecords.erase(it);
    MLOGI(LogGroupHAL, "%s: job completed", getLogPrefixWithFrame(frame).c_str());
}

void MockCamera::processMockPendingFrame(uint32_t frame)
{
    std::lock_guard<std::recursive_mutex> lock{mMutex};
    processMockPendingFrameImpl(frame);
}

MockCamera::MockRequestRecord::MockRequestRecord(std::shared_ptr<CaptureRequest> request)
    : mMockRequest{request}, mPartialResult{0}
{
    for (int i = 0; i < request->getOutBufferNum(); i++) {
        auto streamBuffer = request->getStreamBuffer(i);
        camera3_stream *raw = streamBuffer->getStream()->toRaw();
        mPendingStreams.insert(raw);
    }
}

int MockCamera::MockRequestRecord::flush()
{
    std::vector<std::shared_ptr<StreamBuffer>> buffers;

    auto time = std::chrono::steady_clock::now().time_since_epoch();
    std::chrono::nanoseconds ns = std::chrono::duration_cast<std::chrono::nanoseconds>(time);
    int64_t ts = ns.count();
    auto tempMeta = std::make_unique<Metadata>(*(mMockRequest->getMeta()));

    tempMeta->update(ANDROID_SENSOR_TIMESTAMP, &ts, 1);
    // Notify shutter to Mock camera, CAMERA3_MSG_SHUTTER
    NotifyMessage msg{mMockRequest->getCamera(), mMockRequest->getFrameNumber(), ts};
    MLOGD(LogGroupHAL, "[Mock][mockFrame:%u][Notify]:%s", mMockRequest->getFrameNumber(),
          msg.toString().c_str());
    msg.notify();

    for (int i = 0; i < mMockRequest->getOutBufferNum(); i++) {
        auto streamBuffer = mMockRequest->getStreamBuffer(i);
        if (!streamBuffer) {
            continue;
        }
        camera3_stream *raw = streamBuffer->getStream()->toRaw();
        if (!mPendingStreams.count(raw))
            continue;

        streamBuffer->setErrorBuffer();
        buffers.push_back(streamBuffer);
        NotifyMessage msg{mMockRequest->getCamera(), mMockRequest->getFrameNumber(), raw};
        MLOGI(LogGroupHAL, "[Mock][Flush]buffer error message: %s", msg.toString().c_str());
        msg.notify();
    }

    if (buffers.size()) {
        // Notify CAMERA3_MSG_ERROR to Mock camera, CAMERA3_MSG_ERROR_Result
        NotifyMessage msgResult{mMockRequest->getCamera(), mMockRequest->getFrameNumber(),
                                CAMERA3_MSG_ERROR_RESULT};
        MLOGD(LogGroupHAL, "[Mock][mockFrame:%u][Notify]:%s", mMockRequest->getFrameNumber(),
              msgResult.toString().c_str());
        msgResult.notify();

        LocalResult lr{mMockRequest->getCamera(), mMockRequest->getFrameNumber(), 0, buffers,
                       std::move(tempMeta)};
        MLOGI(LogGroupHAL, "[Mock][Flush]error result for incomplete: %s", lr.toString().c_str());
        mMockRequest->getCamera()->flushPendingRequests(&lr);
    }

    return 0;
}

int MockCamera::updatePendingMockRecords(std::unique_ptr<MockRequestRecord> &record,
                                         const CaptureResult *result)
{
    auto frame = record->mMockRequest->getFrameNumber();
    auto tempResult = const_cast<LocalResult *>(reinterpret_cast<const LocalResult *>(result));
    if (tempResult->getPartial() > record->mPartialResult)
        record->mPartialResult = tempResult->getPartial();
    bool isErrorReport = false;
    for (int i = 0; i < tempResult->getBufferNumber(); i++) {
        auto streamBuffer = tempResult->getStreamBuffer(i);
        camera3_stream *raw = streamBuffer->getStream()->toRaw();
        if (streamBuffer->getStatus() == CAMERA3_BUFFER_STATUS_ERROR) {
            if (!isErrorReport) {
                isErrorReport = true;
            }
            NotifyMessage msg{record->mMockRequest->getCamera(), frame, raw};
            MLOGD(LogGroupHAL, "%s[Mock][mockFrame:%u][Notify]:%s", getLogPrefix().c_str(), frame,
                  msg.toString().c_str());
            msg.notify();
        }

        if (!record->mPendingStreams.erase(raw)) {
            MLOGE(LogGroupHAL, "%s: try update record, but stream 0x%p not found",
                  getLogPrefixWithFrame(frame).c_str(), raw);
        }
    }
    auto oentry = record->mMockRequest->findInMeta(ANDROID_REQUEST_ID);
    if (oentry.count) {
        tempResult->updateMeta(oentry);
    }
    if (!record->mPendingStreams.size()) {
        if (!isErrorReport) {
            auto time = std::chrono::steady_clock::now().time_since_epoch();
            std::chrono::nanoseconds ns =
                std::chrono::duration_cast<std::chrono::nanoseconds>(time);
            int64_t ts = ns.count();

            tempResult->updateMeta(ANDROID_SENSOR_TIMESTAMP, &ts, 1);
            MLOGD(LogGroupHAL, "%s: expected ts %" PRId64, getLogPrefixWithFrame(frame).c_str(),
                  ts);

            // Notify shutter to Mock camera, CAMERA3_MSG_SHUTTER
            NotifyMessage msg{record->mMockRequest->getCamera(), frame, ts};
            MLOGD(LogGroupHAL, "%s[Mock][mockFrame:%u][Notify]:%s", getLogPrefix().c_str(), frame,
                  msg.toString().c_str());
            msg.notify();
        } else {
            // Notify CAMERA3_MSG_ERROR to Mock camera, CAMERA3_MSG_ERROR_Result
            NotifyMessage msg{record->mMockRequest->getCamera(), frame, CAMERA3_MSG_ERROR_RESULT};
            MLOGD(LogGroupHAL, "%s [Mock][mockFrame:%u][Notify]:%s", getLogPrefix().c_str(), frame,
                  msg.toString().c_str());
            msg.notify();
        }

        MLOGI(LogGroupHAL, "%s: job completed", getLogPrefixWithFrame(frame).c_str());
    }

    return 0;
}

int MockCamera::flushPendingRequests(const CaptureResult *result)
{
    std::lock_guard<std::recursive_mutex> lock{mMutex};
    auto frame = result->getFrameNumber();
    auto it = mPendingMockRecords.find(frame);
    if (it != mPendingMockRecords.end()) {
        auto &record = it->second;

        if (result->getPartial() > record->mPartialResult)
            record->mPartialResult = result->getPartial();

        for (int i = 0; i < result->getBufferNumber(); i++) {
            auto streamBuffer = result->getStreamBuffer(i);
            if (!streamBuffer) {
                continue;
            }
            camera3_stream *raw = streamBuffer->getStream()->toRaw();
            if (!record->mPendingStreams.erase(raw)) {
                MLOGW(LogGroupHAL, "%s: stream 0x%p not found",
                      getLogPrefixWithFrame(frame).c_str(), raw);
            }
        }
    }

    mRawCallbacks2Fwk->process_capture_result(mRawCallbacks2Fwk, result->toRaw());

    return 0;
}

void MockCamera::processCaptureResult(const CaptureResult *result)
{
    std::lock_guard<std::recursive_mutex> lock{mMutex};
    uint32_t frame = result->getFrameNumber();

    auto it = mPendingMockRecords.find(frame);
    if (it == mPendingMockRecords.end()) {
        MLOGW(LogGroupHAL, "%s: not found in pending record", getLogPrefixWithFrame(frame).c_str());
        return;
    }

    if (it == mPendingMockRecords.begin() && mHijackedResultMap.empty()) {
        MLOGI(LogGroupHAL, "[Fulfill]%s[ProcessResult]:%s", getLogPrefixWithFrame(frame).c_str(),
              result->toString().c_str());

        updatePendingMockRecords(it->second, result);
        mRawCallbacks2Fwk->process_capture_result(mRawCallbacks2Fwk, result->toRaw());
        mPendingMockRecords.erase(it);
    } else {
        camera_metadata *tempmeta = (const_cast<Metadata *>(result->getMeta()))->release();
        std::vector<std::shared_ptr<StreamBuffer>> outBuffers;
        for (int i = 0; i < result->getBufferNumber(); i++) {
            outBuffers.push_back(result->getStreamBuffer(i));
        }
        auto localResult = std::make_shared<LocalResult>(
            result->getCamera(), result->getFrameNumber(), result->getPartial(), outBuffers,
            std::move(std::make_unique<Metadata>(tempmeta, Metadata::TakeOver)));
        mHijackedResultMap.insert({frame, localResult});
        MLOGI(LogGroupHAL, "result mockFrame:%d has been hijacked", frame);

        int64_t rmfFrame = -1;
        for (auto &iter : mPendingMockRecords) {
            auto it = mHijackedResultMap.find(iter.first);
            if (it != mHijackedResultMap.end()) {
                rmfFrame = iter.first;
                MLOGI(LogGroupHAL, "[Fulfill]%s[ProcessResult]:%s",
                      getLogPrefixWithFrame(iter.first).c_str(), it->second->toString().c_str());

                updatePendingMockRecords(iter.second, it->second.get());
                mRawCallbacks2Fwk->process_capture_result(mRawCallbacks2Fwk, it->second->toRaw());
                mHijackedResultMap.erase(it);
            } else
                break;
        }

        if (rmfFrame != -1) {
            while (true) {
                if (mPendingMockRecords.empty())
                    break;
                auto it = mPendingMockRecords.begin();
                if (it->first > rmfFrame)
                    break;
                else {
                    mPendingMockRecords.erase(it);
                }
            }
        }
    }
}

int MockCamera::flush()
{
    MLOGI(LogGroupHAL, "[MiHalOp]%s[Flush]: Begin flush, pendding %zu", getLogPrefix().c_str(),
          mPendingMockRecords.size());

    std::unique_lock<std::recursive_mutex> lock{mMutex};
    // NOTE: If mPendingMockRecords is empty when flush, it may means that this mockcamera_A has
    // no tasks more than 30s. But may be another mockcamera_B has pending task now. So we do not
    // flush DARKROOM to avoid the postprocessor of mockcamera_B be flushed unexpectedly.
    if (!mPendingMockRecords.empty()) {
        lock.unlock();

        DARKROOM.flush();
        DARKROOM.clear();

        lock.lock();
    }

    for (auto &p : mPendingMockRecords) {
        auto &record = p.second;
        record->flush();
    }
    mPendingMockRecords.clear();
    mHijackedResultMap.clear();

    mFlushed.store(true);
    MLOGI(LogGroupHAL, "[MiHalOp]%s[Flush]: End flush", getLogPrefix().c_str());
    return 0;
}

void MockCamera::signalStreamFlush(uint32_t numStream, const camera3_stream_t *const *streams)
{
    MLOGE(LogGroupHAL, "[MiHalOp] NOT IMPLEMENTED for mock camera %s", getId().c_str());
}

} // namespace mihal
