// FIXME: your file license if you have one

#include "BGService.h"

// #include "EventCallbackMgr.h"
// #include "MiDebugUtils.h"
#include <log/log.h>

using namespace android;
// using namespace midebug;

namespace vendor::xiaomi::hardware::bgservice::implementation {

#undef LOG_TAG
#define LOG_TAG "BGService_mizone"

// Methods from ::vendor::xiaomi::hardware::bgservice::V1_0::IBGService follow.
Return<int32_t> BGService::setEventCallback(int32_t clientId, const sp<IEventCallback> &callback)
{
    // BGServiceWrap::getInstance()->resetCallback();

    // link to death notification for callback
    if (callback != nullptr) {
        ::android::hardware::Return<bool> linked = callback->linkToDeath(this, (uint64_t)clientId);
        if (!linked.isOk()) {
            ALOGE("Transaction error in linking to mEventCallback death: %s, clientId(%d)",
                  linked.description().c_str(), clientId);
        } else if (!linked) {
            ALOGE("Unable to link to mEventCallback death notifications");
        }
    } else {
        ALOGE("App HIDL EventCallback function is empty, callback=%p", callback.get());
    }
    ALOGI("clientId: %d, callback: %p", clientId, callback.get());

    std::string version = "1_0";
    BGServiceWrap::getInstance()->setEventCallback(callback, version, clientId);
    return int32_t{};
}

Return<void> BGService::setCapabilities(const hidl_string &info)
{
    CloudServiceWrap::getInstance()->setCloudInfo(info);
    return Void();
}

void BGService::serviceDied(uint64_t cookie, const wp<hidl::base::V1_0::IBase> &who __unused)
{
    int32_t clientId = (int32_t)cookie;
    ALOGW("BGService IEventCallback client %d serviceDied!!", clientId);

    BGServiceWrap::getInstance()->removeCallback(clientId);

    // EventData data = {
    //     .type = ClientDied,
    //     .clientId = clientId,
    // };

    // EventCallbackMgr::getInstance()->onEventNotify(data);
}

// Methods from ::android::hidl::base::V1_0::IBase follow.

IBGService *HIDL_FETCH_IBGService(const char * /* name */)
{
    return new BGService();
}

BGServiceWrap *BGServiceWrap::getInstance()
{
    static BGServiceWrap inst;
    return &inst;
}

Return<int32_t> BGServiceWrap::setEventCallback(const sp<IEventCallback> &callback,
                                                std::string &version, int32_t clientId)
{
    mVersion = version;
    if (callback == nullptr) {
        ALOGE("App HIDL EventCallback function is empty with clientId=%d", clientId);
        return int32_t{};
    }
    {
        Mutex::Autolock _l(mLock);
        mLastClientId = clientId;
        mEventCallbacks.insert({clientId, callback});
    }
    return int32_t{};
}

void BGServiceWrap::resetCallback()
{
    Mutex::Autolock _l(mLock);
    mEventCallbacks.clear();
}

void BGServiceWrap::removeCallback(int32_t clientId)
{
    Mutex::Autolock _l(mLock);
    auto it = mEventCallbacks.find(clientId);
    if (it != mEventCallbacks.end()) {
        mEventCallbacks.erase(clientId);
    }
}

sp<IEventCallback> BGServiceWrap::getCallback(int32_t clientId)
{
    Mutex::Autolock _l(mLock);
    auto it = mEventCallbacks.find(clientId);
    if (it == mEventCallbacks.end()) {
        return nullptr;
    }

    sp<IEventCallback> callback = it->second;
    return callback;
}

int32_t BGServiceWrap::getClientId()
{
    Mutex::Autolock _l(mLock);
    int32_t clientId = mLastClientId;
    return clientId;
}

void BGServiceWrap::setClientId(int32_t clientId)
{
    Mutex::Autolock _l(mLock);
    mLastClientId = clientId;
}

bool BGServiceWrap::onResultCallback(const ResultData &result)
{
    sp<IEventCallback> callback = getCallback(mLastClientId);
    if (callback == nullptr) {
        ALOGE("[Mock]App HIDL ver %s EventCallback function is empty!", mVersion.c_str());
        return false;
    }

    Return<bool> ret = false;
    // ALOGI("[Mock]Callback via %s interface, fwkframe(%d), timestamp(%" PRId64 ") isParallel: %d",
    //       mVersion.c_str(), result.frameNumber, result.timeStampUs, result.isParallelCamera);

    CallbackData data = {
        .cameraId = result.cameraId,
        .type = 0,
        .frameNumber = result.frameNumber,
        .sessionId = result.sessionId,
        .timeStampUs = result.timeStampUs,
        .isParallelCamera = result.isParallelCamera,
    };
    data.images.resize(result.imageForamts.size());
    for (int i = 0; i < data.images.size(); i++) {
        data.images[i].format = result.imageForamts[i].format;
        data.images[i].width = result.imageForamts[i].width;
        data.images[i].height = result.imageForamts[i].height;
    }

    ret = callback->notifyCallback(data);

    if (!ret.isOk()) {
        ALOGE("[Mock]Transaction error in IEventCallback::onCompleted: %s",
              ret.description().c_str());
        return false;
    }

    return true;
}

bool BGServiceWrap::resultCallback(const ResultData &result)
{
    return BGServiceWrap::getInstance()->onResultCallback(result);
}

bool BGServiceWrap::notifySnapshotAvailability(int32_t available)
{
    sp<IEventCallback> callback = getCallback(mLastClientId);

    if (callback == nullptr) {
        ALOGE("App HIDL ver %s EventCallback function is empty!", mVersion.c_str());
        return false;
    }

    Return<bool> ret = false;
    ALOGI("snapshot availability=%d", available);

    ret = callback->notifySnapshotAvailability(available);
    if (!ret.isOk()) {
        ALOGE("Transaction error in IEventCallback::onCompleted: %s", ret.description().c_str());
        return false;
    }

    return true;
}

bool BGServiceWrap::onCaptureCompleted(std::string fileName, uint32_t frameNumber)
{
    ALOGI("fileName = %s, fwkFrame:%u buffer come back", fileName.c_str(), frameNumber);

    return notifyStatus(fileName, frameNumber, BufferStatus::Completed);
}

bool BGServiceWrap::onCaptureFailed(std::string fileName, uint32_t frameNumber)
{
    ALOGI("fileName = %s, fwkFrame:%u snapshot fail", fileName.c_str(), frameNumber);

    return notifyStatus(fileName, frameNumber, BufferStatus::Failed);
}

bool BGServiceWrap::notifyStatus(std::string fileName, uint32_t frameNumber, BufferStatus status)
{
    sp<IEventCallback> callback = getCallback(mLastClientId);

    if (callback == nullptr) {
        ALOGE("App HIDL ver %s EventCallback function is empty!", mVersion.c_str());
        return false;
    }
    Return<bool> ret = false;
    switch (status) {
    case BufferStatus::Completed:
        ret = callback->onCaptureCompleted(fileName, frameNumber);
        break;
    case BufferStatus::Failed:
        ret = callback->onCaptureFailed(fileName, frameNumber);
        break;
    default:
        break;
    }
    if (!ret.isOk()) {
        ALOGE("Transaction error in IEventCallback::onCompleted: %s", ret.description().c_str());
        return false;
    }

    return true;
}

bool BGServiceWrap::hasEeventCallback(int32_t clientId)
{
    sp<IEventCallback> callback = getCallback(clientId);
    if (callback) {
        return true;
    } else {
        return false;
    }
}

CloudServiceWrap *CloudServiceWrap::getInstance()
{
    static CloudServiceWrap cloudServiceWrap;
    return &cloudServiceWrap;
}

void CloudServiceWrap::setCloudInfo(std::string info)
{
    std::lock_guard<std::mutex> infoLock(mCloudInfoMutex);
    mCloudInfo = info;
}

std::string CloudServiceWrap::getCloudInfo()
{
    std::lock_guard<std::mutex> infoLock(mCloudInfoMutex);
    return mCloudInfo;
}

//
} // namespace vendor::xiaomi::hardware::bgservice::implementation
