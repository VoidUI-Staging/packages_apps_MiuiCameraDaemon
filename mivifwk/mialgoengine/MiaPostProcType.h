////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2020 Xiaomi Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Xiaomi Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __MI_POSTPROC_TYPE__
#define __MI_POSTPROC_TYPE__

#include <cutils/native_handle.h>
#include <fcntl.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <system/camera_metadata.h>
#include <utils/List.h>

#include <vector>

#include "VendorMetadataParser.h"

namespace mialgo2 {

#define MIA_VISIBILITY_PUBLIC __attribute__((visibility("default")))

static const int32_t MiFormatsMaxPlanes = 3;

typedef enum StreamConfigMode {
    StreamConfigModeNormal = 0x0000,                ///< Normal stream configuration operation mode
    StreamConfigModeSAT = 0x8001,                   ///< Xiaomi SAT
    StreamConfigModeBokeh = 0x8002,                 ///< Xiaomi Bokeh
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
    StreamConfigModeAlgoQcfaMfnr = 0x9004,          ///< Xiaomi App AlgoUp QCFA + MFNR mode
    StreamConfigModeAlgoManualSuperHD = 0x9007,     ///< Xiaomi App Manual + HD mode
    StreamConfigModeAlgoManual = 0x9008,            ///< Xiaomi App Manual Camera mode
    StreamConfigModeMiuiCameraJpeg = 0xEF00,        ///< Xiaomi MiuiCamera Jpeg encode mode
    StreamConfigModeMiuiCameraHeic = 0xEF01,        ///< Xiaomi MiuiCamera Heic encode mode
    StreamConfigModeSATBurst = 0xEF05,              ///< Xiaomi SAT in Burst Mode
    StreamConfigModeSATPerf = 0xEF06,               ///< Xiaomi SAT in Performance Mode
    StreamConfigModeTestMemcpy = 0xF101,            /// MiPostProcService_test yuv->sw->sw->yuv
    StreamConfigModeTestCpyJpg = 0xF102,            /// MiPostProcService_test yuv->sw->hw->jpg
    StreamConfigModeTestReprocess = 0xF103, /// MiPostProcService_test yuv->sw->reprocess->yuv
    StreamConfigModeTestBokeh = 0xF104,     /// MiPostProcService_test dual bokeh
    StreamConfigModeTestMulti = 0xF105,     /// MiPostProcService_test multi frame plugin
    StreamConfigModeTestR2Y = 0xF106,       /// MiPostProcService_test raw16->r2y->sw->yuv
    StreamConfigModeThirdPartyNormal = 0xFF0A,
    StreamConfigModeThirdPartyMFNR = 0xFF0B,
    StreamConfigModeThirdPartySuperNight = 0xFF0C,    ///< thirdparty super night
    StreamConfigModeThirdPartySmartEngine = 0xFF0E,   ///< thridparty Smart Engine
    StreamConfigModeThirdPartyFastMode = 0xFF0F,      ///< thridparty fast mode
    StreamConfigModeThirdPartyBokeh = 0xFF12,         ///< thridparty bokeh mode
    StreamConfigModeThirdPartyBokeh2x = 0xFF15,       ///< thridparty bokeh2x mode
    StreamConfigModeThirdPartyAutoExtension = 0xFF17, ///< thridparty auto mode
    StreamConfigModeThirdPartyHDR = 0xFF18,           ///< thridparty hdr mode
    StreamConfigModeThirdPartyBeauty = 0xFF19,        ///< thridparty beauty mode
    StreamConfigModeVendorEnd = 0xEFFF, ///< End value for vendor-defined stream configuration modes
} STREAMCONFIGMODE;

enum class MiaPostProcMode {
    OfflineSnapshot = 0,
    RealTimePreview = 1,
    RealTimeVideo = 2,
};

typedef struct FrameInfo
{
    uint32_t format; ///< Format of the image.
    uint32_t width;  ///< width of the image in pixels.
    uint32_t height; ///< height of the image in pixels.
    uint32_t planeStride;
    uint32_t sliceheight;
    uint32_t cameraId;
} MiaImageFormat;

enum class MiaFrameType { MI_FRAME_TYPE_INPUT = 0, MI_FRAME_TYPE_OUTPUT, MI_FRAME_TYPE_MAX };

enum class MiaResultCbType {
    MIA_META_CALLBACK = 0,
    MIA_ASD_CALLBACK = 1,
    MIA_ANCHOR_CALLBACK = 10,
    MIA_ABNORMAL_CALLBACK = 20,
};

class MiaPostProcSessionCb
{
public:
    virtual ~MiaPostProcSessionCb(){};
    virtual void onSessionCallback(MiaResultCbType type, uint32_t frameNum, int64_t timeStamp,
                                   void *msgData) = 0;
    // void  *priv; // pass by this if app need carry any private data in callback
};

enum OutSurfaceType {
    SURFACE_TYPE_SNAPSHOT_MAIN = 0,
    SURFACE_TYPE_SNAPSHOT_AUX,
    SURFACE_TYPE_SNAPSHOT_DEPTH,
    SURFACE_TYPE_VIEWFINDER,
    SURFACE_TYPE_COUNT
};

typedef struct _MiImage
{
    uint32_t bufferSize;
    int32_t planes;
    int fd[MiFormatsMaxPlanes];         ///< File descriptor, used by node to map the buffer to FD
    uint8_t *pAddr[MiFormatsMaxPlanes]; ///< Starting virtual address of the allocated buffer.
    buffer_handle_t phHandle;           ///< Native handle
    int32_t bufferType;

    void *reserved;
} MiaImageBuffer;

class MiaFrameCb
{
public:
    virtual ~MiaFrameCb(){};
    virtual void release(uint32_t frameNumber, int64_t timeStampUs, MiaFrameType type,
                         uint32_t streamIdx, buffer_handle_t handle) = 0;
    virtual int getBuffer(int64_t timeStampUs, OutSurfaceType type, buffer_handle_t *buffer) = 0;
};

struct MiaFrame
{
    MiaImageFormat format;
    MiaImageBuffer pBuffer;
    std::shared_ptr<DynamicMetadataWraper> metaWraper;
    int64_t timeStampUs;
    MiaFrameCb *callback;
    uint32_t frameNumber;
    uint32_t streamId;
};

struct MiaCreateParams
{
    MiaPostProcMode processMode; ///< proc mode
    uint32_t operationMode;
    uint32_t cameraMode;
    std::vector<MiaImageFormat> inputFormat;  ///< Input buffer params
    std::vector<MiaImageFormat> outputFormat; ///< Output buffer params
    void *pSessionParams;                     ///<  >= CAMERA_DEVICE_API_VERSION_3_5:
    MiaPostProcSessionCb *sessionCb;
    MiaFrameCb *frameCb;
    uint32_t logicalCameraId;
};

struct PostProcPreMetaProxy
{
    camera_metadata_t *mPreMeta;

    PostProcPreMetaProxy(camera_metadata_t *preMeta) { mPreMeta = preMeta; }
    ~PostProcPreMetaProxy()
    {
        if (NULL != mPreMeta) {
            free(mPreMeta);
            mPreMeta = NULL;
        }
    }

    camera_metadata_t *move()
    {
        camera_metadata_t *meta = mPreMeta;
        mPreMeta = nullptr;
        return meta;
    }
};

struct MiaPreProcParams
{
    std::map<uint32_t, MiaImageFormat> inputFormats;
    std::map<uint32_t, MiaImageFormat> outputFormats;
    std::shared_ptr<PostProcPreMetaProxy> metadata;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief This structure used to provide process request
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct MiaProcessParams
{
    uint32_t frameNum;
    uint32_t inputNum;
    MiaFrame *inputFrame;
    uint32_t outputNum;
    MiaFrame *outputFrame;
};

#define CLEAR(x) memset(&(x), 0, sizeof(x))

// the CDKResult is a extension set of android status_t
typedef int32_t CDKResult;
enum {
    MIAS_OK = 0,
    MIAS_UNKNOWN_ERROR = (-2147483647 - 1), // int32_t_MIN value
    MIAS_INVALID_PARAM,
    MIAS_NO_MEM,
    MIAS_MAP_FAILED,
    MIAS_UNMAP_FAILED,
    MIAS_OPEN_FAILED,
    MIAS_INVALID_CALL,
    MIAS_UNABLE_LOAD,
    MIAS_ABNORMAL
};

// private types
typedef void *SessionHandle;

enum QFResult { sessionNotFound, pipelineNotFound, messageSentFinish };

struct QuickFinishMessageInfo
{
    std::string fileName;
    bool needImageResult;
};

} // namespace mialgo2

#endif // __MI_POSTPROC_TYPE__
