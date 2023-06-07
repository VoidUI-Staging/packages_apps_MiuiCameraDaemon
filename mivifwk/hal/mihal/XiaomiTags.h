#ifndef __XIAOMI_TAGS_H__
#define __XIAOMI_TAGS_H__

static const char *MI_AI_ASD_SNAPSHOT_REQ_INFO = "xiaomi.ai.asd.SnapshotReqInfo";
static const char *MI_AI_DEBLUR_CAMERA_FRAME_RATIO = "xiaomi.ai.deblur.cameraFrameRatio";
static const char *MI_AI_MISD_ELLTRIPOD = "xiaomi.ai.misd.ELLTripod";
static const char *MI_AI_MISD_NONSEMANTICSECENE = "xiaomi.ai.misd.NonSemanticScene";
static const char *MI_AI_MISD_STATESCENE = "xiaomi.ai.misd.StateScene";
static const char *MI_AI_MISD_MOTIONCAPTURETYPE = "xiaomi.ai.misd.motionCaptureType";
static const char *MI_AI_MISD_SUPERNIGHTCAPTUREEXPTIME = "xiaomi.ai.misd.SuperNightCaptureExpTime";
static const char *MI_ANCHOR_FRAME_MASK = "xiaomi.capabilities.quick_view_mask"; // int32[1]
static const char *MI_ANCHOR_FRAME_TS = "xiaomi.mfnr.anchorTimeStamp";
static const char *MI_ANCHOR_FRAME_NUM = "com.xiaomi.mivi2.anchorFrameNum";
static const char *MI_ASD_ENABLE = "xiaomi.asd.enabled";
static const char *MI_AI_ASD_ASD_EXIF_INFO = "xiaomi.ai.asd.asdExifInfo";
static const char *MI_AI_ASD_SCENE_APPLIED = "xiaomi.ai.asd.sceneApplied";

static const char *MI_MFNR_BOKEH_SUPPORTED = "xiaomi.capabilities.mfnr_bokeh_supported";
static const char *MI_BOKEH_HDR_ENABLE = "xiaomi.bokeh.hdrEnabled";
static const char *MI_BOKEH_SUPERNIGHT_ENABLE = "xiaomi.bokeh.superNightEnabled";
static const char *MI_BURST_CAPTURE_HINT = "xiaomi.burst.captureHint";
static const char *MI_BURST_TOTAL_NUM = "xiaomi.burst.totalReqNum";
static const char *MI_BURST_MIALGO_TOTAL_NUM = "xiaomi.burst.mialgoTotalReqNum";
static const char *MI_BURST_CURRENT_INDEX = "xiaomi.burst.curReqIndex";
static const char *MI_BURST_DOWN_CAPTURE = "com.xiaomi.mivi2.supportDownCapture";

static const char *MI_BEAUTY_SKINSMOOTH = "xiaomi.beauty.skinSmoothRatio";
static const char *MI_BEAUTY_SLIMFACE = "xiaomi.beauty.slimFaceRatio";
static const char *MI_BEAUTY_MAKEUPGENDER = "xiaomi.beauty.makeupGender";
static const char *MI_BEAUTY_REMOVENEVUS = "xiaomi.beauty.removeNevus";
static const char *MI_BOKEH_ENABLE = "xiaomi.bokeh.enabled";
static const char *MI_BOKEH_FNUMBER_APPLIED = "xiaomi.bokeh.fNumberApplied";
static const char *MI_HDR_BOKEH_CROPSIZE = "xiaomi.bokeh.hdrCropsize";
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

// 1x  2x 人像 cameraid 一样，需要告诉底下当前是哪个role id
static const char *MI_CAMERA_BOKEH_ROLE = "com.xiaomi.sessionparams.bokehRole";

// 人像模式-大师镜头
static const char *MI_CAMERA_BOKEH_MDMODE = "com.xiaomi.sessionparams.bokehMDmode";

// 屏幕的物理尺寸，L18这种折叠屏有时需要根据此判断当前是展开还是折叠
static const char *MI_CAMERA_PREVIEW_FULLSIZE = "com.xiaomi.sessionparams.previewFullSize";

// 支持多摄架构的机型:
// 因为半身人像(wide和tele)和全身人像(uw和wide)共用同一个logical camera id,
// 所以半身人像和全身人像需要各使用一个vendor tag来加以区分。
//
// 不支持多摄架构的机型:
// 因为半身人像(wide和tele)和全身人像(uw和wide)使用不同的logical camera id,
// 所以半身人像和全身人像可以共用同一个vendor tag.
static const char *MI_CAMERA_BOKEHINFO_MASTER_CAMERAID = "xiaomi.camera.bokehinfo.masterCameraId";
static const char *MI_CAMERA_BOKEHINFO_SLAVE_CAMERAID = "xiaomi.camera.bokehinfo.slaveCameraId";

/**
 * 支持多摄架构机型的半身人像主摄 yuv size, 或者不支持多摄架构机型的人像主摄 yuv size
 */
static const char *MI_CAMERA_BOKEHINFO_MASTER_OPTIMALSIZE =
    "xiaomi.camera.bokehinfo.masterOptimalSize";

/**
 * 支持多摄架构机型的半身人像辅摄 yuv size, 或者不支持多摄架构机型的人像辅摄 yuv size
 */
static const char *MI_CAMERA_BOKEHINFO_SLAVE_OPTIMALSIZE =
    "xiaomi.camera.bokehinfo.slaveOptimalSize";

static const char *MI_CAMERA_BOKEHINFO_OPTIMAL_PICTURE_SIZE =
    "xiaomi.camera.bokehinfo.optimalPictureSize";

static const char *MI_CAMERA_BOKEHINFO_MASTER1X_CAMERAID =
    "xiaomi.camera.bokehinfo.masterCameraId1X";
static const char *MI_CAMERA_BOKEHINFO_SLAVE1X_CAMERAID = "xiaomi.camera.bokehinfo.slaveCameraId1X";

/**
 * 支持多摄架构机型的全身人像主摄 yuv size
 */
static const char *MI_CAMERA_BOKEHINFO_MASTER_1X_OPTIMALSIZE =
    "xiaomi.camera.bokehinfo.masterOptimalSize1X";

/**
 * 支持多摄架构机型的全身人像辅摄 yuv size
 */
static const char *MI_CAMERA_BOKEHINFO_SLAVE_1X_OPTIMALSIZE =
    "xiaomi.camera.bokehinfo.slaveOptimalSize1X";

static const char *MI_CAMERA_BOKEHINFO_OPTIMAL_PICTURE_1X_SIZE =
    "xiaomi.camera.bokehinfo.optimalPictureSize1X";

/**
 * 支持多摄架构机型的半身人像主摄 raw size, 或者不支持多摄架构机型的人像主摄 raw size
 */
static const char *MI_CAMERA_BOKEHINFO_MASTER_RAW_OPTIMALSIZE =
    "xiaomi.camera.bokehinfo.masterOptimalRawSize";

/**
 * 支持多摄架构机型的半身人像辅摄 raw size, 或者不支持多摄架构机型的人像辅摄 raw size
 */
static const char *MI_CAMERA_BOKEHINFO_SLAVE_RAW_OPTIMALSIZE =
    "xiaomi.camera.bokehinfo.slaveOptimalRawSize";

/**
 * 支持多摄架构机型的全身人像主摄 raw size
 */

static const char *MI_CAMERA_BOKEHINFO_MASTER_1X_RAW_OPTIMALSIZE =
    "xiaomi.camera.bokehinfo.masterOptimalRawSize1X";

/**
 * 支持多摄架构机型的全身人像辅摄 raw size
 */
static const char *MI_CAMERA_BOKEHINFO_SLAVE_1X_RAW_OPTIMALSIZE =
    "xiaomi.camera.bokehinfo.slaveOptimalRawSize1X";
//<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

static const char *MI_CAMERAID_ROLE_CAMERAID = "com.xiaomi.cameraid.role.cameraId";
static const char *MI_CAMERA_SUPPORTED_FEATURES_BOKEH_VENDORID =
    "com.xiaomi.camera.supportedfeatures.bokehVendorID";

static const char *MI_CAPTURE_FUSION_IS_ON = "xiaomi.capturefusion.isFusionOn";
static const char *MI_CAPTURE_FUSION_SUPPORT_CPFUSION = "xiaomi.capturefusion.supportCPFusion";
static const char *MI_CAPTURE_FUSION_IS_PIPELINE_READY = "xiaomi.capturefusion.isPipelineReady";
static const char *MI_COLOR_RETENTION_VALUE = "xiaomi.colorRetention.value";
static const char *MI_COLOR_RETENTION_FRONT_VALUE = "xiaomi.colorRetention.frontValue";

static const char *MI_DEPURPLE_ENABLE = "xiaomi.depurple.enabled";
static const char *MI_DISTORTION_LEVEL = "xiaomi.distortion.distortionLevelApplied";
static const char *MI_DISTORTION_ULTRAWIDE_LEVEL = "xiaomi.distortion.ultraWideDistortionLevel";
static const char *MI_DISTORTION_ULTRAWIDE_ENABLE = "xiaomi.distortion.ultraWideDistortionEnable";
static const char *MI_DEVICE_ORIENTATION = "xiaomi.device.orientation";

static const char *MI_FLIP_ENABLE = "xiaomi.flip.enabled";
static const char *MI_FAKESAT_ENABLED = "xiaomi.FakeSat.enabled";

static const char *MI_HDR_CHECKER_ADRC = "xiaomi.hdr.hdrChecker.adrc";
static const char *MI_HDR_CHECKER_ENABLE = "xiaomi.hdr.hdrChecker.enabled";
static const char *MI_HDR_CHECKER_RESULT = "xiaomi.hdr.hdrChecker";
static const char *MI_HDR_RAW_CHECKER_RESULT = "xiaomi.hdr.raw.hdrChecker";
static const char *MI_HDR_CHECKER_SCENE_TYPE = "xiaomi.hdr.hdrChecker.sceneType";
static const char *MI_HDR_CHECKER_STATUS = "xiaomi.hdr.hdrChecker.status";
static const char *MI_HDR_MOTION_DETECTED = "xiaomi.ai.misd.hdrmotionDetected";
static const char *MI_HDR_HIGH_THERMAL_DETECTED = "xiaomi.ai.asd.isHDRHighThermal";
static const char *MI_HDR_DETECTED = "xiaomi.hdr.hdrDetected";
static const char *MI_HDR_ZSL = "xiaomi.ai.asd.isZSLHDR";
static const char *MI_HDR_ENABLE = "xiaomi.hdr.enabled";
static const char *MI_RAWHDR_ENABLE = "xiaomi.hdr.raw.enabled";
static const char *MI_HDR_MODE = "xiaomi.hdr.hdrMode";
static const char *MI_HDR_REQ_SETTING = "xiaomi.hdr.hdrFrameReq";
static const char *MI_HDR_SR_ENABLE = "xiaomi.hdr.sr.enabled";
static const char *MI_HDR_SR_HDR_DETECTED = "xiaomi.hdr.srhdrDetected";
static const char *MI_HDR_SR_REQUEST_NUM = "xiaomi.hdr.srhdrRequestNumber";

static const char *MI_HEICSNAPSHOT_ENABLED = "xiaomi.HeicSnapshot.enabled";

// HDR-DISPLAY ADD: start
static const char *MI_SNAPSHOT_ADRC = "xiaomi.snapshot.adrc";
static const char *MI_SNAPSHOT_SHOTTYPE = "xiaomi.snapshot.shotType";
// HDR-DISPLAY ADD: end

static const char *MI_SR_RATIO_THRESHOLD = "xiaomi.superResolution.zoomRatioThresholdToStartSr";

static const char *MI_MIVI_FG_FRAME = "com.xiaomi.mivi2.fgFrameNumber";
static const char *MI_MIVI_LINEARGAIN = "com.xiaomi.mivi2.linearGain";
static const char *MI_MIVI_SHORTGAIN = "com.xiaomi.mivi2.shortGain";
static const char *MI_MIVI_LUXINDEX = "com.xiaomi.mivi2.luxIndex";
static const char *MI_MIVI_ADRCGAIN = "com.xiaomi.mivi2.adrcGain";
static const char *MI_MIVI_DARKRATIO = "com.xiaomi.mivi2.darkRatio";
static const char *MI_FLASH_MODE = "xiaomi.flash.mode";
static const char *MI_THERMAL_LEVEL = "xiaomi.thermal.thermalLevel";
static const char *MI_MIVI_POST_ERROR = "com.xiaomi.mivi2.postError";
static const char *MI_MIVI_POST_SESSION_SIG = "com.xiaomi.mivi2.postSessionSig";
static const char *MI_MIVI_SESSION_ID = "com.xiaomi.mivi2.sessionId";
static const char *MI_MIVI_SWJPEG_ENABLE = "com.xiaomi.mivi2.swjpeg.enable";
static const char *MI_MIVI_ALGO_EXIF = "com.xiaomi.mivi2.exif";
static const char *MI_MIVI_SNAPSHOT_OUTPUTSIZE = "com.xiaomi.mivi2.outputSize";
static const char *MI_MIVI_RAW_NONZSL_ENABLE = "com.xiaomi.mivi2.rawnonzsl.enable";
static const char *MI_MIVI_RAW_ZSL_ENABLE = "com.xiaomi.mivi2.rawzsl.enable";
static const char *MI_MIVI_RAW_ZSL_FRAMES = "com.xiaomi.mivi2.rawzsl.selectedFrames";
static const char *MI_MIVI_MAX_JPEGSIZE = "com.xiaomi.mivi2.maxJpegSize";
static const char *MI_MIVI_BGSERVICE_CLIENTID = "com.xiaomi.sessionparams.processId";
static const char *MI_MIVI_MIUI3RD = "com.xiaomi.mivi2.miui3rd";
static const char *MI_MIVI_FULL_SCENE_RAW_CB_SUPPORTED =
    "com.xiaomi.sessionparams.miviFullSceneRawCb";

static const char *MI_MIVI_SUPERNIGHT_MODE = "xiaomi.mivi.supernight.mode";
static const char *MI_MIVI_NIGHT_MOTION_MODE = "xiaomi.nightmotioncapture.mode";
static const char *MI_MIVI_SUPERNIGHT_SUPPORTMASK = "xiaomi.capabilities.MIVISuperNightSupportMask";
static const char *MI_MIVI_FWK_VERSION = "xiaomi.capabilities.MiviVersion";
static const char *MI_MIVI_UI_RELATED_METAS = "xiaomi.capabilities.ui_related_metas";

static const char *MI_MFNR_ENABLE = "xiaomi.mfnr.enabled";
static const char *MI_MFNR_FRAMENUM = "com.xiaomi.customization.mfnr.frameNumber";
static const char *MI_MFNR_LLS_NEEDED = "com.qti.stats_control.is_lls_needed";
static const char *MI_MULTIFRAME_INPUTNUM = "xiaomi.multiframe.inputNum";

static const char *MI_PARAMS_CAPTURE_RATION = "com.xiaomi.params.captureRatio";
static const char *MI_PORTRAIT_LIGHTING = "xiaomi.portrait.lighting";

static const char *MI_QUICKSHOT_ISQUICKSNAPSHOT = "xiaomi.quicksnapshot.isQuickSnapshot";
static const char *MI_QUICKSHOT_ISHQQUICKSHOT = "xiaomi.highqualityquickshot.isHQQuickshot";
static const char *MI_QUICKSHOT_SUPPORT_MASK =
    "com.xiaomi.camera.supportedfeatures.HighQualityQuickShotSupportMask";
static const char *MI_QUICKSHOT_DELAYTIME =
    "com.xiaomi.camera.supportedfeatures.QuickShotDelayTimeMask";
static const char *MI_QUICKSHOT_MFNR_RATIOTHRESHOLD =
    "xiaomi.superResolution.zoomRatioThresholdQuickshotByMfnr";

static const char *MI_MACROMODE_ENABLED = "xiaomi.MacroMode.enabled";
static const char *MI_REMOSAIC_DETECTED = "xiaomi.remosaic.detected";
static const char *MI_REMOSAIC_ENABLED = "xiaomi.remosaic.enabled";

static const char *MI_SCALER_AVAILABLE_SUPERRESOLUTION_STREAM_CONFIGURATIONS =
    "xiaomi.scaler.availableSuperResolutionStreamConfigurations";

static const char *MI_SAT_MASTER_ID = "xiaomi.smoothTransition.physicalCameraId";
static const char *MI_SAT_MASTER_ROLE_ID = "xiaomi.smoothTransition.masterCameraRole";

static const char *MI_SESSION_PARA_CLIENT_NAME = "com.xiaomi.sessionparams.clientName";

static const char *MI_SCALER_AVAILABLE_STREAM_CONFIGURATIONS =
    "xiaomi.scaler.availableStreamConfigurations";
static const char *MI_SCALER_AVAILABLE_LIMIT_STREAM_CONFIGURATIONS =
    "xiaomi.scaler.availableLimitStreamConfigurations";

static const char *MI_SUPPORT_FEATURE_FOV_CROP = "com.xiaomi.camera.supportedfeatures.fovCrop";
static const char *MI_SENSOR_INFO_ACTIVE_ARRAY_SIZE = "xiaomi.sensor.info.activeArraySize";

static const char *MI_SNAPSHOT_IMAGE_NAME = "xiaomi.snapshot.imageName";

static const char *MI_SUPER_RESOLUTION_ENALBE = "xiaomi.superResolution.enabled";
static const char *MI_SRHDR_ZSL_ENALBE = "xiaomi.superResolution.isHdsrZSLSupported";
static const char *MI_SRHDR_EV_ARRAY = "xiaomi.superResolution.hdsrEvValueArray";

static const char *MI_SUPERNIGHT_CHECKER = "xiaomi.supernight.checker";
static const char *MI_SUPERNIGHT_ENABLED = "xiaomi.supernight.enabled";
static const char *MI_SUPERNIGHT_MFNR_ENABLED = "xiaomi.supernight.mfnr.enabled";
static const char *MI_PROTRAIT_REPAIR_ENABLED = "xiaomi.protraitrepair.enabled";

static const char *MI_SNAPSHOT_ISPURESNAPSHOT = "com.xiaomi.snapshot.isPureSnapshot";

static const char *MI_SUPPORTED_ALGO_HDR =
    "com.xiaomi.camera.supportedfeatures.supportedAlgoEngineHdr";

static const char *MI_SUPPORT_ANCHOR_FRAME = "xiaomi.capabilities.quick_view_support"; // byte[1]

static const char *MI_VIDEO_BOKEH_PARAM_BACK = "xiaomi.videoBokehParam.back";
static const char *MI_VIDEO_BOKEH_PARAM_FRONT = "xiaomi.videoBokehParam.front";

static const char *MI_OPERATION_MODE = "xiaomi.operationmode.value";

static const char *MI_MULTIFRAME_KEYFRAMEID = "com.xiaomi.multiframe.keyframeId";

static const char *MI_VIDEO_CONTROL = "xiaomi.video.recordControl";

static const char *MI_FALLBACK_DISABLE = "xiaomi.smoothTransition.disablefallback";

// Madrid bokeh tag
static const char *MI_BOKEH_MADRID_MODE_SUPPORTED = "xiaomi.capabilities.bokehMDmodeSupported";
static const char *MI_BOKEH_MADRID_MODE = "xiaomi.bokeh.MDMode";
static const char *MI_BOKEH_MADRID_EV_LIST = "xiaomi.camera.bokehinfo.MDEvList";

static const char *QCOM_AE_BRACKET_MODE = "org.codeaurora.qcamera3.ae_bracket.mode";
static const char *QCOM_RAW_CB_INFO_IDEALRAW = "com.qti.chi.rawcbinfo.IdealRaw";
static const char *QCOM_IS_QCFA_SENSOR =
    "org.codeaurora.qcamera3.quadra_cfa.is_qcfa_sensor"; // byte[1]
static const char *QCOM_QCFA_DIMENSION =
    "org.codeaurora.qcamera3.quadra_cfa.qcfa_dimension"; // int32[2]

static const char *QCOM_MFHDR_ENABLE = "org.codeaurora.qcamera3.sessionParameters.EnableMFHDR";

static const char *QCOM_3A_DEBUG_DATA = "org.quic.camera.debugdata.DebugDataAll";

static const char *QCOM_JPEG_DEBUG_DATA = "org.quic.camera.jpegdebugdata.size";

static const char *QCOM_VIDEOHDR10_MODE = "org.quic.camera2.streamconfigs.HDRVideoMode";

static const char *QCOM_VIDEOHDR10_SESSIONPARAM =
    "org.codeaurora.qcamera3.sessionParameters.HDRVideoMode";
//>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

// sdk tag

static const char *SDK_HDR_MODE = "com.xiaomi.algo.hdrMode";
static const char *SDK_BEAUTY_SKINSMOOTH = "com.xiaomi.algo.beautySkinSmoothRatio";
static const char *SDK_BEAUTY_SLIMFACE = "com.xiaomi.algo.beautySlimFaceRatio";
static const char *SDK_BOKEH_ENABLE = "com.xiaomi.algo.mibokehEnable";
static const char *SDK_MFNR_ENABLE = "com.xiaomi.algo.mfnrEnable";
static const char *SDK_NIGHT_MODE_ENABLE = "com.xiaomi.algo.nightModeEnable";
static const char *SDK_DEPURLE_ENABLE = "com.xiaomi.algo.depurpleEnable";
static const char *SDK_ENABLE = "com.xiaomi.capabilities.algoSDKEnabled";
static const char *SDK_CAMERAX_ENABLE = "com.xiaomi.capabilities.algoCameraXEnabled";
static const char *SDK_VIDEO_BOKEH_ENABLE_FRONT = "com.xiaomi.algo.videoBokehFrontEnabled";
static const char *SDK_VIDEO_BOKEH_PARAM_FRONT = "com.xiaomi.algo.videoBokehFrontParam";
static const char *SDK_VIDEO_BOKEH_ENABLE_REAR = "com.xiaomi.algo.videoBokehRearEnabled";
static const char *SDK_VIDEO_BOKEH_PARAM_REAR = "com.xiaomi.algo.videoBokehRearParam";
static const char *SDK_VIDEOHDR10_MODE = "com.xiaomi.algo.hdrVideoMode";
static const char *SDK_VIDEOCONTROL = "com.xiaomi.algo.videoControl";
static const char *SDK_DEVICE_ORIENTATION = "com.xiaomi.algo.deviceOrientation";
static const char *SDK_BOKEH_FNUMBER_APPLIED = "com.xiaomi.algo.bokeh.fNumberApplied";
static const char *SDK_SESSION_PARAMS_OPERATION = "com.xiaomi.sessionparams.operation";
static const char *SDK_SESSION_PARAMS_ISCAMERAX = "com.xiaomi.sessionparams.cameraxConnection";
static const char *SDK_SESSION_PARAMS_CLIENT = "com.xiaomi.sessionparams.clientName";
static const char *SDK_SESSION_PARAMS_BOKEHROLE = "com.xiaomi.sessionparams.bokehRole";
static const char *SDK_SESSION_PARAMS_YUVSNAPSHOT =
    "com.xiaomi.sessionparams.thirdPartyYUVSnapshot";

// use it in bokeh mode
// 0 -> bokeh mode off; 1 -> still capture bokeh; 2 -> continuous bokeh mode
static const char *SDK_BOKEH_MODE = "android.control.extendedSceneMode";

#endif // __XIAOMI_TAGS_H__
