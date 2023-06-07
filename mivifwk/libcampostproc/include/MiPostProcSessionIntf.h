/*
 * Copyright (c) 2018. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef __MI_POSTPROCSESSIONINTF__
#define __MI_POSTPROCSESSIONINTF__

#include <fmq/MessageQueue.h>

#include <queue>

#include "MiPostProcCallbacks.h"

using ::android::hardware::kSynchronizedReadWrite;
using ::android::hardware::MessageQueue;
namespace mialgo {

typedef enum {
    SnapshotType = 0,
    PreviewType = 1,
    VideType = 2,
    MaxType = 3,
} MiFrameType;

enum class CallbackType { SOURCE_CALLBACK = 0, SINK_CALLBACK = 1 };

typedef CDKResult (*mia_thread_func_t)(void *);

enum CameraCombinationMode {
    CAM_COMBMODE_UNKNOWN = 0x0,
    CAM_COMBMODE_REAR_WIDE = 0x01,
    CAM_COMBMODE_REAR_TELE = 0x02,
    CAM_COMBMODE_REAR_ULTRA = 0x03,
    CAM_COMBMODE_REAR_MACRO = 0x04,
    CAM_COMBMODE_FRONT = 0x11,
    CAM_COMBMODE_FRONT_AUX = 0x12,
    CAM_COMBMODE_REAR_SAT_WT = 0x201,
    CAM_COMBMODE_REAR_SAT_WU = 0x202,
    CAM_COMBMODE_FRONT_SAT = 0x203,
    CAM_COMBMODE_REAR_SAT_T_UT = 0x204,
    CAM_COMBMODE_REAR_BOKEH_WT = 0x301,
    CAM_COMBMODE_REAR_BOKEH_WU = 0x302,
    CAM_COMBMODE_FRONT_BOKEH = 0x303,
};

// copy from chi.h TODO: defined our self ownded mode. But it will need application to transacte.
typedef enum StreamConfigMode {
    StreamConfigModeNormal = 0x0000, ///< Normal stream configuration operation mode
    StreamConfigModeConstrainedHighSpeed =
        0x0001, ///< Special constrained high speed operation mode for devices that can
                ///  not support high speed output in NORMAL mode
    StreamConfigModeVendorStart =
        0x8000,                     ///< First value for vendor-defined stream configuration modes
    StreamConfigModeSAT = 0x8001,   ///< Xiaomi SAT
    StreamConfigModeBokeh = 0x8002, ///< Xiaomi Bokeh
    StreamConfigModeMiuiManual = 0x8003,            ///< Xiaomi Manual mode
    StreamConfigModeMiuiVideo = 0x8004,             ///< Xiaomi Video mode
    StreamConfigModeMiuiZslFront = 0x8005,          ///< Xiaomi zsl for front camera
    StreamConfigModeMiuiFaceUnlock = 0x8006,        ///< Xiaomi faceunlock for front camera
    StreamConfigModeMiuiQcfaFront = 0x8007,         ///< Xiaomi Qcfa for front camera
    StreamConfigModeMiuiPanorama = 0x8008,          ///< Xiaomi Panorama mode
    StreamConfigModeMiuiVideoBeauty = 0x8009,       ///< Xiaomi video beautifier
    StreamConfigModeMiuiSuperNight = 0x800A,        ///< Xiaomi super night
    StreamConfigModeMiuiShortVideo = 0x8030,        ///< Xiaomi short video mode
    StreamConfigModeHFR120FPS = 0x8078,             ///< Xiaomi HFR 120fps
    StreamConfigModeHFR240FPS = 0x80F0,             ///< Xiaomi HFR 240fps
    StreamConfigModeFrontBokeh = 0x80F1,            ///< Xiaomi single front Bokeh
    StreamConfigModeUltraPixelPhotography = 0x80F3, ///< Xiaomi zsl for rear qcfa camera, only used
                                                    ///< for upscale binning size to full size
    StreamConfigModeVendorEnd = 0xEFFF, ///< End value for vendor-defined stream configuration modes
    StreamConfigModeQTIStart = 0xF000,  ///< First value for QTI-defined stream configuration modes
    StreamConfigModeQTITorchWidget = 0xF001, ///< Operation mode for Torch Widget.
    StreamConfigModeVideoHdr = 0xF002,       ///< Video HDR On
    StreamConfigModeQTIEISRealTime =
        0xF004, ///< Operation mode for Real-Time EIS recording usecase.
    StreamConfigModeQTIEISLookAhead =
        0xF008,                           ///< Operation mode for Look Ahead EIS recording usecase
    StreamConfigModeQTIFOVC = 0xF010,     ///< Field of View compensation in recording usecase.
    StreamConfigModeSensorMode = 0xF0000, ///< Sensor Mode selection for >=60fps
                                          ///  require 4 bits for sensor mode index
} STREAMCONFIGMODE;

/*
 * SurfaceWrapper: Encapsulate surface interface and manager the
 * surface buffer associated with MiaImage
 */
class SurfaceWrapper
{
public:
    SurfaceWrapper(ANativeWindow *window, SurfaceType type);
    ~SurfaceWrapper();

    CDKResult setup();
    const native_handle_t *dequeueBuffer(int64_t timestamp);
    CDKResult enqueueBuffer(int64_t timestamp);
    uint32_t getWidth() { return mWidth; }
    uint32_t getHeight() { return mHeight; }
    uint32_t getFormat() { return mFormat; }
    SurfaceType getType() { return mType; }

private:
    ANativeWindow *mANWindow;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mFormat;
    SurfaceType mType;
    mutable Mutex mLock;
    std::map<ANativeWindowBuffer *, int64_t> mANWindowBufferQs;
};

class MiPostProcSessionIntf
{
public:
    MiPostProcSessionIntf();

    ~MiPostProcSessionIntf();

    CDKResult initialize(GraphDescriptor *gd, SessionOutput *output, MiaSessionCb *cb);
    CDKResult processFrame(MiaFrame *frame);
    CDKResult preProcessFrame(PreProcessInfo *params);
    CDKResult processFrameWithSync(ProcessRequestInfo *processRequestInfo);
    CDKResult destory();
    void setProcessStatus();
    void flush();
    CDKResult quickFinish(int64_t timeStamp, bool needImageResult);

    void enqueueBuffer(CallbackType type, int64_t timeStampUs, uint32_t stremIdx);

    const native_handle_t *dequeuBuffer(SurfaceType type, int64_t timeStampUs);

    void onSessionCallback(ResultType type, int64_t timeStamp, void *msgData);

    void setEngineBypass(bool bypass) { mEngineBypass = bypass; }

    void bypassProcess();
    void bypass(const sp<MiaFrameWrapper> &frameWrp);

private:
    CDKResult buildSurface(void **surfaces);
    void returnOutImageToSurface(int64_t timeStamp, uint32_t stremIdx);

    SurfaceWrapper *mANWindows[SURFACE_TYPE_COUNT];
    uint32_t mAcitveSurfaceCnt;
    MiaSessionCb *mSessionCb;

    uint32_t mCameraMode;
    uint32_t mOperationMode;
    sp<IMiPostProcSession> mPostSessionsProxy;
    std::map<int64_t, sp<MiaFrameWrapper>> mProcessedFrameWrpMap;

    // internal typedefs
    using MetadataQueue = MessageQueue<uint8_t, kSynchronizedReadWrite>;
    std::shared_ptr<MetadataQueue> mLogicMetadataQueue;
    std::shared_ptr<MetadataQueue> mPhysicalMetadataQueue;
    std::shared_ptr<MetadataQueue> mMetadataQueue; // for preprocess

    // add them for bypass
    Mutex mBypassLock;
    bool mProcessStatusError;
    bool mEngineBypass;
    int32_t mIsBokeh;         // 0->not bokeh  1->front bokeh  2->dual bokeh
    int32_t mNeedInputNumber; // Record the number of frames needed for each shot
    int32_t mInputTimes;      // Record the number of frames entered
    std::queue<int64_t> mTimestampQueue;
    std::queue<uint32_t> mStreamIdxQueue;

    std::map<int64_t, std::string> mFirstTimestampsOfRequests;
    std::string mLastRequestName;

    Mutex mTaskLock;
};

} // namespace mialgo

#endif /* __MI_POSTPROCSESSIONINTF__ */
