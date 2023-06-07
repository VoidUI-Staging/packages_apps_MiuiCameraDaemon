#include <utils/Log.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "algoAdapter.h"
#include "device.h"
#include "metaGetter.h"
#include "sr_api.h"
#include "sr_common.h"

#define ALIGN_TO_SIZE(size, align) ((size + align - 1) & ~(align - 1))
#undef LOG_TAG
#define LOG_TAG      "TETRAS3ADPTER"

#define ADVANCEMOONMODEON (35)

using namespace mialgo2;
using namespace std;

#define recordSDKRet(record, format, function)\
do{\
    int ret = function;\
    MLOGI(Mia2LogGroupPlugin, format, ret);\
    record << 1;\
    if(ret)record |= 0x1;\
}while(0)

#define recordSDKSetRet(record, format, name, function)\
do{\
    int ret = function;\
    MLOGI(Mia2LogGroupPlugin, format, name, ret);\
    record << 1;\
    if(ret)record |= 0x1;\
}while(0)


#define init_extention() {\
{SR_EXTENTION_NM_ISP_EV0, ARG_IDX_BLACK_LEVEL,	ARG_COUNT_BLACK_LEVEL * sizeof(int),	SR_EXT_ARG_FOLAT_PRT,		getBlackLevel(in, inputCount),		true, nullptr, "black level"},\
{SR_EXTENTION_NM_ISP_EV0, ARG_IDX_ISO,		ARG_COUNT_ISO * sizeof(float), 		SR_EXT_ARG_FOLAT_PRT,		&iso[0],				true, nullptr, "iso"},\
{SR_EXTENTION_NM_ISP_EV0, ARG_IDX_AWB_GAINS,	ARG_COUNT_AWB_GAINS * sizeof(float),	SR_EXT_ARG_FOLAT_PRT,		&awbInfo,				true, nullptr, "awb"},\
{SR_EXTENTION_NM_ISP_EV0, ARG_IDX_CCM,		ARG_COUNT_CCM * sizeof(float),		SR_EXT_ARG_FOLAT_PRT,		getColorCorrection(in, inputCount), 	true, nullptr, "cc"},\
{SR_EXTENTION_NM_ISP_EV0, ARG_IDX_R_LSCM,	ARG_COUNT_R_LSCM * sizeof(float),	SR_EXT_ARG_FOLAT_PRT,		lensShadingMap,				true, nullptr, "lensShadingMap_R"},\
{SR_EXTENTION_NM_ISP_EV0, ARG_IDX_Gr_LSCM,	ARG_COUNT_Gr_LSCM * sizeof(float),	SR_EXT_ARG_FOLAT_PRT,		lensShadingMap + 1,			true, nullptr, "lensShadingMap_GR"},\
{SR_EXTENTION_NM_ISP_EV0, ARG_IDX_Gb_LSCM,	ARG_COUNT_Gb_LSCM * sizeof(float),	SR_EXT_ARG_FOLAT_PRT,		lensShadingMap + 2,			true, nullptr, "lensShadingMap_GB"},\
{SR_EXTENTION_NM_ISP_EV0, ARG_IDX_B_LSCM,	ARG_COUNT_B_LSCM * sizeof(float),	SR_EXT_ARG_FOLAT_PRT,		lensShadingMap + 3,			true, nullptr, "lensShadingMap_B"},\
{SR_EXTENTION_NM_ISP_EV0, ARG_IDX_R_CURVE,	ARG_COUNT_R_CURVE * sizeof(float),	SR_EXT_ARG_FOLAT_PRT,		&f_gamma,				true, nullptr, "R_CURVE"},\
{SR_EXTENTION_NM_ISP_EV0, ARG_IDX_G_CURVE,	ARG_COUNT_G_CURVE * sizeof(float),	SR_EXT_ARG_FOLAT_PRT,		&f_gamma,				true, nullptr, "G_CURVE"},\
{SR_EXTENTION_NM_ISP_EV0, ARG_IDX_B_CURVE,	ARG_COUNT_B_CURVE * sizeof(float),	SR_EXT_ARG_FOLAT_PRT,		&f_gamma,				true, nullptr, "B_CURVE"},\
{SR_EXTENTION_FASE_PARAM, 0,			sizeof(sr_face_info_t),			SR_EXT_ARG_SR_FACE_RECTS_PTR,	&faceInfo,				true, nullptr, "face"},\
}

#define define_extention(extention)\
const struct {\
        const sr_extention_t    func_name;\
        const int                  arg_idx;\
        const int                  arg_bytes;\
        const sr_ext_arg_type_t arg_type;\
        const void* arg_ptr;\
        const bool  is_tmp_memory;\
        void** arg_pp;\
        const char * const name;\
} extention[] = init_extention()

#define set_extention_ptr(handle, extention, record)\
    do{\
    for(unsigned int i = 0; i < sizeof(extention) / sizeof(extention[0]); i++)\
        recordSDKSetRet(record, "set extention ptr(%s): %d", extention[i].name,\
        sr_set_extention_ptr(handle,\
                             extention[i].func_name,\
                             extention[i].arg_idx,\
                             extention[i].arg_bytes,\
                             extention[i].arg_type,\
                             extention[i].arg_ptr,\
                             extention[i].is_tmp_memory,\
                             extention[i].arg_pp\
                             ));\
    }while(0)


static void setUVDenoiseParam(int nGain, sr_uv_denoise_param_t &param,
                              sr_specific_param_t &specParam)
{
    nGain = (nGain > 7000) ? 7000 : (nGain < 10 ? 10 : nGain);
    // the parameters of UV denoise pcannot be set too small, such as 0.1
    if (nGain <= 200) {
        param.levelStrength[0] = 0.5;
        param.levelStrength[1] = 0.5;
        param.levelStrength[2] = 0.2;
        param.levelStrength[3] = 0.2;
        param.levelStrength[4] = 0.0;
        param.motionLevelStrength[0] = 3;
        param.motionLevelStrength[1] = 1;
        param.motionLevelStrength[2] = 1.2;
        param.motionLevelStrength[3] = 0.5;
        param.motionLevelStrength[4] = 0.0;
        specParam.fSharpFilterRadius = 1.0;
        specParam.fSharpGradinetThresh = 0;
        specParam.fModelRefMaskRadius = 3;
        specParam.fModelRefMaskThresh = 7;
        specParam.fModelRefMaskRatio = 8;
        specParam.fModelFinetuneRefMask = 1.0;
    } else if (nGain <= 500) {
        param.levelStrength[0] = 1;
        param.levelStrength[1] = 1;
        param.levelStrength[2] = 0.5;
        param.levelStrength[3] = 0.5;
        param.levelStrength[4] = 0.0;
        param.motionLevelStrength[0] = 3;
        param.motionLevelStrength[1] = 2;
        param.motionLevelStrength[2] = 2;
        param.motionLevelStrength[3] = 0.5;
        param.motionLevelStrength[4] = 0.0;
        specParam.fSharpFilterRadius = 1.0;
        specParam.fSharpGradinetThresh = 0;
        specParam.fModelRefMaskRadius = 3;
        specParam.fModelRefMaskThresh = 7;
        specParam.fModelRefMaskRatio = 8;
        specParam.fModelFinetuneRefMask = 1.0;
    } else if (nGain <= 1000) {
        param.levelStrength[0] = 2.5;
        param.levelStrength[1] = 1.5;
        param.levelStrength[2] = 0.5;
        param.levelStrength[3] = 0.5;
        param.levelStrength[4] = 0.0;
        param.motionLevelStrength[0] = 6;
        param.motionLevelStrength[1] = 4;
        param.motionLevelStrength[2] = 2;
        param.motionLevelStrength[3] = 0.5;
        param.motionLevelStrength[4] = 0.0;
        specParam.fSharpFilterRadius = 1.0;
        specParam.fSharpGradinetThresh = 0;
        specParam.fModelRefMaskRadius = 3;
        specParam.fModelRefMaskThresh = 7;
        specParam.fModelRefMaskRatio = 8;
        specParam.fModelFinetuneRefMask = 1.0;
    } else if (nGain <= 1800) {
        param.levelStrength[0] = 2.5;
        param.levelStrength[1] = 1.5;
        param.levelStrength[2] = 0.8;
        param.levelStrength[3] = 0.5;
        param.levelStrength[4] = 0.0;
        param.motionLevelStrength[0] = 6;
        param.motionLevelStrength[1] = 5;
        param.motionLevelStrength[2] = 2;
        param.motionLevelStrength[3] = 1;
        param.motionLevelStrength[4] = 0.0;
        specParam.fSharpFilterRadius = 1.0;
        specParam.fSharpGradinetThresh = 0;
        specParam.fModelRefMaskRadius = 3;
        specParam.fModelRefMaskThresh = 7;
        specParam.fModelRefMaskRatio = 8;
        specParam.fModelFinetuneRefMask = 1.0;
    } else if (nGain <= 3000) {
        param.levelStrength[0] = 3.2;
        param.levelStrength[1] = 1.8;
        param.levelStrength[2] = 0.6;
        param.levelStrength[3] = 0.5;
        param.levelStrength[4] = 0.0;
        param.motionLevelStrength[0] = 12;
        param.motionLevelStrength[1] = 8;
        param.motionLevelStrength[2] = 3;
        param.motionLevelStrength[3] = 1;
        param.motionLevelStrength[4] = 0.0;
        specParam.fSharpFilterRadius = 1.0;
        specParam.fSharpGradinetThresh = 0;
        specParam.fModelRefMaskRadius = 3;
        specParam.fModelRefMaskThresh = 7;
        specParam.fModelRefMaskRatio = 8;
        specParam.fModelFinetuneRefMask = 1.0;
    } else if (nGain <= 5000) {
        param.levelStrength[0] = 6;
        param.levelStrength[1] = 2.5;
        param.levelStrength[2] = 1;
        param.levelStrength[3] = 0.5;
        param.levelStrength[4] = 0.0;
        param.motionLevelStrength[0] = 12;
        param.motionLevelStrength[1] = 10;
        param.motionLevelStrength[2] = 4;
        param.motionLevelStrength[3] = 1.5;
        param.motionLevelStrength[4] = 0.0;
        specParam.fSharpFilterRadius = 1.0;
        specParam.fSharpGradinetThresh = 0;
        specParam.fModelRefMaskRadius = 3;
        specParam.fModelRefMaskThresh = 7;
        specParam.fModelRefMaskRatio = 8;
        specParam.fModelFinetuneRefMask = 1.0;
    } else if (nGain <= 7000) {
        param.levelStrength[0] = 6;
        param.levelStrength[1] = 4;
        param.levelStrength[2] = 1;
        param.levelStrength[3] = 0.5;
        param.levelStrength[4] = 0.0;
        param.motionLevelStrength[0] = 15;
        param.motionLevelStrength[1] = 10;
        param.motionLevelStrength[2] = 6;
        param.motionLevelStrength[3] = 2;
        param.motionLevelStrength[4] = 0.0;
        specParam.fSharpFilterRadius = 1.0;
        specParam.fSharpGradinetThresh = 0;
        specParam.fModelRefMaskRadius = 3;
        specParam.fModelRefMaskThresh = 7;
        specParam.fModelRefMaskRatio = 8;
        specParam.fModelFinetuneRefMask = 1.0;
    }
}

static void onLibEvent(sr_event_t *event)
{
    if (event->type == SR_EVENT_IMG_REF_INDEX) {
        MLOGI(Mia2LogGroupPlugin, "SR_EVENT_IMG_REF_INDEX %d", event->imgRefIdx);
    } else if (event->type == SR_EVENT_IMG_CROP_ROI) {
        MLOGI(Mia2LogGroupPlugin, "SR_EVENT_IMG_CROP_ROI %d", event->imgCropRoi.width);
    } else {
        MLOGI(Mia2LogGroupPlugin, "unknown event type %d", event->type);
    }
}

static CameraRoleId getRole(const SessionInfo *sessionInfo, uint32_t fwkCameraId)
{
    if (sessionInfo->getOperationMode() == 0x8003)
        if (sessionInfo->getFrameworkCameraId() == 0x5)
            return RoleIdRearTele;
    return getRoleSAT(fwkCameraId);
}

static unsigned int getCameraInfoIndex(const ImageParams *in, unsigned int inputCount,
                                       const SessionInfo *sessionInfo,
                                       function<void(uint32_t)> onGetFwkCameraId)
{
    uint32_t fwkCameraId = getFwkCameraId(in, inputCount);
    onGetFwkCameraId(fwkCameraId);
#if MOON_MODE
    if (ADVANCEMOONMODEON == getMode(in, inputCount).getDataWithDefault<unsigned int>(0))
        return TETRAS3_CAMERA_INFO_LIST_MOON_INDEX;
#endif
#if ISZ_MODE
    if (2 == getISZState(in, inputCount).getDataWithDefault<unsigned int>(0)) {
        MLOGI(Mia2LogGroupPlugin, "InSensorZoom SR mode");
        return TETRAS3_CAMERA_INFO_LIST_WIDEISZ_INDEX;
    }
#endif
    return static_cast<unsigned int>(getRole(sessionInfo, fwkCameraId));
}

static const sr_camera_info_t *getCameraInfoByIndex(unsigned int index)
{
    static const sr_camera_info_t cameraInfo[] = TETRAS3_CAMERA_INFO_LIST;
    return cameraInfo + (index % (sizeof(cameraInfo) / sizeof(cameraInfo[0])));
}

static const sr_camera_info_t *getCameraInfo(const ImageParams *in, unsigned int inputCount,
                                             const SessionInfo *sessionInfo,
                                             function<void(uint32_t)> onGetFwkCameraId)
{
    return getCameraInfoByIndex(getCameraInfoIndex(in, inputCount, sessionInfo, onGetFwkCameraId));
}

static void dumpParams(const FLOAT lensShadingMap[LSC_TOTAL_CHANNELS][LSC_MESH_ROLLOFF_SIZE],
                       const float f_gamma[65], const sr_face_info_t faceInfo)
{
    static int32_t isDump = property_get_int32("persist.vendor.camera.srdump.para", 0);
    if (isDump == 0)
        return;

    char filename[100], timeBuf[50];
    time_t currentTime;
    struct tm *timeInfo;

    time(&currentTime);
    timeInfo = localtime(&currentTime);
    strftime(timeBuf, sizeof(timeBuf), "IMG_%Y%m%d%H%M%S", timeInfo);
    snprintf(filename, sizeof(filename), "%s%s_sr_para.txt", PluginUtils::getDumpPath(), timeBuf);

    FILE *pFile = fopen(filename, "w");
    if (pFile) {
        fprintf(pFile, " ===========================LSC table===========================\n");
        const char *pChannelName[] = {"R", "GR", "GB", "B"};
        for (int channel = 0; channel < LSC_TOTAL_CHANNELS; channel++) {
            for (int i = 0; i < LSC_MESH_PT_V_V40; i++) {
                fprintf(pFile, "%s: ", pChannelName[channel]);
                for (int j = 0; j < LSC_MESH_PT_H_V40; j++) {
                    fprintf(pFile, "%f ", lensShadingMap[channel][i * LSC_MESH_PT_H_V40 + j]);
                }
                fprintf(pFile, "\n");
            }
        }

        fprintf(pFile, " ===========================Gamma table===========================\n");
        for (int i = 0; i < 65; i++) {
            fprintf(pFile, "%f ", f_gamma[i]);
        }
        fprintf(pFile, "\n");

        fprintf(pFile, " ===========================faceInfo table===========================\n");
        for (int i = 0; i < faceInfo.faceNumber; i++) {
            fprintf(pFile, "faceIndex:%d faceRect[%d,%d,%d,%d]\n", i, faceInfo.pfaceRects[i].x,
                    faceInfo.pfaceRects[i].y, faceInfo.pfaceRects[i].width,
                    faceInfo.pfaceRects[i].height);
        }
        fprintf(pFile, "\n");

        fclose(pFile);
    } else {
        MLOGE(Mia2LogGroupPlugin, "failed to open file %s", filename);
    }
}

static void getLensShadingMapSerialize(const ImageParams *buffer, unsigned char metaSize,
                                       FLOAT lensShadingMapSerialize[][LSC_MESH_ROLLOFF_SIZE])
{
    unsigned int i = 0;
    const float *lensShadingMap = getLensShadingMap(buffer, metaSize);
    if (lensShadingMap) {
        for (i = 0; i < (LSC_MESH_ROLLOFF_SIZE * LSC_TOTAL_CHANNELS); i++) {
            lensShadingMapSerialize[i % LSC_TOTAL_CHANNELS][i / LSC_TOTAL_CHANNELS] =
                lensShadingMap[i];
        }
    } else {
        MLOGW(Mia2LogGroupPlugin, "failed to get lsc mapping");
    }
}

static void getAWB(const ImageParams *buffer, unsigned char metaSize, float *awb)
{
    const AWBFrameControl *pAWBFrameControl = getAWBFrameControl(buffer, metaSize);
    if (pAWBFrameControl) {
        awb[0] = pAWBFrameControl->AWBGains.rGain;
        awb[1] = pAWBFrameControl->AWBGains.gGain;
        awb[2] = pAWBFrameControl->AWBGains.bGain;
    }
}

static void getGamma(const ImageParams *buffer, unsigned char metaSize, float *gamma,
                     unsigned int count)
{
    const GammaInfo2 *gammaInfo = getGreenGamma(buffer, metaSize);
    if (gammaInfo) {
        assert(count == sizeof(gammaInfo->gammaG) / sizeof(gammaInfo->gammaG[0]));
        for (int i = 0; i < count; i++) {
            gamma[i] = static_cast<float>(gammaInfo->gammaG[i]);
        }
        if (gamma[count - 1] == 0)
            gamma[count - 1] = 1023.0f;
    }
}



static int getAnchor(const ImageParams *buffer, unsigned char metaSize)
{
#if HDSR
    if (getHDSREnable(buffer, metaSize))
        return metaSize - 1;
#endif
    return getAnchorIndex(buffer, metaSize).getDataWithDefault<int>(-1);
}

static int getFaceInfo(const FaceResult * faceResult, sr_rect_t * faceRect)
{
    if(faceResult){
        const ChiRectEXT * const chiFaceInfo = faceResult->chiFaceInfo;
        for(unsigned int i = 0; i < faceResult->faceNumber; i++)
            faceRect[i] = {chiFaceInfo[i].left, chiFaceInfo[i].top,
                           chiFaceInfo[i].right - chiFaceInfo[i].left,
                           chiFaceInfo[i].top - chiFaceInfo[i].bottom};
        return faceResult->faceNumber;
    }
    MLOGI(Mia2LogGroupPlugin, "MiaNodeSr faces are not found:%#x", faceResult);
    return 0;
}

static void process(const AlgorithmAdapter *me, const SessionInfo *sessionInfo,
                    const Request *request)
{
    static const sr_extention_para_t extention_param = {.c = "/vendor/etc/camera"};
    static mutex processMutex;

    MLOGI(Mia2LogGroupPlugin, "get request %p from session %p at adapter %p", request, sessionInfo, me);
    // need add metadata null check
    void *handle = nullptr;
    unsigned int retSDK = 0;
    const unsigned int inputCount = request->getInputCount();
    const ImageParams *const in = request->getInput();
    const ImageParams *const out = request->getOutput();
    float f_gamma[65] = {};
    FLOAT lensShadingMap[LSC_TOTAL_CHANNELS][LSC_MESH_ROLLOFF_SIZE] = {};
    float awbInfo[3] = {};
    sr_rect_t face_rect[MAX_FACE] = {};
    const sr_face_info_t faceInfo = {face_rect, getFaceInfo(getFaceRect(in, inputCount), face_rect)};
    vector<sr_rect_t> pRect(inputCount);
    vector<float> iso = getIso(in, inputCount);
    vector<int> exp_time = getExposureTime(in, inputCount, int);
    int anchor = getAnchor(in, inputCount);
    vector<int32_t> af = getLensPosition(in, inputCount, int32_t);
    sr_image_t srOutBuffer = {{getPlane(out)[0], getPlane(out)[1]},
                              SR_PIX_FMT_NV12,
                              static_cast<int>(getWidth(out)),
                              static_cast<int>(getHeight(out)),
                              static_cast<int>(getStride(out))};

    MLOGI(Mia2LogGroupPlugin, "get out buffer plane[%p, %p], %d x %d, stride %d", srOutBuffer.planes[0], srOutBuffer.planes[1], srOutBuffer.width, srOutBuffer.height, srOutBuffer.stride);
    vector<sr_image_t> input(inputCount);
    for (uint32_t i = 0; i < inputCount; i++){
        input[i] = {{getPlane(in + i)[0], getPlane(in + i)[1]},
                    SR_PIX_FMT_NV12,
                    static_cast<int>(getWidth(in + i)),
                    static_cast<int>(getHeight(in + i)),
                    static_cast<int>(getStride(in + i))};
        MLOGI(Mia2LogGroupPlugin, "get in buffer[%u] plane[%p, %p], %d x %d, stride %d", i, input[i].planes[0], input[i].planes[0],input[i].width, input[i].height, input[i].stride);
    }

    const sr_camera_info_t *info =
        getCameraInfo(in, inputCount, sessionInfo, [&](uint32_t fwkCameraId) mutable {
            unsigned int index = (anchor >= 0 && anchor < inputCount? anchor : 0);
            for (unsigned int i = 0; i < inputCount; i++)
                getCropRegion(getMetaData(in + i, "xiaomi.snapshot.residualCropRegion"),
                              getRefSize(fwkCameraId),
                              input[i].width,
                              input[i].height,
                              [&pRect, i](int left, int top, int width, int height) mutable {
                                  pRect[i] = {left, top, width, height};
                              });
        #if HDSR
            if (getHDSREnable(in, inputCount))
                publishHDSRCropRegion(getLogicalMetaDataPointer(out), pRect[index].x, pRect[index].y, pRect[index].width, pRect[index].height);
        #endif
        });

    getGamma(in, inputCount, f_gamma, sizeof(f_gamma) / sizeof(f_gamma[0]));
    getLensShadingMapSerialize(in, inputCount, lensShadingMap);
    getAWB(in, inputCount, awbInfo);

    // config paramters
    sr_interface_param_t srParam = {};
    srParam.imageParam.nImageGain = iso[0];
    srParam.specificParam.fImgEdgeEnhanceS = 1.0;
    srParam.specificParam.fImgContrastEnhanceS = 0;
    srParam.specificParam.fMotionDetectS = 2.0; //!!
    srParam.specificParam.fModelDenoiseS = -1;
    srParam.specificParam.fModelFinetuneDenoiseS = 0;
    srParam.specificParam.fModelFinetuneMotionDenoiseS = 1;
    setUVDenoiseParam(iso[0], srParam.uvDenoiseParam, srParam.specificParam);

    MLOGI(Mia2LogGroupPlugin, "init srParam by iso %f", iso[0]);

    dump(in, inputCount, me->getName(), [=](unsigned int index) mutable -> string {
        stringstream filename;
        filename << info->name << "_crop_" << pRect[index].x << '_' << pRect[index].y << '_'
                 << pRect[index].width << '_' << pRect[index].height << "_iso_"
                 << static_cast<int>(iso[index]) << "_exp_time_" << exp_time[index] << "_anchor_"
                 << anchor << "_input";
        return filename.str();
    });

    dumpParams(lensShadingMap, f_gamma, faceInfo);
    define_extention(extention);
    // In CETUS product, AWB algo is provided by XIAOMI, not by QCOM,CCM is set in IPE,not
    // in BPS
    // pCCM = (float *)pAWBFrameControl->AWBCCM[0].CCM;
    processMutex.lock();

    recordSDKRet(retSDK,"create ex %d", sr_create_ex(*info, &handle));
    set_extention_ptr(handle, extention, retSDK);
    recordSDKRet(retSDK, "set extention(xml): %d",sr_set_extention(handle, SR_EXTENTION_XML_PARAMS, &extention_param));
    recordSDKRet(retSDK, "set callback: %d", sr_set_callback(handle, onLibEvent));
    recordSDKRet(retSDK, "run ex: %d", sr_run_ex(handle, input.data(), inputCount, anchor, &pRect[(anchor >= 0 && anchor < inputCount) ? anchor : 0], &srParam, &srOutBuffer));
    recordSDKRet(retSDK, "destroy: %d", srsuper_destroy(handle));
    processMutex.unlock();

    dump(out, 1, me->getName(), [=](unsigned int index) mutable -> string {
        stringstream filename;
        index = ((anchor >= 0 && anchor < inputCount) ? anchor : 0);
        filename << info->name << "_crop_" << pRect[index].x << '_' << pRect[index].y << '_'
                 << pRect[index].width << '_' << pRect[index].height << "_iso_"
                 << static_cast<int>(iso[index]) << "_exp_time_" << exp_time[index] << "_anchor_"
                 << anchor << "_output";
        return filename.str();
    });

    sessionInfo->setResultMetadata(
        inputCount, request->getTimeStamp(),(stringstream() << "sr:{algo:" << me->getName() << " inNum:" << inputCount << " info:" << info << ' '
            << vector2exifstr<int32_t>("af", af).c_str() << ' '
            << vector2exifstr<float>("iso", iso).c_str() << ' '
            << vector2exifstr<int>("exp", exp_time).c_str() << "}").str()
        );
    MLOGI(Mia2LogGroupPlugin, "get record of sdk: %#x", retSDK);
}

static SingletonAlgorithmAdapter adapter = SingletonAlgorithmAdapter("tetras3", process);

extern Entrance tetras3 = &adapter;
