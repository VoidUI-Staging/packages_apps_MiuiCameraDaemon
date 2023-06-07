#ifndef __BGSERVICE_H__
#define __BGSERVICE_H__

#include <ResultCallback.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <utils/Mutex.h>
#include <vendor/xiaomi/hardware/bgservice/1.0/IBGService.h>

using namespace mihal;

namespace vendor::xiaomi::hardware::bgservice::implementation {

extern const char *ecoCloudInfoPath;

using ::android::sp;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_vec;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::vendor::xiaomi::hardware::bgservice::V1_0::CallbackData;
using ::vendor::xiaomi::hardware::bgservice::V1_0::IBGService;
using ::vendor::xiaomi::hardware::bgservice::V1_0::IEventCallback;

struct BGService : public V1_0::IBGService, public android::hardware::hidl_death_recipient
{
    // Methods from ::vendor::xiaomi::hardware::bgservice::V1_0::IBGService follow.
    Return<int32_t> setEventCallback(int32_t clientId, const sp<IEventCallback> &callback) override;

    Return<void> setCapabilities(const hidl_string &info) override;
    // Methods from ::android::hidl::base::V1_0::IBase follow.
public:
    virtual void serviceDied(uint64_t cookie,
                             const android::wp<android::hidl::base::V1_0::IBase> &who) override;
};

enum BufferStatus {
    Completed = 0,
    Failed = 1,
};

// FIXME: most likely delete, this is only for passthrough implementations
extern "C" IBGService *HIDL_FETCH_IBGService(const char *name);

/*
 * Wrapper for calling BGServiceMgr
 */
class BGServiceWrap
{
public:
    static BGServiceWrap *getInstance();

    Return<int32_t> setEventCallback(const sp<IEventCallback> &callback, std::string &version,
                                     int32_t clientId);

    void resetCallback();

    void removeCallback(int32_t clientId);

    bool onResultCallback(const ResultData &result);

    static bool resultCallback(const ResultData &result);

    bool notifySnapshotAvailability(int32_t available);

    bool onCaptureCompleted(std::string fileName, uint32_t frameNumber);

    bool onCaptureFailed(std::string fileName, uint32_t frameNumber);

    bool hasEeventCallback(int32_t clientId);

    int32_t getClientId();
    void setClientId(int32_t clientId);

private:
    BGServiceWrap(){};
    ~BGServiceWrap(){};
    sp<IEventCallback> getCallback(int32_t clientId);
    bool notifyStatus(std::string flieName, uint32_t frameNumber, BufferStatus status);

private:
    std::map<int32_t, sp<IEventCallback>> mEventCallbacks;
    std::string mVersion;
    mutable android::Mutex mLock;
    int32_t mLastClientId = (int32_t)0x7FFFFFFF;

}; // class BGServiceWrap

} // namespace vendor::xiaomi::hardware::bgservice::implementation

#endif
