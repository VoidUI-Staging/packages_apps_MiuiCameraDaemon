/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiPostProcCallbacks.h"

#include "MiPostProcSessionIntf.h"

namespace mialgo {

MiPostProcCallBacks::MiPostProcCallBacks() : mSessionIntf(NULL)
{
    MLOGI("[HIDL]MiPostProcCallBacks constructor called");
}

MiPostProcCallBacks::~MiPostProcCallBacks()
{
    MLOGI("[HIDL]MiPostProcCallBacks destructor called");
}

Return<void> MiPostProcCallBacks::notifyCallback(const PostProcResult &result)
{
    int64_t timeStamp = result.timeStampUs;
    uint32_t streamIdx = result.streamIdx;
    MLOGI("E requestId: %d type: %d, timestamp: %" PRIu64 ", streamIdx: %d, mSessionIntf: %p",
          result.resultId, result.type, timeStamp, streamIdx, mSessionIntf);

    if (mSessionIntf != NULL) {
        MiPostProcSessionIntf *session = (MiPostProcSessionIntf *)mSessionIntf;
        CallbackType type;
        if (result.type == NotifyType::RELEASE_INPUT_BUFFER_TYPE) {
            type = CallbackType::SOURCE_CALLBACK;
            session->enqueueBuffer(type, timeStamp, streamIdx);
        } else if (result.type == NotifyType::RELEASE_OUTPUT_BUFFER_TYPE) {
            type = CallbackType::SINK_CALLBACK;
            session->enqueueBuffer(type, timeStamp, streamIdx);
        } else if (result.type == NotifyType::METADATA_RESULT_TYPE) {
            if (!result.metadata.size()) {
                MLOGE("result metadata is empty");
                return Void();
            }

            MiaResult miaResult = {
                .type = MIA_RESULT_METADATA,
                .resultId = result.resultId,
                .timeStamp = result.timeStampUs,
            };
            miaResult.metaData.clear();
            std::string resultKey = "MiviFwkResult";
            std::string resultValue = "{";
            resultValue += "num:" + to_string(result.metadata.size());
            for (int i = 0; i < result.metadata.size(); i++) {
                resultValue += " ";
                resultValue += result.metadata[i];
            }
            resultValue += "}";
            miaResult.metaData.insert(std::make_pair(resultKey, resultValue));
            session->onSessionCallback(MIA_RESULT_METADATA, result.resultId, (void *)&miaResult);
        } else if (result.type == NotifyType::ANCHOR_RESULT_TYPE) {
            MiaResult miaResult = {
                .type = MIA_ANCHOR_CALLBACK,
                .resultId = result.resultId,
                .timeStamp = result.timeStampUs,
            };
            session->onSessionCallback(MIA_RESULT_METADATA, result.resultId, (void *)&miaResult);
        } else if (result.type == NotifyType::ABNORMAL_RESULT_TYPE) {
            MiaResult miaResult = {
                .type = MIA_ABNORMAL_CALLBACK,
                .resultId = result.resultId,
                .timeStamp = result.timeStampUs,
            };
            session->onSessionCallback(MIA_ABNORMAL_CALLBACK, result.resultId, (void *)&miaResult);
        } else {
            MLOGE("unsupported result type: %d", result.type);
        }
    }

    MLOGI("X");
    return Void();
}

Return<void> MiPostProcCallBacks::getOutputBuffer(OutSurfaceType type, int64_t timeStampUs,
                                                  getOutputBuffer_cb hidlCb)
{
    MLOGI("getOutputBuffer type: %d, timestamp: %" PRIu64, type, timeStampUs);
    const native_handle_t *nativeHandle = nullptr;
    Error error = Error::INVALID_HANDLE;
    if (mSessionIntf != NULL) {
        SurfaceType surfaceType = (SurfaceType)type;
        MiPostProcSessionIntf *session = (MiPostProcSessionIntf *)mSessionIntf;
        nativeHandle = session->dequeuBuffer(surfaceType, timeStampUs);
        error = Error::NONE;
    }

    hidl_handle buffer_hidl(nativeHandle);
    hidlCb(buffer_hidl, error);
    MLOGI("getOutputBuffer end, handle: %p, %p", nativeHandle, buffer_hidl.getNativeHandle());
    return Void();
}

IMiPostProcCallBacks *HIDL_FETCH_IMiPostProcCallBacks(const char * /* name */)
{
    return new MiPostProcCallBacks();
}

} // namespace mialgo
