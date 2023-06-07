
#ifndef __CAMERA_DEVICE_H__
#define __CAMERA_DEVICE_H__

#include <errno.h>
#include <hardware/camera3.h>

#include <memory>
#include <string>

#include "Camera3Plus.h"
#include "LogUtil.h"

namespace mihal {

class MiHal3BufferMgr;
class AsyncAlgoSession;

enum FusionCombination {
    UltraWideAndWide,
    WideAndTele,
    WideAndTele5x,
    TeleAndTele5x,
};

struct CameraCallback
{
    std::function<void(const CaptureResult *result)> processResultCb;
    std::function<void(const NotifyMessage *)> notifyCb;
    std::function<std::shared_ptr<StreamBuffer>(uint32_t frame, uint32_t streamIdx)>
        requestStreambuffersCb;
};

class CameraDevice : public camera3_callback_ops
{
public:
    enum CameraRoleId {
        RoleIdTypeNone = -1,
        RoleIdRearWide = 0,
        RoleIdFront = 1,
        RoleIdRearTele = 20,
        RoleIdRearUltra = 21,
        RoleIdRearMacro = 22,
        RoleIdRearTele4x = 23,
        RoleIdRearMacro2x = 24,
        RoleIdRearDepth = 25,
        RoleIdRearTele2x = 26,
        RoleIdRearTele3x = 27,
        RoleIdRearTele5x = 28,
        RoleIdRearTele10x = 29,
        RoleIdFrontAux = 40,
        RoleIdRearSat = 60,
        RoleIdRearBokeh2x = 61, // w+t half body
        RoleIdRearSatVideo = 62,
        RoleIdRearBokeh1x = 63,  // w+d || w+u full body
        RoleIdRear3PartSat = 64, ///< 3Part sat
        RoleIdFrontSat = 80,
        RoleIdFrontBokeh = 81,
        RoleIdRearVt = 100,
        RoleIdFrontVt = 101,
        RoleIdParallelVt = 102,
        RoleIdMock = 108,
        RoleIdRearSatUWT = 120,
        RoleIdRearSatUWTT4x = 180,
        RoleIdRearAONCam = 200,
        RoleIdFrontAONCam = 201,
    };

    /**
     * Bit Mask for HighQualityQuickShot Configure
     * Bit[0]     - Support MFNR/LLS in SAT Mode
     * Bit[1]     - Support HDR in SAT Mode
     * Bit[2]     - Support SR in SAT Mode
     * Bit[3]     - Support SuperNightSE in SAT Mode
     * Bit[4]     - Support HDRSR in SAT Mode
     * Bit[5~7]   - reserve
     * Bit[8]     - Support Bokeh MFNR in Back Camera
     * Bit[9]     - Support Bokeh HDR in Back Camera
     * Bit[10]    - Support MFNR in Front Camera
     * Bit[11]    - Support HDR in Front Camera
     * Bit[12]    - Support Bokeh in Front Camera
     * Bit[13]    - Support Macro Mode
     * BIt[14]    - Support Professional Mode
     * Bit[15]    - reserve
     * Bit[16~19] - HighQualityQuickShot Queue Length(max number of HighQualityQuickShot)
     * Bit[20]    - Support Reuse RDI Buffer
     * Bit[21]    - Support Limit MFNR Input Frames
     * Bit[22~31] - reserve
     **/
    enum HighQualityQuickShotType {
        QS_SUPPORT_NONE = 0,
        QS_SUPPORT_SAT_MFNR = 0x1,
        QS_SUPPORT_SAT_HDR = 0x2,
        QS_SUPPORT_SAT_SR = 0x4,
        QS_SUPPORT_SAT_SUPER_NIGHT = 0x8,
        QS_SUPPORT_SAT_HDRSR = 0x10,
        QS_SUPPORT_BACK_BOKEH_MFNR = 0x100,
        QS_SUPPORT_BACK_BOKEH_HDR = 0x200,
        QS_SUPPORT_FRONT_MFNR = 0x400,
        QS_SUPPORT_FRONT_HDR = 0x800,
        QS_SUPPORT_FRONT_BOKEH = 0x1000,
        QS_SUPPORT_MACRO_MODE = 0x2000,
        QS_SUPPORT_PRO_MODE = 0x4000,
        QS_QUEUE_LENGTH_MASK = 0xF0000,
    };

    /**
     * Bit Mask for HighQualityQuickShotDelayTime Configure
     * Bit[0 ~ 3]   - DelayTime XX ms of Bokeh MFNR in Back Camera
     * Bit[4 ~ 7]   - DelayTime XX ms of Bokeh MFNR in Front Camera
     * Bit[8 ~ 11]  - DelayTime XX ms of Back Normal Capture
     * Bit[12 ~ 15] - DelayTime XX ms of HDR in Front Camera
     * Bit[16 ~ 19] - DelayTime XX ms of HDR in Back Camera
     * Bit[20 ~ 23] - DelayTime XX ms of SuperNightSE in Back Camera
     * Bit[24 ~ 27] - DelayTime XX ms of SR in Back Camera
     * Bit[28 ~ 31] - DelayTime XX ms of Front Normal Capture
     * Bit[32 ~ 35] - DelayTime XX ms of Macro Mode in Back Camera
     * Bit[36 ~ 39] - DelayTime XX ms of Bokeh HDR in Back Camera
     * Bit[40 ~ 43] - DelayTime XX ms of HDRSR in Back Camera
     * Bit[44 ~ 47] - DelayTime XX ms of Professional Mode in Back Camera
     * Bit[48 ~ 63] - reserve
     **/
    enum HighQualityQuickShotDelayTime {
        QS_DELAYTIME_NONE = 0,
        QS_BACK_BOKEH_MFNR_DELAY_TIME = 0xFL,
        QS_FRONT_BOKEH_MFNR_DELAY_TIME = 0xF0L,
        QS_BACK_NORMAL_CAPTURE_DELAY_TIME = 0xF00L,
        QS_FRONT_HDR_DELAY_TIME = 0xF000L,
        QS_BACK_HDR_DELAY_TIME = 0xF0000L,
        QS_BACK_SUPER_NIGHT_SE_DELAY_TIME = 0xF00000L,
        QS_BACK_SR_CAPTURE_DELAY_TIME = 0xF000000L,
        QS_FRONT_NORMAL_CAPTURE_DELAY_TIME = 0xF0000000L,
        QS_BACK_MACRO_MODE_DELAY_TIME = 0xF00000000L,
        QS_BACK_BOKEH_HDR_DELAY_TIME = 0xF000000000L,
        QS_BACK_HDRSR_DELAY_TIME = 0xF0000000000L,
        QS_BACK_PRO_MODE_DELAY_TIME = 0xF00000000000L,
    };

    CameraDevice(std::string cameraId);
    virtual ~CameraDevice() = default;

    virtual std::string getName() const { return "CameraDevice"; }

    std::string getLogPrefix() const
    {
        if (!mLogPrefix.empty()) {
            return mLogPrefix;
        } else {
            std::ostringstream str;
            str << "[" << getName() << "][ID:" << getId() << "]";
            mLogPrefix = str.str();
            return mLogPrefix;
        }
    }

    virtual int getCameraInfo(camera_info *info) const = 0;
    hw_device_t *getCommonHwDevice() { return &mHal3Device.common; }
    virtual int setTorchMode(bool) { return -EINVAL; };
    virtual int turnOnTorchWithStrengthLevel(int32_t) { return -EINVAL; };
    virtual int getTorchStrengthLevel(int32_t *) { return -EINVAL; };
    virtual int open() = 0;
    virtual int close() { return -ENOEXEC; };
    virtual int initialize(const camera3_callback_ops *callbacks)
    {
        MLOGI(LogGroupHAL, "initialization for camera %s", getId().c_str());
        mRawCallbacks2Fwk = callbacks;
        return 0;
    }

    virtual int initialize(const CameraCallback callbacks)
    {
        MLOGI(LogGroupHAL, "initialization for camera %s", getId().c_str());
        mCallbacks2Client = callbacks;
        return 0;
    }
    virtual int configureStreams(std::shared_ptr<StreamConfig> config) { return -ENOEXEC; }
    virtual int configureStreams(StreamConfig *config) { return -ENOEXEC; }
    virtual const camera_metadata *defaultSettings(int) const { return nullptr; }
    virtual int processCaptureRequest(std::shared_ptr<CaptureRequest>) { return -ENOEXEC; }
    virtual int processCaptureRequest(CaptureRequest *) { return -ENOEXEC; }
    virtual void dump(int) {}
    virtual int flush() { return -ENOEXEC; }
    virtual void signalStreamFlush(uint32_t numStream, const camera3_stream_t *const *streams) {}
    virtual void processCaptureResult(const camera3_capture_result *result)
    {
        if (mRawCallbacks2Fwk) {
            mRawCallbacks2Fwk->process_capture_result(mRawCallbacks2Fwk, result);
        }
    }

    virtual void processCaptureResult(const CaptureResult *result)
    {
        if (mRawCallbacks2Fwk) {
            mRawCallbacks2Fwk->process_capture_result(mRawCallbacks2Fwk, result->toRaw());
        } else if (mCallbacks2Client.processResultCb) {
            mCallbacks2Client.processResultCb(result);
        } else {
            MLOGE(LogGroupHAL, "no valid callbacks registered!!");
        }
    }

    // for MockCam
    virtual void processCaptureResult(std::shared_ptr<const CaptureResult> result)
    {
        if (mRawCallbacks2Fwk) {
            mRawCallbacks2Fwk->process_capture_result(mRawCallbacks2Fwk, result->toRaw());
        } else if (mCallbacks2Client.processResultCb) {
            mCallbacks2Client.processResultCb(result.get());
        } else {
            MLOGE(LogGroupHAL, "no valid callbacks registered!!");
        }
    }

    virtual void notify(const camera3_notify_msg *msg)
    {
        if (mRawCallbacks2Fwk) {
            mRawCallbacks2Fwk->notify(mRawCallbacks2Fwk, msg);
        }
    }

    virtual camera3_buffer_request_status requestStreamBuffers(
        uint32_t num_buffer_reqs, const camera3_buffer_request *buffer_reqs,
        /*out*/ uint32_t *num_returned_buf_reqs,
        /*out*/ camera3_stream_buffer_ret *returned_buf_reqs)
    {
        if (!mRawCallbacks2Fwk->request_stream_buffers)
            return CAMERA3_BUF_REQ_FAILED_UNKNOWN;

        return mRawCallbacks2Fwk->request_stream_buffers(mRawCallbacks2Fwk, num_buffer_reqs,
                                                         buffer_reqs, num_returned_buf_reqs,
                                                         returned_buf_reqs);
    }

    virtual std::shared_ptr<StreamBuffer> requestStreamBuffers(uint32_t frame, uint32_t streamIdx)
    {
        if (mCallbacks2Client.requestStreambuffersCb)
            return mCallbacks2Client.requestStreambuffersCb(frame, streamIdx);
        else
            return std::shared_ptr<StreamBuffer>(nullptr);
    }

    virtual void returnStreamBuffers(uint32_t num_buffers,
                                     const camera3_stream_buffer *const *buffers)
    {
        if (!mRawCallbacks2Fwk->request_stream_buffers)
            return;

        mRawCallbacks2Fwk->return_stream_buffers(mRawCallbacks2Fwk, num_buffers, buffers);
    }

    virtual bool isMockCamera() const { return false; }
    virtual void processMockPendingFrame(uint32_t frame) {}

    virtual int flushPendingRequests(const CaptureResult *result) { return 0; }

    template <class... Args>
    camera_metadata_ro_entry findInStaticInfo(Args &&... args) const
    {
        camera_info info;
        getCameraInfo(&info);

        const Metadata meta{info.static_camera_characteristics};

        return meta.find(std::forward<Args>(args)...);
    }

    std::vector<CameraDevice *> getPhyCameras() const;

    std::string getId() const { return mId; }
    int32_t getLogicalId() const;
    int32_t getRoleId();
    int32_t getPartialResultCount();
    CameraDevice *getMostTelePhyCamera() const;
    bool isFrontCamera() const;
    bool isMultiCamera() const;
    bool isSupportAnchorFrame() const;
    int getAnchorFrameMask() const;
    int getQuickShotSupportMask() const;
    uint64_t getQuickShotDelayTimeMask() const;
    std::vector<CameraDevice *> getCurrentFusionCameras(FusionCombination fc);
    bool supportBufferManagerAPI();

    virtual MiHal3BufferMgr *getFwkStreamHal3BufMgr() { return nullptr; }

protected:
    std::string mId;
    int32_t mRoleId;
    // camera3 device linking mihal and framework
    camera3_device mHal3Device;
    // camera3 callbacks linking mihal and framework
    const camera3_callback_ops *mRawCallbacks2Fwk;

    // camera3 callbacks linking mihal and the client with callable objects, and
    // it is mutual exclusive with mRawCallbacks2Fwk
    CameraCallback mCallbacks2Client;
    mutable std::vector<CameraDevice *> mPhyCameras;

    mutable std::string mLogPrefix;

private:
    int32_t mNumPartialResults;
    uint32_t mBufferManagerVersion;
};

} // namespace mihal

#endif // __CAMERA_DEVICE_H__
