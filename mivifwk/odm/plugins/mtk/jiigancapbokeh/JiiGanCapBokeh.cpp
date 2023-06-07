#include "JiiGanCapBokeh.h"

#include <pthread.h>

#include "intsense_utils.h"

using namespace mialgo2;

#undef LOG_TAG
#define LOG_TAG "MIA_N_JG_RCBK"

#define CAL_DATA_PATH       "/data/vendor/camera/"
#define CAL_DATA_PATH_W_T   "/data/data/com.android.camera/files/back_dual_camera_caldata.bin"
#define CAL_DATA_PATH_W_U   "/data/data/com.android.camera/files/back_dual_camera_caldata_wu.bin"
#define CAL_DATA_PATH_FRONT "/data/data/com.android.camera/files/front_dual_camera_caldata.bin"

#define VENDOR_DOF_MODEL_CAPTURE_CACHE "/vendor/etc/camera/model/dof_model/capture_cache"
#define SYSTEM_DOF_MODEL_CAPTURE_MODEL "/data/data/com.android.camera/files/capture_model"

#define MEMCP_PATH "/vendor/etc/camera/jiiganmodel/com.xiaomi_l11_mecp.bin"
#define MEMCP_PATH_FOR_NOCALIB \
    "/vendor/etc/camera/jiiganmodel/com.xiaomi_l11_mecp_for\
_nocali.bin"
#define MODEL_PATH "/vendor/etc/camera/jiiganmodel/dof_model"

#define CALIBRATIONDATA_LEN 4096

#define DOF_ROTATION 270

#ifdef ANDROMEDA_CAM
#define CALIBRATIONDATA_OFFSET_W_T 5780
#else
#define CALIBRATIONDATA_OFFSET_W_T 7942
#endif
#define CALIBRATIONDATA_OFFSET_W_U 5732

#define CALIBRATIONDATA_OFFSET_FRONT 2911

#define SAFE_DEL(ptr) \
    if (ptr) {        \
        delete ptr;   \
        ptr = NULL;   \
    }
#define SAFE_FREE(ptr) \
    if (ptr) {         \
        free(ptr);     \
        ptr = NULL;    \
    }

typedef struct TagDisparityFileHeader
{
    int32_t i32Header;
    int32_t i32HeaderLength;
    int32_t i32PointX;
    int32_t i32PointY;
    int32_t i32BlurLevel;
    int32_t reserved[32];
    int32_t i32DataLength;
} DisparityFileHeader;

typedef struct
{
    int orient;
    int mode;
    MIIMAGEBUFFER pSrcImg;
    MIIMAGEBUFFER pDstImg;
    MIIMAGEBUFFER pBokehImg;
    MHandle *mRelightingHandle;
} LocalThreadParams;

typedef struct
{
    // int m_mapped_orient;
    int i32Width;  // [in] The image width
    int i32Height; // [in] The image height
    MHandle *mRelightingHandle;
} RelightThreadParams;

typedef struct
{
    LPASVLOFFSCREEN pImage;
    BeautyFeatureParams beautyFeatureParams;
    float gender;
    int orient;
    int rearCamera;
} BokehFaceBeautyParams;

static std::mutex mtx;

static int get_device_region()
{
    char devicePtr[PROPERTY_VALUE_MAX];
    property_get("ro.boot.hwc", devicePtr, "0");
    if (strcasecmp(devicePtr, "India") == 0)
        return BEAUTYSHOT_REGION_INDIAN;
    else if (strcasecmp(devicePtr, "CN") == 0)
        return BEAUTYSHOT_REGION_CHINA;

    return BEAUTYSHOT_REGION_GLOBAL;
}

static void *bokehRelightingThreadProcInit(void *params)
{
    MLOGD(Mia2LogGroupPlugin, "relighting init thread start");
    int ret = 0;
    double t0, t1;
    t0 = t1 = 0.0f;
    RelightThreadParams *prtp = (RelightThreadParams *)params;
    int width = prtp->i32Width;
    int height = prtp->i32Height;
    MHandle relightingHandle = NULL;
    // m_useBokeh = false;

    t0 = PluginUtils::nowMSec();
    ret = AILAB_PLI_Init(&relightingHandle, MI_PL_INIT_MODE_PERFORMANCE, width, height);
    t1 = PluginUtils::nowMSec();
    MLOGE(Mia2LogGroupPlugin, "midebug: AILAB_PLI_Init nRet: %d,  costtime =%.2f ", ret, t1 - t0);
    if (ret != 0) {
        *(prtp->mRelightingHandle) = NULL;
        return NULL;
    } else {
        *(prtp->mRelightingHandle) = relightingHandle;
    }
    MLOGD(Mia2LogGroupPlugin, "midebug:AILAB_PLI_Init RelightingHandle:%p", relightingHandle);
    return NULL;
}

static void *bokehRelightingThreadProcPreprocess(void *params)
{
    MLOGD(Mia2LogGroupPlugin, "midebug: relighting process thread start");
    LocalThreadParams *pParam = (LocalThreadParams *)params;
    int ret = 0;
    double t0, t1;
    t0 = t1 = 0.0f;
    MIPLLIGHTREGION lightRegion = {};
    int orientation = pParam->orient;
    MHandle relightingHandle = *(pParam->mRelightingHandle);

    // m_useBokeh = false;
    t0 = PluginUtils::nowMSec();

    MLOGD(Mia2LogGroupPlugin, "AILAB_PLI_PreProcess src i32Width %d i32Height %d ",
          pParam->pSrcImg.i32Width, pParam->pSrcImg.i32Height);
    MLOGD(Mia2LogGroupPlugin, "AILAB_PLI_PreProcess dst i32Width %d i32Height %d ",
          pParam->pDstImg.i32Width, pParam->pDstImg.i32Height);

    if (relightingHandle == NULL) {
        MLOGI(Mia2LogGroupPlugin, "m_hRelightingHandle: %p", relightingHandle);
        return NULL;
    }

    ret = AILAB_PLI_PreProcess(relightingHandle, &pParam->pSrcImg, orientation, &lightRegion);
    t1 = PluginUtils::nowMSec();
    if (ret != 0) {
        MLOGE(Mia2LogGroupPlugin, "AILAB_PLI_PreProcess failed= %d", ret);
        // m_useBokeh = true;
        AILAB_PLI_Uninit(&relightingHandle);
        *(pParam->mRelightingHandle) = NULL;
        return NULL;
    }

    MLOGD(Mia2LogGroupPlugin, "AILAB_PLI_PreProcess= %d: preprocess=%.2f", ret, t1 - t0);
    return NULL;
}

static void *bokehFaceBeautyThreadProc(void *params)
{
    MRESULT ret = 0;
    BokehFaceBeautyParams *beautyParams = (BokehFaceBeautyParams *)params;
    LPASVLOFFSCREEN pImage = beautyParams->pImage;
    BeautyShot_Image_Algorithm *beautyShotImageAlgorithm = NULL;
    int orient = beautyParams->orient;
    // int facebeautyLevel = beautyParams->facebeautyLevel;
    int region_code = get_device_region();
    int mfeatureCounts = beautyParams->beautyFeatureParams.featureCounts;

    MLOGD(Mia2LogGroupPlugin, " arcdebug bokeh orient=%d region=%d", orient, region_code);
    double t0, t1;
    t0 = PluginUtils::nowMSec();

    ret = Create_BeautyShot_Image_Algorithm(BeautyShot_Image_Algorithm::CLASSID,
                                            &beautyShotImageAlgorithm);
    if (ret != 0 || beautyShotImageAlgorithm == NULL) {
        MLOGE(Mia2LogGroupPlugin, "arcdebug bokeh: Create_BeautyShot_Image_Algorithm, fail");
        return NULL;
    }

    ret = beautyShotImageAlgorithm->Init(ABS_INTERFACE_VERSION, XIAOMI_FB_FEATURE_LEVEL);
    if (ret != 0) {
        beautyShotImageAlgorithm->Release();
        beautyShotImageAlgorithm = NULL;
        MLOGE(Mia2LogGroupPlugin, "arcdebug bokeh: beautyShotImageAlgorithm init Failed");

        return NULL;
    }

    for (unsigned int i = 0; i < mfeatureCounts; i++) {
        beautyShotImageAlgorithm->SetFeatureLevel(
            beautyParams->beautyFeatureParams.beautyFeatureInfo[i].featureKey,
            beautyParams->beautyFeatureParams.beautyFeatureInfo[i].featureValue);
        MLOGD(Mia2LogGroupPlugin, "arcsoft debug beauty capture: %s = %d",
              beautyParams->beautyFeatureParams.beautyFeatureInfo[i].TagName,
              beautyParams->beautyFeatureParams.beautyFeatureInfo[i].featureValue);
    }

    ABS_TFaces faceInfoIn, faceInfoOut;
    ret = beautyShotImageAlgorithm->Process(pImage, pImage, &faceInfoIn, &faceInfoOut, orient,
                                            region_code);

    beautyShotImageAlgorithm->UnInit();
    beautyShotImageAlgorithm->Release();
    beautyShotImageAlgorithm = NULL;
    t1 = PluginUtils::nowMSec();
    MLOGD(Mia2LogGroupPlugin,
          "arcdebug bokeh beauty capture process ret=%ld w:%d, h:%d, costtime = %.2f", ret,
          pImage->i32Width, pImage->i32Height, t1 - t0);

    return NULL;
}

void MiaNodeIntSenseCapBokeh::processAIFilter(WAOFFSCREEN &inputTeleImage, Image &rMask)
{
    double t0, t1;
    t0 = PluginUtils::nowMSec();
    bokeh::MibokehFilterPara para;
    para.stride = inputTeleImage.aWidth;
    para.height = inputTeleImage.i32Height;
    para.width = inputTeleImage.i32Width;
    para.src_Img = inputTeleImage.ppu8Plane[0];
    para.dst_img = inputTeleImage.ppu8Plane[0];
    para.category = mSceneMode;
    para.filter_strength = -1; //?????
    para.format = bokeh::PixelFormat::NV12;
    para.is_preview = false;
    para.uv_offset = para.stride * para.height;

    bokeh::DispMap dispMap;

    constexpr MDWord MASK_FROM_XIAOMI = 0xffff0030;
    dispMap.from = MASK_FROM_XIAOMI;
    dispMap.width = rMask.realWidth();
    dispMap.height = rMask.realHeight();
    dispMap.pitch = rMask.realWidth();
    dispMap.reserve = 0;

    if (NULL != mDepthinfo) {
        int dispHeaderSize = sizeof(bokeh::DispMap);
        int dispDataSize = dispMap.height * dispMap.pitch; // or mask.total() ??

        mDepthinfo->lDepthDataSize = dispHeaderSize + dispDataSize;
        mDepthinfo->pDepthData = rMask.data1();
        mDepthinfo->dispMap = dispMap;
        bokeh::Filter(mDepthinfo, &para);
        t1 = PluginUtils::nowMSec();
        MLOGI(Mia2LogGroupPlugin, "processAIFilter X costime=%.2f", t1 - t0);
    }
}

int MiaNodeIntSenseCapBokeh::processRelighting(WAOFFSCREEN &input, WAOFFSCREEN &output,
                                               WAOFFSCREEN &bokehImg, Image *rMask)
{
    int ret = PROCSUCCESS;
    double t0, t1, t2;
    t0 = PluginUtils::nowMSec();
    MIPLLIGHTPARAM lightParam = {0};
    lightParam.i32LightMode = mLightingMode;

    MIIMAGEBUFFER SrcImg;
    MIIMAGEBUFFER DstImg;
    MIIMAGEBUFFER SrcDokehImg;

    prepareRelightImage(SrcImg, input);         // [in] The input image data unused !!!
    prepareRelightImage(DstImg, output);        // [in] The output image data
    prepareRelightImage(SrcDokehImg, bokehImg); // [in] The bokeh image data

    MLOGI(Mia2LogGroupPlugin, "mRelightingHandle = %p", mRelightingHandle);
    if (mRelightingHandle) {
        MIPLDEPTHINFO depthInfo;
        MIPLDEPTHINFO *plDepthInfo = &depthInfo;
        if (rMask != NULL) {
            depthInfo.i32MaskWidth = rMask->realWidth();
            depthInfo.i32MaskHeight = rMask->realHeight();
            depthInfo.pMaskImage = (MUInt8 *)rMask->data1();
        }
        depthInfo.pBlurImage = &SrcDokehImg;
        t1 = PluginUtils::nowMSec();
        ret = AILAB_PLI_Process(mRelightingHandle, plDepthInfo, &SrcImg, &lightParam, &DstImg);
        t2 = PluginUtils::nowMSec();
        if (ret != 0) {
            MLOGE(Mia2LogGroupPlugin,
                  "AILAB_PLI_Process =failed= %d,mRelightingHandle=%p,costime=%.2f", ret,
                  mRelightingHandle, t2 - t1);
            AILAB_PLI_Uninit(&mRelightingHandle);
            mRelightingHandle = NULL;

            return ret;
        }
        AILAB_PLI_Uninit(&mRelightingHandle);
        mRelightingHandle = NULL;

        MLOGI(Mia2LogGroupPlugin, "AILAB_PLI_Process ret = %d,mode=%d,costime=%.2f X", ret,
              mLightingMode, t2 - t0);
    }
    return ret;
}

MiaNodeIntSenseCapBokeh::~MiaNodeIntSenseCapBokeh()
{
    MLOGI(Mia2LogGroupPlugin, " distruct MiaNodeIntSenseCapBokeh");
    deInit();
};

MiaNodeIntSenseCapBokeh::MiaNodeIntSenseCapBokeh()
{
    MLOGI(Mia2LogGroupPlugin, " construct MiaNodeIntSenseCapBokeh");
}

int MiaNodeIntSenseCapBokeh::init(uint32_t cameraId)
{
    mCalibration = NULL;
    mMecpData = NULL;
    mDumpPriv = false;
    mDumpTimeStamp = 0;
    mDrawROI = 0;
    mViewCtrl = 0;
    mOrient = 0;
    mSceneMode = 0;
    mSceneEnable = 0;
    mBeautyEnabled = 0;
    mRelightingHandle = NULL;
    mDepth = NULL;
    mRefocus = NULL;
    mDepthinfo = NULL;

    memset(mModuleInfo, 0, sizeof(mModuleInfo));
    memset(&mFaces, 0, sizeof(mFaces));
    memset(&mBokehParam, 0, sizeof(WA_DCIR_PARAM));
    memset(&mFrameParams, 0, sizeof(is_frame_params_t));
    memset(&mFovRectIFEForTele, 0, sizeof(MIRECT));
    memset(&mFovRectIFEForWide, 0, sizeof(MIRECT));
    memset(&mActiveArraySizeWide, 0, sizeof(MIRECT));
    memset(&mActiveArraySizeTele, 0, sizeof(MIRECT));
    memset(&mRtFocusROIOnInputData, 0, sizeof(MIRECT));

    mCameraId = cameraId;

    if (CAM_COMBMODE_FRONT == mCameraId || CAM_COMBMODE_FRONT_BOKEH == mCameraId ||
        CAM_COMBMODE_FRONT_AUX == mCameraId) {
        mRearCamera = false;
    } else {
        mRearCamera = true;
    }
    mTotalCost = 0;
    mProcessCount = 0;

    return iniEngine();
}

int MiaNodeIntSenseCapBokeh::deInit()
{
    std::unique_lock<std::mutex> lock(mtx);

    MLOGD(Mia2LogGroupPlugin, "mDepth: %p,  mRefocus:%p, mDepthinfo:%p E", mDepth, mRefocus,
          mDepthinfo);
    SAFE_FREE(mBokehParam.faceParam.pi32FaceAngles);
    SAFE_FREE(mBokehParam.faceParam.prtFaces);
    SAFE_FREE(mCalibration);
    SAFE_FREE(mMecpData);

    if (mDepth != NULL) {
        delete mDepth;
        mDepth = NULL;
    }
    if (mRefocus != NULL) {
        delete mRefocus;
        mRefocus = NULL;
    }
    if (mDepthinfo) {
        delete mDepthinfo;
        mDepthinfo = NULL;
    }

    MLOGD(Mia2LogGroupPlugin, " X");
    return MIA_RETURN_OK;
}

int MiaNodeIntSenseCapBokeh::iniEngine()
{
    bool ret = false;
    const char *pVersion = NULL;
    mBlurLevel = 0;
    mFNum = 4.0;

    std::unique_lock<std::mutex> lock(mtx);

    if (mBokehParam.faceParam.pi32FaceAngles == NULL && mBokehParam.faceParam.prtFaces == NULL) {
        mBokehParam.faceParam.prtFaces = (PMRECT)malloc(sizeof(MRECT) * MAX_FACE_NUM);
        mBokehParam.faceParam.pi32FaceAngles = (MInt32 *)malloc(sizeof(MInt32) * MAX_FACE_NUM);
        memset(mBokehParam.faceParam.prtFaces, 0, sizeof(MRECT) * MAX_FACE_NUM);
        memset(mBokehParam.faceParam.pi32FaceAngles, 0, sizeof(MInt32) * MAX_FACE_NUM);

        mBokehParam.faceParam.i32FacesNum = 0;
    }

    int beautyFeatureCounts = mBeautyFeatureParams.featureCounts;
    for (int i = 0; i < beautyFeatureCounts; i++) {
        mBeautyFeatureParams.beautyFeatureInfo[i].featureValue = 0;
    }

    mIsNonCalibAlgo = 0;
    mForceNonCalibAlgo = 0;
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.forcenoncalib", prop, "0");
    mForceNonCalibAlgo = (int32_t)atoi(prop);
    MLOGI(Mia2LogGroupPlugin, "mForceNonCalibAlgo = %d cid = %d", mForceNonCalibAlgo, mCameraId);

    mCaliDataSz = 0;
    mMecpDataSz = 0;
    const char *fileName = NULL;
    if (mCameraId == CAM_COMBMODE_FRONT || mCameraId == CAM_COMBMODE_FRONT_BOKEH) {
        fileName = CAL_DATA_PATH_FRONT;
    } else if (mCameraId == CAM_COMBMODE_REAR_BOKEH_WU) {
        fileName = CAL_DATA_PATH_W_U;
    } else {
        fileName = CAL_DATA_PATH_W_T;
    }
    if ((access(fileName, F_OK) == -1) || mForceNonCalibAlgo == 1) {
        mIsNonCalibAlgo = 1;
        MLOGI(Mia2LogGroupPlugin,
              "calibdata does not exist, using non-calib JIIGAN algom.mIsNonCalibAlgo = %d !",
              mIsNonCalibAlgo);
    } else {
        mIsNonCalibAlgo = 0;
        ret = readFile(fileName, &mCalibration, &mCaliDataSz);
    }

    if (ret == false && mIsNonCalibAlgo == 0) {
        MLOGE(Mia2LogGroupPlugin, "read calidata failed");
        mCaliDataSz = 0;
        SAFE_FREE(mCalibration);
    }

    fileName = NULL;
    if (mIsNonCalibAlgo == 1) {
        fileName = MEMCP_PATH_FOR_NOCALIB;
    } else {
        fileName = MEMCP_PATH;
    }
    ret = readFile(fileName, &mMecpData, &mMecpDataSz);
    if (ret == false) {
        MLOGE(Mia2LogGroupPlugin, "read mecpdata failed");
        mMecpDataSz = 0;
        SAFE_FREE(mMecpData);
    }

    MLOGI(Mia2LogGroupPlugin, " Creat mDepth and mRefocus");
    if (mDepth == NULL) {
        mDepth = new Depth();
        pVersion = mDepth->getVersion();
        MLOGI(Mia2LogGroupPlugin, "get algorithm library version for Depth: %s cameraId %d",
              pVersion, mCameraId);
    }
    if (mRefocus == NULL) {
        mRefocus = new Refocus();
        pVersion = mRefocus->getVersion();
        MLOGI(Mia2LogGroupPlugin, "get algorithm library version for Refocus: %s cameraId %d",
              pVersion, mCameraId);
    }

    if (mDepth != NULL && mRefocus != NULL &&
        ((mIsNonCalibAlgo == 0 &&
          (mMecpDataSz != 0 && mMecpData != NULL && mCaliDataSz != 0 && mCalibration != NULL)) ||
         (mIsNonCalibAlgo == 1 && (mMecpDataSz != 0 && mMecpData != NULL)))) {
        int result = 0;
        WDepthConfig config;
        memset(&config, 0, sizeof(WDepthConfig));
        config.model.path = const_cast<char *>(MODEL_PATH);
        if (mIsNonCalibAlgo == 0) {
            config.calib.length = mCaliDataSz;
            config.calib.data = reinterpret_cast<void *>(mCalibration);
        }
        config.mecp.length = mMecpDataSz;
        config.mecp.data = reinterpret_cast<void *>(mMecpData);
        result = mDepth->init(config);
        if (result != 1) {
            MLOGE(Mia2LogGroupPlugin, "init depth Failed: %d !!!", result);
        }

        result = 0;
        WRefocusConfig refConfig;
        memset(&refConfig, 0, sizeof(WRefocusConfig));
        refConfig.mecpBlob.length = mMecpDataSz;
        refConfig.mecpBlob.data = reinterpret_cast<void *>(mMecpData);
        result = mRefocus->init(refConfig);
        if (1 != result) {
            MLOGD(Mia2LogGroupPlugin, "mRefocus init failed! ret: %d", result);
        }
    } else {
        MLOGE(Mia2LogGroupPlugin, "MecpData or CaliData is null !!!");
    }

    if (mDepthinfo == NULL) {
        mDepthinfo = new bokeh::MIBOKEH_FILTER_DEPTHINFO;
        if (mDepthinfo == NULL) {
            MLOGE(Mia2LogGroupPlugin, "failed to malloc buffer!!!");
        }
    }

    MLOGD(Mia2LogGroupPlugin, "mDepth: %p,  mRefocus:%p, mDepthinfo:%p X", mDepth, mRefocus,
          mDepthinfo);
    return ret;
}

void MiaNodeIntSenseCapBokeh::updateExifInfo(std::string &results)
{
    char buf[1024] = {0};

    snprintf(buf, sizeof(buf),
             "jiiGanCapBokeh:{processCount: %d blurLevel:%.3f beautyEnabled:%d "
             "sceneEnable:%d sceneMode:%d lightingMode:%d costTime:%.3f}",
             mProcessCount, mFNum, mBeautyEnabled, mSceneEnable, mSceneMode, mLightingMode,
             mTotalCost);
    results = buf;
}

int MiaNodeIntSenseCapBokeh::processBuffer(struct MiImageBuffer *teleImage,
                                           struct MiImageBuffer *wideImage,
                                           struct MiImageBuffer *outputBokeh,
                                           struct MiImageBuffer *outputTele,
                                           struct MiImageBuffer *outputDepth,
                                           camera_metadata_t *meta, std::string &results)
{
    int result = PROCSUCCESS;
    bool dumpVisible = PluginUtils::isDumpVisible();

    if (teleImage == NULL || wideImage == NULL || outputBokeh == NULL || outputTele == NULL ||
        outputDepth == NULL || meta == NULL) {
        MLOGE(Mia2LogGroupPlugin, "error invalid param %p %p %p %p", teleImage, wideImage,
              outputDepth, meta);
        return PROCFAILED;
    }

    uint8_t *depthOutput = outputDepth->plane[0];
    if (depthOutput != NULL) {
        memset(depthOutput, 0x0F, (outputDepth->stride) * (outputDepth->scanline));
    }

    result = setFrameParams(teleImage, wideImage, outputBokeh, outputTele, outputDepth, meta);
    if (PROCSUCCESS != result) {
        MLOGE(Mia2LogGroupPlugin, "ERR! setFrameParams failed:%d", result);
        return result;
    }
    checkPersist();

    WAOFFSCREEN &stTeleImage = mFrameParams.mainimg;
    WAOFFSCREEN &stWideImage = mFrameParams.auximage;
    WAOFFSCREEN &stDestImage = mFrameParams.outimage;
    // WAOFFSCREEN &stDepthImage = mFrameParams.depthimage;

    if (mDumpPriv || dumpVisible) {
        mDumpTimeStamp = PluginUtils::nowMSec();

        dumpYUVData(&stWideImage, 1, "INPUT_wide_frame_", mDumpTimeStamp);
        dumpYUVData(&stTeleImage, 0, "INPUT_tele_frame_", mDumpTimeStamp);
    }

#define SHOW_WIDE_ONLY 1
#define SHOW_TELE_ONLY 2
#define SHOW_HALF_HALF 3

    switch (mViewCtrl) {
    case SHOW_WIDE_ONLY:
        copyImage(&stWideImage, &stDestImage);
        break;
    case SHOW_TELE_ONLY:
        copyImage(&stTeleImage, &stDestImage);
        break;
    case SHOW_HALF_HALF:
        copyImage(&stTeleImage, &stDestImage);
        mergeImage(&stDestImage, &stWideImage, 2);
        break;
    default:
        doProcess();
        break;
    }

    mProcessCount++;
    updateExifInfo(results);

    if (mDrawROI) {
        DrawPoint(&stDestImage, mFrameParams.refocus.ptFocus, 10, 0);
        DrawRectROI(&stDestImage, mRtFocusROIOnInputData, 5, 0);
        for (int i = 0; i < mBokehParam.faceParam.i32FacesNum; i++)
            DrawRect(&stDestImage, mBokehParam.faceParam.prtFaces[i], 5, 3);
    }

    if (mDumpPriv || dumpVisible) {
        dumpYUVData(&stDestImage, 3, "OUT_frame_", mDumpTimeStamp);
    }

    return PROCSUCCESS;
}

int MiaNodeIntSenseCapBokeh::readCalibrationDataFromFile(const char *szFile, void *pCaliBuff,
                                                         uint32_t dataSize)
{
    MLOGD(Mia2LogGroupPlugin, "E file: %s", szFile);
    if (dataSize == 0 || NULL == pCaliBuff) {
        MLOGE(Mia2LogGroupPlugin, "(OUT) failed ! Invalid param");
        return MIA_RETURN_INVALID_PARAM;
    }

    uint32_t bytes_read = 0;
    FILE *califile = NULL;
    califile = fopen(szFile, "r");
    if (NULL != califile) {
        bytes_read = fread(pCaliBuff, 1, dataSize, califile);
        MLOGD(Mia2LogGroupPlugin, "read calidata bytes_read = %d, dataSize:%d", bytes_read,
              dataSize);
        fclose(califile);
    } else {
        MLOGE(Mia2LogGroupPlugin, "open califile(%s) failed %s", szFile, strerror(errno));
        return MIA_RETURN_OPEN_FAILED;
    }
    if (bytes_read != dataSize) {
        MLOGE(Mia2LogGroupPlugin, "Read incorrect size %d vs %d", bytes_read, dataSize);
        return MIA_RETURN_INVALID_CALL;
    }

    MLOGD(Mia2LogGroupPlugin, "X success !");
    return MIA_RETURN_OK;
}

void MiaNodeIntSenseCapBokeh::checkPersist()
{
    MLOGD(Mia2LogGroupPlugin, "E");

    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.jiigan.cp.dumpimg", prop, "0");
    mDumpPriv = (bool)atoi(prop);

    // m_nDebugView:
    //         0:algo
    //         1:only show wide
    //         2:only show tele
    //         3:show half wide and half tele
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.jiigan.cp.debugview", prop, "0");
    mViewCtrl = atoi(prop);

    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.jiigan.cp.drawroi", prop, "0");
    mDrawROI = atoi(prop);

    MLOGD(Mia2LogGroupPlugin, "X");
}

void MiaNodeIntSenseCapBokeh::dumpYUVData(WAOFFSCREEN *pASLIn, uint32_t index,
                                          const char *namePrefix, long long timestamp)
{
    MLOGD(Mia2LogGroupPlugin, "IN");

    char filename[256];
    memset(filename, 0, sizeof(char) * 256);
    snprintf(filename, sizeof(filename), "%s%s_%llu_%d_%dx%d.yuv", PluginUtils::getDumpPath(),
             namePrefix, timestamp, index, pASLIn->pi32Pitch[0], pASLIn->i32Height);

    MLOGD(Mia2LogGroupPlugin, "try to open file : %s", filename);

    int file_fd = open(filename, O_RDWR | O_CREAT | O_APPEND, 0777);
    if (file_fd) {
        MLOGD(Mia2LogGroupPlugin, "dumpYUVData open success, ppu8Plane[0, 1]: (%p, %p)",
              pASLIn->ppu8Plane[0], pASLIn->ppu8Plane[1]);
        uint32_t bytes_write;
        bytes_write =
            write(file_fd, pASLIn->ppu8Plane[0], pASLIn->pi32Pitch[0] * pASLIn->i32Height);
        MLOGD(Mia2LogGroupPlugin, " plane[0] bytes_write: %d", bytes_write);
        bytes_write =
            write(file_fd, pASLIn->ppu8Plane[1], pASLIn->pi32Pitch[0] * (pASLIn->i32Height >> 1));
        MLOGD(Mia2LogGroupPlugin, " plane[1] bytes_write: %d", bytes_write);
        close(file_fd);
    } else {
        MLOGE(Mia2LogGroupPlugin, "fileopen  error: %d", errno);
    }
    MLOGD(Mia2LogGroupPlugin, "X");
}

void MiaNodeIntSenseCapBokeh::prepareImage(struct MiImageBuffer *image, WAOFFSCREEN &stImage)
{
    MLOGD(Mia2LogGroupPlugin,
          "prepareImage width: %d, height: %d, stride: %d, sliceHeight:%d, plans: %d", image->width,
          image->height, image->stride, image->scanline, image->numberOfPlanes);

    stImage.i32Width = image->width;
    stImage.i32Height = image->height;
    stImage.aWidth = image->stride;
    stImage.aHeight = image->scanline;

    if (image->format != CAM_FORMAT_YUV_420_NV12 && image->format != CAM_FORMAT_YUV_420_NV21 &&
        image->format != CAM_FORMAT_Y16) {
        MLOGE(Mia2LogGroupPlugin, "format[%d] is not supported!", image->format);
        return;
    }
    // MLOGD("stImage(%dx%d) PixelArrayFormat:%d", stImage.i32Width,stImage.i32Height,
    // pNodeBuff->PixelArrayFormat);
    if (image->format == CAM_FORMAT_YUV_420_NV12) {
        stImage.u32PixelArrayFormat = wa::Image::IMAGETYPE_NV12;
        MLOGD(Mia2LogGroupPlugin, "stImage(ASVL_PAF_NV12)");
    } else if (image->format == CAM_FORMAT_YUV_420_NV21) {
        stImage.u32PixelArrayFormat = wa::Image::IMAGETYPE_NV21;
        MLOGD(Mia2LogGroupPlugin, "stImage(ASVL_PAF_NV21)");
    } else if (image->format == CAM_FORMAT_Y16) {
        stImage.u32PixelArrayFormat = wa::Image::IMAGETYPE_GRAY;
        MLOGD(Mia2LogGroupPlugin, "stImage(ASVL_PAF_GRAY)");
    }

    for (uint32_t i = 0; i < image->numberOfPlanes; i++) {
        stImage.ppu8Plane[i] = image->plane[i];
        stImage.pi32Pitch[i] = image->stride;
    }

    MLOGD(Mia2LogGroupPlugin, "OUT");
}

void MiaNodeIntSenseCapBokeh::prepareArcImage(ASVLOFFSCREEN &dstimage, WAOFFSCREEN &srtImage)
{
    dstimage.i32Width = srtImage.i32Width;
    dstimage.i32Height = srtImage.i32Height;

    if (srtImage.u32PixelArrayFormat == wa::Image::IMAGETYPE_NV12) {
        dstimage.u32PixelArrayFormat = ASVL_PAF_NV12;
    } else if (srtImage.u32PixelArrayFormat == wa::Image::IMAGETYPE_NV21) {
        dstimage.u32PixelArrayFormat = ASVL_PAF_NV21;
    } else if (srtImage.u32PixelArrayFormat == wa::Image::IMAGETYPE_GRAY) {
        dstimage.u32PixelArrayFormat = ASVL_PAF_GRAY;
    }

    dstimage.ppu8Plane[0] = srtImage.ppu8Plane[0];
    dstimage.ppu8Plane[1] = srtImage.ppu8Plane[1];
    dstimage.pi32Pitch[0] = srtImage.pi32Pitch[0];
    dstimage.pi32Pitch[1] = srtImage.pi32Pitch[1];
}

int MiaNodeIntSenseCapBokeh::setFrameParams(struct MiImageBuffer *inputTele,
                                            struct MiImageBuffer *inputWide,
                                            struct MiImageBuffer *outputBokeh,
                                            struct MiImageBuffer *outputTele,
                                            struct MiImageBuffer *outputDepth,
                                            camera_metadata_t *metaData)
{
    MLOGD(Mia2LogGroupPlugin, "E");
    WAOFFSCREEN &stWideImage = mFrameParams.auximage;
    WAOFFSCREEN &stTeleImage = mFrameParams.mainimg;
    WAOFFSCREEN &stDestImage = mFrameParams.outimage;
    WAOFFSCREEN &stDepthImage = mFrameParams.depthimage;
    WAOFFSCREEN &stDestTeleImage = mFrameParams.outteleimage;

    MLOGD(Mia2LogGroupPlugin, " (tele->plane = %p)", inputTele->plane);
    MLOGD(Mia2LogGroupPlugin, " (wide->plane = %p)", inputWide->plane);
    MLOGD(Mia2LogGroupPlugin, " (output_bokeh->plane = %p)", outputBokeh->plane);
    MLOGD(Mia2LogGroupPlugin, " (output_depth->plane = %p)", outputDepth->plane);
    MLOGD(Mia2LogGroupPlugin, " (output_tele->plane = %p)", outputTele->plane);

    WeightedRegion focusROI = {0};
    // uint32_t afStatus;

    prepareImage(inputTele, stTeleImage);
    prepareImage(inputWide, stWideImage);
    prepareImage(outputBokeh, stDestImage);
    prepareImage(outputDepth, stDepthImage);
    prepareImage(outputTele, stDestTeleImage);

    mInputDataWidth = stTeleImage.i32Width;
    mInputDataHeight = stTeleImage.i32Height;

    if ('\0' == mModuleInfo[0]) {
        void *pMetaData = NULL;
        const char *moduleInfo = "xiaomi.camera.bokehinfo.moduleInfo";
        VendorMetadataParser::getVTagValue(metaData, moduleInfo, &pMetaData);
        if (NULL != pMetaData) {
            strcpy(mModuleInfo, (char *)pMetaData);
            MLOGD(Mia2LogGroupPlugin, "getMetaData moduleInfo [%s]", mModuleInfo);
        } else {
            MLOGE(Mia2LogGroupPlugin, "getMetaData  moduleInfo FAILED");
        }
    }

    // get BokehFNumber
    void *pMetaData = NULL;
    char fNumber[8] = {0};
    const char *BokehFNumber = "xiaomi.bokeh.fNumberApplied";
    VendorMetadataParser::getVTagValue(metaData, BokehFNumber, &pMetaData);
    if (NULL != pMetaData) {
        memcpy(fNumber, static_cast<char *>(pMetaData), sizeof(fNumber));
        mFNum = atof(fNumber);
        MLOGI(Mia2LogGroupPlugin, "MiaNodeIntSenseCapBokeh getMetaData m_fNumberApplied  %f",
              mFNum);
        if (mFNum > 16.0) {
            MLOGI(Mia2LogGroupPlugin,
                  "MiaNodeMiBokeh capture fNum(%f) is bigger than 16.0, force to 16.0", mFNum);
            mFNum = 16.0;
        } else if (mFNum < 1.0) {
            MLOGI(Mia2LogGroupPlugin,
                  "MiaNodeMiBokeh capture fNum(%f) is smaller than 1.0, force to 1.0", mFNum);
            mFNum = 1.0;
        }
    }

#ifndef _HARD_CODE_FOR_PARAMS_
    int orient = 0;
    pMetaData = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_JPEG_ORIENTATION, &pMetaData);
    if (NULL != pMetaData) {
        orient = *static_cast<int *>(pMetaData);
    }
    mFaceOrientation = orient; // for beauty and relighting

    switch (orient) { // for depth.setFace()
    case 0:
        mFrameParams.i32Orientation = wa::Image::ROTATION_270;
        break;
    case 90:
        mFrameParams.i32Orientation = wa::Image::ROTATION_0;
        break;
    case 180:
        mFrameParams.i32Orientation = wa::Image::ROTATION_90;
        break;
    case 270:
        mFrameParams.i32Orientation = wa::Image::ROTATION_180;
        break;
    }

    /* xiaomi.device.orientation
        pMetaData = NULL;
        VendorMetadataParser::getVTagValue(metaData, Device, &pMetaData);
        if (NULL != pMetaData)
        {
            int orientation = *static_cast<int*>(pMetaData);
            mFaceOrientation = (orientation + 90)%360;
        }
    */

    MLOGD(Mia2LogGroupPlugin, "setFrameParams cameraroll:%d, mFaceOrientation: %d",
          mFrameParams.i32Orientation, mFaceOrientation);

    InputMetadataBokeh *pInputCTB = NULL;
    pMetaData = NULL;
    const char *bokehMetadata = "com.xiaomi.multicamera.InputMetadataBokeh";
    VendorMetadataParser::getVTagValue(metaData, bokehMetadata, &pMetaData);
    if (NULL != pMetaData) {
        pInputCTB = static_cast<InputMetadataBokeh *>(pMetaData);
    } else {
        MLOGE(Mia2LogGroupPlugin, "ERR! get BokehMetadata failed!");
        return PROCFAILED;
    }

    for (uint8_t i = 0; i < MAX_LINKED_SESSIONS; i++) {
        MLOGD(Mia2LogGroupPlugin,
              "setFrameParams cameraMetadata[%d] masterCameraRole %d cameraRold %d "
              "activeArraySize(t,l,w,h): [%d, %d, %d ,%d] "
              "fovRectIFE(t,l,w,h): [%d, %d, %d ,%d]",
              i, pInputCTB->cameraMetadata[i].masterCameraRole,
              pInputCTB->cameraMetadata[i].cameraRole,
              pInputCTB->cameraMetadata[i].activeArraySize.top,
              pInputCTB->cameraMetadata[i].activeArraySize.left,
              pInputCTB->cameraMetadata[i].activeArraySize.width,
              pInputCTB->cameraMetadata[i].activeArraySize.height,
              pInputCTB->cameraMetadata[i].fovRectIFE.top,
              pInputCTB->cameraMetadata[i].fovRectIFE.left,
              pInputCTB->cameraMetadata[i].fovRectIFE.width,
              pInputCTB->cameraMetadata[i].fovRectIFE.height);

        if (pInputCTB->cameraMetadata[i].cameraRole == CameraRoleTypeTele) {
            // float imageRatio = (float)mInputDataWidth / mInputDataHeight;
            uint32_t sensorW = pInputCTB->cameraMetadata[i].fovRectIFE.width;
            uint32_t sensorH = pInputCTB->cameraMetadata[i].fovRectIFE.height;
            if (true == pInputCTB->cameraMetadata[i].isQuadCFASensor) {
                sensorW /= 2;
                sensorH /= 2;
            }
            mFrameParams.caminfo.i32MainWidth_CropNoScale = sensorW;
            mFrameParams.caminfo.i32MainHeight_CropNoScale =
                sensorW * mInputDataHeight / mInputDataWidth;

            mFovRectIFEForTele = pInputCTB->cameraMetadata[i].fovRectIFE;
            mActiveArraySizeTele = pInputCTB->cameraMetadata[i].activeArraySize;

            focusROI = pInputCTB->cameraMetadata[i].afFocusROI;
            MIRECT rtFocusROI = {(uint32_t)focusROI.xMin, (uint32_t)focusROI.yMin,
                                 (uint32_t)(focusROI.xMax - focusROI.xMin + 1),
                                 (uint32_t)(focusROI.yMax - focusROI.yMin + 1)};
            memset(&mRtFocusROIOnInputData, 0, sizeof(MIRECT));
            mapRectToInputData(rtFocusROI, mActiveArraySizeTele, mInputDataWidth, mInputDataHeight,
                               mRtFocusROIOnInputData);

            mFrameParams.refocus.ptFocus.x =
                mRtFocusROIOnInputData.left + mRtFocusROIOnInputData.width / 2;
            mFrameParams.refocus.ptFocus.y =
                mRtFocusROIOnInputData.top + mRtFocusROIOnInputData.height / 2;

            if (mFrameParams.refocus.ptFocus.x == 0 && mFrameParams.refocus.ptFocus.y == 0) {
                mFrameParams.refocus.ptFocus.x = mFrameParams.mainimg.i32Width / 2;
                mFrameParams.refocus.ptFocus.y = mFrameParams.mainimg.i32Height / 2;
            }

            MLOGI(Mia2LogGroupPlugin,
                  "setFrameParams focus rect(l,t,w,h):[%d,%d,%d,%d] ptFocus:[%d,%d]", focusROI.xMin,
                  focusROI.yMin, focusROI.xMax, focusROI.yMax, mFrameParams.refocus.ptFocus.x,
                  mFrameParams.refocus.ptFocus.y);

            FDMetadataResults fdResult = {0};
            fdResult.numFaces = pInputCTB->cameraMetadata[0].fdMetadata.numFaces;
            MLOGD(Mia2LogGroupPlugin, "tele facenum = %d", fdResult.numFaces);

            if (fdResult.numFaces > FDMaxFaces)
                fdResult.numFaces = FDMaxFaces;

            for (int i = 0; i < fdResult.numFaces; i++) {
                int32_t xMin = pInputCTB->cameraMetadata[0].fdMetadata.faceRect[i].left;
                int32_t yMin = pInputCTB->cameraMetadata[0].fdMetadata.faceRect[i].top;
                int32_t xMax = pInputCTB->cameraMetadata[0].fdMetadata.faceRect[i].width;
                int32_t yMax = pInputCTB->cameraMetadata[0].fdMetadata.faceRect[i].height;
                MLOGI(Mia2LogGroupPlugin,
                      "get MTK_STATISTICS_FACE_RECTANGLES "
                      "[%d,%d,%d,%d]",
                      xMin, yMin, xMax, yMax);

                int faceWidth = xMax - xMin;
                int faceHeight = yMax - yMin;
                // convert face rectangle to be based on current input dimension
                fdResult.faceRect[i].left = xMin;
                fdResult.faceRect[i].top = yMin;
                fdResult.faceRect[i].width = xMax - xMin;
                fdResult.faceRect[i].height = yMax - yMin;
            }

            for (int nFaceIndex = 0; nFaceIndex < fdResult.numFaces; ++nFaceIndex) {
                MLOGD(Mia2LogGroupPlugin, " tele facerect[%d](l,t,w,h)[%d,%d,%d,%d]", nFaceIndex,
                      fdResult.faceRect[nFaceIndex].left, fdResult.faceRect[nFaceIndex].top,
                      fdResult.faceRect[nFaceIndex].width, fdResult.faceRect[nFaceIndex].height);
            }

            // for Face info
            convertToWaFaceInfo(fdResult, mBokehParam.faceParam);

            // aftype
            // 1 : Face AF
            // 0 : Focus AF
            if (pInputCTB->cameraMetadata[i].afType == 1) {
                mFrameParams.refocus.ptFocus.x = (mFaces[0].rect.left + mFaces[0].rect.right) / 2;
                mFrameParams.refocus.ptFocus.y = (mFaces[0].rect.top + mFaces[0].rect.bottom) / 2;
            } else {
                mFrameParams.refocus.ptFocus.x =
                    mRtFocusROIOnInputData.left + mRtFocusROIOnInputData.width / 2;
                mFrameParams.refocus.ptFocus.y =
                    mRtFocusROIOnInputData.top + mRtFocusROIOnInputData.height / 2;
            }
        } else {
            // float imageRatio = (float)mInputDataWidth / mInputDataHeight;
            int32_t sensorW = pInputCTB->cameraMetadata[i].fovRectIFE.width;
            int32_t sensorH = pInputCTB->cameraMetadata[i].fovRectIFE.height;
            if (true == pInputCTB->cameraMetadata[i].isQuadCFASensor) {
                sensorW /= 2;
                sensorH /= 2;
            }
            mFrameParams.caminfo.i32AuxWidth_CropNoScale = sensorW;
            mFrameParams.caminfo.i32AuxHeight_CropNoScale =
                sensorW * mInputDataHeight / mInputDataWidth;
            mFovRectIFEForWide = pInputCTB->cameraMetadata[i].fovRectIFE;
            mActiveArraySizeWide = pInputCTB->cameraMetadata[i].activeArraySize;
        }
    }

    MLOGD(Mia2LogGroupPlugin,
          "setFrameParams "
          "LeftFullWidth:%d,LeftFullHeight:%d,RightFullWidth:%d,RightFullHeight:%d",
          mFrameParams.caminfo.i32MainWidth_CropNoScale,
          mFrameParams.caminfo.i32MainHeight_CropNoScale,
          mFrameParams.caminfo.i32AuxWidth_CropNoScale,
          mFrameParams.caminfo.i32AuxHeight_CropNoScale);

#else
    MLOGD(Mia2LogGroupPlugin, " todo hard code for setFrameParams");
    // set crop size and af info
    mFrameParams.caminfo.i32MainWidth_CropNoScale = 4032;  /******hard code*******/
    mFrameParams.caminfo.i32MainHeight_CropNoScale = 3024; /******hard code*******/
    mFrameParams.caminfo.i32AuxWidth_CropNoScale = 4032;   /******hard code*******/
    mFrameParams.caminfo.i32AuxHeight_CropNoScale = 3024;  /******hard code*******/

    mFrameParams.refocus.ptFocus.x = mFrameParams.mainimg.i32Width / 2;  /******hard code*******/
    mFrameParams.refocus.ptFocus.y = mFrameParams.mainimg.i32Height / 2; /******hard code*******/

    mFrameParams.refocus.i32BlurIntensity = mBlurLevel;
#endif

    mBokehParam.fMaxFOV = 78.5;
    mBokehParam.i32ImgDegree = CAPTURE_BOKEH_DEGREE;
    /*int degree = mRearCamera ? CAPTURE_BOKEH_DEGREE_0 : CAPTURE_BOKEH_DEGREE_270;
#if defined(UMI_CAM) || defined(PICASSO_CAM) || defined(LMI_CAM)
    degree = mRearCamera ? CAPTURE_BOKEH_DEGREE_180: CAPTURE_BOKEH_DEGREE_270;
#elif defined(MONET_CAM)
    degree = mRearCamera ? CAPTURE_BOKEH_DEGREE_315 : CAPTURE_BOKEH_DEGREE_0;
#elif defined(VANGOGH_CAM)
    degree = mRearCamera ? CAPTURE_BOKEH_DEGREE_0 : CAPTURE_BOKEH_DEGREE_270;
#endif
    mBokehParam.i32ImgDegree = degree;*/

    pMetaData = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_JPEG_ORIENTATION, &pMetaData);
    if (NULL != pMetaData) {
        mOrient = *static_cast<int *>(pMetaData);
    }

    /*switch(mOrient){
        case 0:  mMappedOrient= ARC_PL_OPF_0;   break;
        case 90: mMappedOrient = ARC_PL_OPF_90;  break;
        case 180:mMappedOrient = ARC_PL_OPF_180; break;
        case 270:mMappedOrient = ARC_PL_OPF_270; break;
    }*/

    char tagStr[256] = {0};
    int beautyFeatureCounts = mBeautyFeatureParams.featureCounts;
    for (int i = 0; i < beautyFeatureCounts; i++) {
        if (mBeautyFeatureParams.beautyFeatureInfo[i].featureMask == true) {
            memcpy(tagStr, mBeautyFeatureParams.beautyFeatureInfo[i].TagName,
                   strlen(mBeautyFeatureParams.beautyFeatureInfo[i].TagName) + 1);

            pMetaData = NULL;
            VendorMetadataParser::getVTagValue(metaData, tagStr, &pMetaData);
            if (pMetaData != NULL) {
                mBeautyFeatureParams.beautyFeatureInfo[i].featureValue =
                    *static_cast<int *>(pMetaData);
                MLOGD(Mia2LogGroupPlugin, "Get Metadata Value TagName: %s, Value: %d", tagStr,
                      mBeautyFeatureParams.beautyFeatureInfo[i].featureValue);
                if (mBeautyFeatureParams.beautyFeatureInfo[i].featureValue !=
                    0) // feature vlude support -100 -- + 100, 0 means no effect
                {
                    mBeautyEnabled = true;
                }
            } else {
                MLOGD(Mia2LogGroupPlugin, "Get Metadata Failed! TagName: %s", tagStr);
            }
        }
    }

    int light_mode = 0;
    const char *BokehLight = "xiaomi.portrait.lighting";
    VendorMetadataParser::getVTagValue(metaData, BokehLight, &pMetaData);
    if (NULL != pMetaData) {
        light_mode = *static_cast<int *>(pMetaData);
        MLOGD(Mia2LogGroupPlugin, "getMetaData light mode: %d", light_mode);
    }
    switch (light_mode) {
    case 9:
        mLightingMode = MI_PL_MODE_NEON_SHADOW;
        break;
    case 10:
        mLightingMode = MI_PL_MODE_PHANTOM;
        break;
    case 11:
        mLightingMode = MI_PL_MODE_NOSTALGIA;
        break;
    case 12:
        mLightingMode = MI_PL_MODE_RAINBOW;
        break;
    case 13:
        mLightingMode = MI_PL_MODE_WANING;
        break;
    case 14:
        mLightingMode = MI_PL_MODE_DAZZLE;
        break;
    case 15:
        mLightingMode = MI_PL_MODE_GORGEOUS;
        break;
    case 16:
        mLightingMode = MI_PL_MODE_PURPLES;
        break;
    case 17:
        mLightingMode = MI_PL_MODE_DREAM;
        break;
    default:
        mLightingMode = 0;
        break;
    }

    pMetaData = NULL;
    const char *faceAnalyzeResult = "xiaomi.faceAnalyzeResult.result";
    VendorMetadataParser::getVTagValue(metaData, faceAnalyzeResult, &pMetaData);
    if (NULL != pMetaData) {
        memcpy(&mFaceAnalyze, pMetaData, sizeof(OutputMetadataFaceAnalyze));
        MLOGD(Mia2LogGroupPlugin, "getMetaData faceNum %d", mFaceAnalyze.faceNum);
    }

    pMetaData = NULL;
    const char *multiCameraRole = "com.xiaomi.multicamerainfo.MultiCameraIdRole";
    VendorMetadataParser::getVTagValue(metaData, multiCameraRole, &pMetaData);
    if (NULL != pMetaData) {
        MultiCameraIdRole inputMetadata = *static_cast<MultiCameraIdRole *>(pMetaData);
        MLOGD(Mia2LogGroupPlugin, "getMetaData MultiCameraIdRole %d",
              inputMetadata.currentCameraRole);
    }

    pMetaData = NULL;
    const char *aiAsdEnabled = "xiaomi.ai.asd.enabled";
    VendorMetadataParser::getVTagValue(metaData, aiAsdEnabled, &pMetaData);
    if (NULL != pMetaData) {
        mSceneEnable = *static_cast<int *>(pMetaData);
        MLOGD(Mia2LogGroupPlugin, "getMetaData mSceneEnable: %d", mSceneEnable);
    }

    if (mSceneEnable) {
        pMetaData = NULL;
        const char *aiAsdDetected = "xiaomi.ai.asd.sceneDetected";
        VendorMetadataParser::getVTagValue(metaData, aiAsdDetected, &pMetaData);
        if (NULL != pMetaData) {
            mSceneMode = *static_cast<int *>(pMetaData);
            MLOGD(Mia2LogGroupPlugin, "getMetaData mSceneMode: %d", mSceneMode);
        }
    } else {
        mSceneMode = 0;
        MLOGD(Mia2LogGroupPlugin, "set default mSceneMode: %d", mSceneMode);
    }

    pMetaData = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_SENSOR_SENSITIVITY, &pMetaData);
    if (NULL != pMetaData) {
        int iso = *static_cast<int *>(pMetaData);
        mFrameParams.isoValue = iso;
        MLOGD(Mia2LogGroupPlugin, "getMetaData iso: %d", iso);
    }
    // light spot parameters
    pMetaData = NULL;
    VendorMetadataParser::getTagValue(metaData, ANDROID_SENSOR_EXPOSURE_TIME, &pMetaData);
    if (NULL != pMetaData) {
        mFrameParams.expTime = *static_cast<int64_t *>(pMetaData) / 1000000000.0f;
        MLOGD(Mia2LogGroupPlugin, "getMetaData exposure time: %.6f s", mFrameParams.expTime);
    } else {
        mFrameParams.expTime = 0.0f;
        MLOGD(Mia2LogGroupPlugin, "getMetaData exposure time fail");
    }

    pMetaData = NULL;
    VendorMetadataParser::getVTagValue(metaData, "com.xiaomi.statsconfigs.AecInfo", &pMetaData);
    if (NULL != pMetaData) {
        mFrameParams.ispGain = static_cast<InputMetadataAecInfo *>(pMetaData)->linearGain;
        MLOGD(Mia2LogGroupPlugin, "getMetaData ispgain: %.6f", mFrameParams.ispGain);
    } else {
        mFrameParams.ispGain = 0.0f;
        MLOGD(Mia2LogGroupPlugin, "getMetaData ispgain fail");
    }

    return PROCSUCCESS;
}
void MiaNodeIntSenseCapBokeh::copyImage(WAOFFSCREEN *pSrcImg, WAOFFSCREEN *pDstImg)
{
    if (pSrcImg == NULL || pDstImg == NULL)
        return;

    int32_t i;
    uint8_t *l_pSrc = NULL;
    uint8_t *l_pDst = NULL;

    l_pSrc = pSrcImg->ppu8Plane[0];
    l_pDst = pDstImg->ppu8Plane[0];
    for (i = 0; i < pDstImg->i32Height; i++) {
        memcpy(l_pDst, l_pSrc, pDstImg->i32Width);
        l_pDst += pDstImg->pi32Pitch[0];
        l_pSrc += pSrcImg->pi32Pitch[0];
    }

    l_pSrc = pSrcImg->ppu8Plane[1];
    l_pDst = pDstImg->ppu8Plane[1];
    for (i = 0; i < pDstImg->i32Height / 2; i++) {
        memcpy(l_pDst, l_pSrc, pDstImg->i32Width);
        l_pDst += pDstImg->pi32Pitch[1];
        l_pSrc += pSrcImg->pi32Pitch[1];
    }
}

void MiaNodeIntSenseCapBokeh::mergeImage(WAOFFSCREEN *pBaseImg, WAOFFSCREEN *pOverImg, int downsize)
{
    if (pBaseImg == NULL || pOverImg == NULL || downsize <= 0)
        return;

    int32_t i, j, index;
    uint8_t *l_pBase = NULL;
    uint8_t *l_pOver = NULL;

    l_pBase = pBaseImg->ppu8Plane[0];
    l_pOver = pOverImg->ppu8Plane[0];
    for (i = 0; i < pOverImg->i32Height;) {
        for (j = 0, index = 0; j < pOverImg->i32Width; index++, j += downsize) {
            *(l_pBase + index) = *(l_pOver + j);
        }
        l_pBase += pBaseImg->pi32Pitch[0];
        l_pOver += pOverImg->pi32Pitch[0] * downsize;
        i += downsize;
    }

    l_pBase = pBaseImg->ppu8Plane[1];
    l_pOver = pOverImg->ppu8Plane[1];
    for (i = 0; i < pOverImg->i32Height / 2;) {
        for (j = 0, index = 0; j < pOverImg->i32Width / 2; index++, j += downsize) {
            *(l_pBase + 2 * index) = *(l_pOver + 2 * j);
            *(l_pBase + 2 * index + 1) = *(l_pOver + 2 * j + 1);
        }

        l_pBase += pBaseImg->pi32Pitch[1];
        l_pOver += pOverImg->pi32Pitch[1] * downsize;
        i += downsize;
    }
}

void MiaNodeIntSenseCapBokeh::convertToWaFaceInfo(FDMetadataResults &fdMeta,
                                                  WA_FACE_PARAM &faceinfo)
{
    if (faceinfo.prtFaces == NULL || faceinfo.pi32FaceAngles == NULL)
        return;
    faceinfo.i32FacesNum = fdMeta.numFaces;
    MIRECT rtOnInputData;

    for (int i = 0; i < fdMeta.numFaces && i < MAX_FACE_NUM; ++i) {
        memset(&rtOnInputData, 0, sizeof(MIRECT));
        mapFaceRectToInputData(fdMeta.faceRect[i], mActiveArraySizeTele, mInputDataWidth,
                               mInputDataHeight, rtOnInputData);

        faceinfo.prtFaces[i].left = rtOnInputData.left;
        faceinfo.prtFaces[i].top = rtOnInputData.top;
        faceinfo.prtFaces[i].right = rtOnInputData.left + rtOnInputData.width - 1;
        faceinfo.prtFaces[i].bottom = rtOnInputData.top + rtOnInputData.height - 1;

        mFaces[i].rect.left = rtOnInputData.left;
        mFaces[i].rect.top = rtOnInputData.top;
        mFaces[i].rect.right = rtOnInputData.left + rtOnInputData.width - 1;
        mFaces[i].rect.bottom = rtOnInputData.top + rtOnInputData.height - 1;

        MLOGD(Mia2LogGroupPlugin,
              " convertToWaFaceInfo facenum = %d,face[%d] = (l,t,w,h)[%d,%d,%d,%d]",
              faceinfo.i32FacesNum, i, fdMeta.faceRect[i].left, fdMeta.faceRect[i].top,
              fdMeta.faceRect[i].width, fdMeta.faceRect[i].height);
    }
}

void MiaNodeIntSenseCapBokeh::mapRectToInputData(MIRECT &rtSrcRect, MIRECT &rtIFERect,
                                                 int &nDataWidth, int &nDataHeight,
                                                 MIRECT &rtDstRectOnInputData)
{
    MLOGD(Mia2LogGroupPlugin,
          "rtSrcRect w:%d, h:%d, rtIFERect w:%d, h:%d, nDataWidth:%d, nDataHeight:%d",
          rtSrcRect.width, rtSrcRect.height, rtIFERect.width, rtIFERect.height, nDataWidth,
          nDataHeight);

    if (rtSrcRect.width <= 0 || rtSrcRect.height <= 0 || rtIFERect.width <= 0 ||
        rtIFERect.height <= 0 || nDataWidth <= 0 || nDataHeight <= 0) {
        MLOGE(Mia2LogGroupPlugin, "invalide width or height");
        return;
    }
    MIRECT rtSrcOnIFE = rtSrcRect;
    rtSrcOnIFE.left -= rtIFERect.left;
    if (rtSrcOnIFE.left < 0)
        rtSrcOnIFE.left = 0;
    rtSrcOnIFE.top -= rtIFERect.top;
    if (rtSrcOnIFE.top < 0)
        rtSrcOnIFE.top = 0;

    float fLRRatio = nDataWidth / (float)rtIFERect.width;
    // float fTBRatio = nDataHeight / (float)rtIFERect.height;
    int iTBOffset = (((int)rtIFERect.height) - (int)rtIFERect.width * nDataHeight / nDataWidth) / 2;
    if (iTBOffset < 0)
        iTBOffset = 0;

    rtDstRectOnInputData.left = round(rtSrcOnIFE.left * fLRRatio);
    if (int(rtSrcOnIFE.top) >= iTBOffset) {
        rtDstRectOnInputData.top = round((rtSrcOnIFE.top - iTBOffset) * fLRRatio);
    } else {
        MLOGE(Mia2LogGroupPlugin, "mapRectToInputData focus point ERROR !!!");
        rtDstRectOnInputData.top = 0;
    }
    rtDstRectOnInputData.width = floor(rtSrcOnIFE.width * fLRRatio);
    rtDstRectOnInputData.height = floor(rtSrcOnIFE.height * fLRRatio);
    rtDstRectOnInputData.width &= 0xFFFFFFFE;
    rtDstRectOnInputData.height &= 0xFFFFFFFE;

    MLOGD(Mia2LogGroupPlugin,
          "mapRectToInputData rtSrcRect(l,t,w,h)= [%d,%d,%d,%d], rtIFERect(l,t,w,h)= "
          "[%d,%d,%d,%d], rtDstRectOnInputData[%d,%d,%d,%d]",
          rtSrcRect.left, rtSrcRect.top, rtSrcRect.width, rtSrcRect.height, rtIFERect.left,
          rtIFERect.top, rtIFERect.width, rtIFERect.height, rtDstRectOnInputData.left,
          rtDstRectOnInputData.top, rtDstRectOnInputData.width, rtDstRectOnInputData.height);
}
void MiaNodeIntSenseCapBokeh::mapFaceRectToInputData(MIRECT &rtSrcRect, MIRECT &rtIFERect,
                                                     int &nDataWidth, int &nDataHeight,
                                                     MIRECT &rtDstRectOnInputData)
{
    MLOGD(Mia2LogGroupPlugin,
          "rtSrcRect w:%d, h:%d, rtIFERect w:%d, h:%d, nDataWidth:%d, nDataHeight:%d",
          rtSrcRect.width, rtSrcRect.height, rtIFERect.width, rtIFERect.height, nDataWidth,
          nDataHeight);

    if (rtSrcRect.width <= 0 || rtSrcRect.height <= 0 || rtIFERect.width <= 0 ||
        rtIFERect.height <= 0 || nDataWidth <= 0 || nDataHeight <= 0) {
        MLOGE(Mia2LogGroupPlugin, "invalide width or height");
        return;
    }
    MIRECT rtSrcOnIFE = rtSrcRect;
    // rtSrcOnIFE.left -= rtIFERect.left;
    if (rtSrcOnIFE.left < 0)
        rtSrcOnIFE.left = 0;
    // rtSrcOnIFE.top -= rtIFERect.top;
    if (rtSrcOnIFE.top < 0)
        rtSrcOnIFE.top = 0;

    float fLRRatio = nDataWidth / (float)rtIFERect.width;
    // float fTBRatio = nDataHeight / (float)rtIFERect.height;
    int iTBOffset = (((int)rtIFERect.height) - (int)rtIFERect.width * nDataHeight / nDataWidth) / 2;
    if (iTBOffset < 0)
        iTBOffset = 0;

    rtDstRectOnInputData.left = round(rtSrcOnIFE.left * fLRRatio);
    if (int(rtSrcOnIFE.top) >= iTBOffset) {
        rtDstRectOnInputData.top = round((rtSrcOnIFE.top - iTBOffset) * fLRRatio);
    } else {
        MLOGE(Mia2LogGroupPlugin, "mapRectToInputData focus point ERROR !!!");
        rtDstRectOnInputData.top = 0;
    }
    rtDstRectOnInputData.width = floor(rtSrcOnIFE.width * fLRRatio);
    rtDstRectOnInputData.height = floor(rtSrcOnIFE.height * fLRRatio);
    rtDstRectOnInputData.width &= 0xFFFFFFFE;
    rtDstRectOnInputData.height &= 0xFFFFFFFE;

    MLOGD(Mia2LogGroupPlugin,
          "mapRectToInputData rtSrcRect(l,t,w,h)= [%d,%d,%d,%d], rtIFERect(l,t,w,h)= "
          "[%d,%d,%d,%d], rtDstRectOnInputData[%d,%d,%d,%d]",
          rtSrcRect.left, rtSrcRect.top, rtSrcRect.width, rtSrcRect.height, rtIFERect.left,
          rtIFERect.top, rtIFERect.width, rtIFERect.height, rtDstRectOnInputData.left,
          rtDstRectOnInputData.top, rtDstRectOnInputData.width, rtDstRectOnInputData.height);
}

int MiaNodeIntSenseCapBokeh::doProcess()
{
    MLOGI(Mia2LogGroupPlugin, " IN");
    int result = 1;
    int ret = MIA_RETURN_OK;
    float waFNumber = 0.0f;
    WAOFFSCREEN &stWideImage = mFrameParams.auximage;
    WAOFFSCREEN &stTeleImage = mFrameParams.mainimg;
    WAOFFSCREEN &stDestImage = mFrameParams.outimage;
    WAOFFSCREEN &stDepthImage = mFrameParams.depthimage;
    WAOFFSCREEN &stDestTeleImage = mFrameParams.outteleimage;

    double t0, t1, t2, t3;
    t0 = t1 = t2 = t3 = PluginUtils::nowMSec();

    uint8_t *disparityData = NULL;
    uint8_t *maskBuffer = NULL;
    WAOFFSCREEN tmpTeleImage;
    WAOFFSCREEN tmpArlDstImage;
    memset(&tmpTeleImage, 0, sizeof(WAOFFSCREEN));
    memset(&tmpArlDstImage, 0, sizeof(WAOFFSCREEN));
    copyWaImage(&tmpTeleImage, &stTeleImage);
    copyWaImage(&tmpArlDstImage, &stTeleImage);

    pthread_t tidRelighting;
    void *tidRelightingExitStatus = NULL;
    bool relighting_joinable = false;
    pthread_t tidBeauty;
    bool beautyJoinable = false;
    void *tidBeautyExitStatus = NULL;

    std::unique_lock<std::mutex> lock(mtx);

    do {
        MLOGD(Mia2LogGroupPlugin,
              " stTeleImage plane:%p, %p, pitch:%d, %d  aWidth = %d, aHeight = %d i32Width "
              "= %d, i32Height = %d format = %d",
              stTeleImage.ppu8Plane[0], stTeleImage.ppu8Plane[1], stTeleImage.pi32Pitch[0],
              stTeleImage.pi32Pitch[1], stTeleImage.aWidth, stTeleImage.aHeight,
              stTeleImage.i32Width, stTeleImage.i32Height, stTeleImage.u32PixelArrayFormat);
        MLOGD(Mia2LogGroupPlugin,
              " stWideImage plane:%p, %p, pitch:%d, %d aWidth = %d, aHeight = %d i32Width "
              "= %d, i32Height = %d format = %d",
              stWideImage.ppu8Plane[0], stWideImage.ppu8Plane[1], stWideImage.pi32Pitch[0],
              stWideImage.pi32Pitch[1], stWideImage.aWidth, stWideImage.aHeight,
              stWideImage.i32Width, stWideImage.i32Height, stWideImage.u32PixelArrayFormat);
        MLOGD(Mia2LogGroupPlugin,
              " stDestImage plane:%p, %p, pitch:%d, %d  aWidth = %d, aHeight = %d i32Width "
              "= %d, i32Height = %d format = %d",
              stDestImage.ppu8Plane[0], stDestImage.ppu8Plane[1], stDestImage.pi32Pitch[0],
              stDestImage.pi32Pitch[1], stDestImage.aWidth, stDestImage.aHeight,
              stDestImage.i32Width, stDestImage.i32Height, stDestImage.u32PixelArrayFormat);

        t1 = PluginUtils::nowMSec();

        // update outImage and outTeleImage with teleImage(main)
        copyImage(&stTeleImage, &stDestImage);
        copyImage(&stTeleImage, &stDestTeleImage);

        if (mDepth == NULL || mRefocus == NULL ||
            (mIsNonCalibAlgo == 0 &&
             (mMecpDataSz == 0 || mMecpData == NULL || mCaliDataSz == 0 || mCalibration == NULL)) ||
            (mIsNonCalibAlgo == 1 && (mMecpDataSz == 0 || mMecpData == NULL))) {
            ret = MIA_RETURN_INVALID_PARAM;
            MLOGE(Mia2LogGroupPlugin, "MecpData or CaliData is null, Exit!!!");
            break;
        }

        // do MI bokeh_relighting_thread_proc_init
        RelightThreadParams rtp = {};
        if (mLightingMode > 0 && mRelightingHandle == NULL) {
            rtp.i32Width = stTeleImage.i32Width;
            rtp.i32Height = stTeleImage.i32Height;
            rtp.mRelightingHandle = &mRelightingHandle;
            MLOGD(Mia2LogGroupPlugin,
                  "before init, give :width %d height %d, m_phRelightingHandle:%p, "
                  "m_bRearCamera:%d",
                  rtp.i32Width, rtp.i32Height, *rtp.mRelightingHandle, mRearCamera);
            pthread_create(&tidRelighting, NULL, bokehRelightingThreadProcInit, &rtp);
            relighting_joinable = true;
            MLOGI(Mia2LogGroupPlugin, "run bokehRelightingThreadProcInit relighting_joinable:%d ",
                  relighting_joinable);
        }

        // do ARCSOFT beauty , inplace
        ASVLOFFSCREEN tmpBeautyImage = {};
        BokehFaceBeautyParams params;
        if (mBeautyEnabled > 0) { //
            prepareArcImage(tmpBeautyImage, tmpTeleImage);
            params.pImage = &tmpBeautyImage;
            MLOGD(Mia2LogGroupPlugin,
                  "params.pImage plane:%p, %p, pitch:%d, %d i32Width = %d, i32Height = "
                  "%d,format = %d",
                  params.pImage->ppu8Plane[0], params.pImage->ppu8Plane[1],
                  params.pImage->pi32Pitch[0], params.pImage->pi32Pitch[1], params.pImage->i32Width,
                  params.pImage->i32Height, params.pImage->u32PixelArrayFormat);

            params.orient = mFaceOrientation;
            params.rearCamera = mRearCamera;
            memcpy(&params.beautyFeatureParams.beautyFeatureInfo[0],
                   &mBeautyFeatureParams.beautyFeatureInfo[0],
                   sizeof(mBeautyFeatureParams.beautyFeatureInfo));
            pthread_create(&tidBeauty, NULL, bokehFaceBeautyThreadProc, &params);
            beautyJoinable = true;
            MLOGD(Mia2LogGroupPlugin, "run bokeh_facebeauty_thread_proc beautyJoinable: %d",
                  beautyJoinable);
        }

        Image maskImage;
        Image mainImage;
        Image tmpMainImage;
        Image auxImage;
        Image bokehImage;
        Image depthImage;
        // get depth
        mainImage = Image(stTeleImage.pi32Pitch[0], stTeleImage.i32Height, stTeleImage.ppu8Plane[0],
                          stTeleImage.ppu8Plane[1], NULL, stTeleImage.u32PixelArrayFormat,
                          Image::DT_8U, wa::Image::ROTATION_0);
        mainImage.setImageWH(stTeleImage.i32Width, stTeleImage.i32Height);
        mainImage.setExifModuleInfo(mModuleInfo);

        auxImage = Image(stWideImage.pi32Pitch[0], stWideImage.i32Height, stWideImage.ppu8Plane[0],
                         stWideImage.ppu8Plane[1], NULL, stWideImage.u32PixelArrayFormat,
                         Image::DT_8U, wa::Image::ROTATION_0);
        auxImage.setImageWH(stWideImage.i32Width, stWideImage.i32Height);
        auxImage.setExifModuleInfo(mModuleInfo);
        mDepth->setImages(mainImage, auxImage);

        int depthWidth = mDepth->getWidthOfDepth();
        int depthHeight = mDepth->getHeightOfDepth();
        int elemSize = mDepth->getElemSizeOfDepth();
        int depthSize = 0;
        depthSize = depthWidth * depthHeight * elemSize * sizeof(uint8_t);

        if (depthSize > 0 && NULL == disparityData) {
            disparityData = reinterpret_cast<uint8_t *>(malloc(depthSize));
            MLOGD(Mia2LogGroupPlugin, " malloc %d buffer %p ", depthSize, disparityData);
        }
        if (NULL == disparityData) {
            ret = MIA_RETURN_NO_MEM;
            MLOGE(Mia2LogGroupPlugin, " m_pDisparityData == NULL");
            break;
        }

        wa::Image::DataType dataType = wa::Image::DT_32F;
        if (1 == elemSize) {
            dataType = wa::Image::DT_8U;
        } else if (2 == elemSize) {
            dataType = wa::Image::DT_16U;
        }
        depthImage = wa::Image(depthWidth, depthHeight, disparityData, wa::Image::IMAGETYPE_GRAY,
                               dataType, wa::Image::ROTATION_0);
        MLOGD(Mia2LogGroupPlugin,
              "disparityData:%p size: %d depth_WxH: %d * %d, dt: %d , elemSize: %d"
              " stDepthImage_PitchxHeight: %d * %d",
              disparityData, depthSize, depthWidth, depthHeight, dataType, elemSize,
              stDepthImage.pi32Pitch[0], stDepthImage.i32Height);

        int facesNum = mBokehParam.faceParam.i32FacesNum;
        if (facesNum == 0) {
            result = mDepth->setFaces(NULL, 0, (Image::Rotation)mFrameParams.i32Orientation);
        } else {
            result =
                mDepth->setFaces(mFaces, facesNum, (Image::Rotation)mFrameParams.i32Orientation);
        }
        if (result != 1) {
            ret = MIA_RETURN_INVALID_PARAM;
            MLOGE(Mia2LogGroupPlugin, "depth setFaces Failed: %d !!!", result);
            // break;
        }

        t2 = PluginUtils::nowMSec();
        result = mDepth->run(depthImage);
        t3 = PluginUtils::nowMSec();
        MLOGI(Mia2LogGroupPlugin, "mDepth run costtime = %.2f X", t3 - t2);
        if (result != 1) {
            ret = MIA_RETURN_UNKNOWN_ERROR;
            MLOGE(Mia2LogGroupPlugin, "depth run Failed result: %d !!!", result);
            // break;
        }
        MLOGD(Mia2LogGroupPlugin,
              "depthImage width: %d, height: %d, realWidth: %d, realHight: %d: total: "
              "%lu, depth size: %d",
              depthImage.width(), depthImage.height(), depthImage.realWidth(),
              depthImage.realHeight(), depthImage.total(), depthSize);

        if (mDumpPriv) {
            dumpRawData(disparityData, depthSize, 5, "OUT_disparity_", mDumpTimeStamp);
        }

        // get mask
        bool isValidSize = (depthWidth > 0 && depthHeight > 0) ? true : false;
        int maskWidth = isValidSize ? depthWidth : stTeleImage.i32Width;
        int maskHeight = isValidSize ? depthHeight : stTeleImage.i32Height;
        int maskSize = 0;
        maskSize = maskWidth * maskHeight * sizeof(uint8_t);

        if (maskSize > 0 && maskBuffer == NULL) {
            maskBuffer = reinterpret_cast<uint8_t *>(malloc(maskSize));
        }
        if (NULL == maskBuffer) {
            ret = MIA_RETURN_NO_MEM;
            MLOGE(Mia2LogGroupPlugin, "pMaskBuffer malloc Failed, Exit!!!");
            break;
        } else {
            maskImage = Image(maskWidth, maskHeight, maskBuffer, wa::Image::IMAGETYPE_GRAY,
                              wa::Image::DT_8U);
            mDepth->getMask(maskImage);

            if (mDumpPriv) {
                dumpRawData(maskBuffer, maskSize, 4, "AI_Mask", mDumpTimeStamp);
            }
            MLOGD(Mia2LogGroupPlugin, " get mask size: %dx%d, total: %lu, scene mode: %d",
                  maskImage.realWidth(), maskImage.realHeight(), maskImage.total(), mSceneMode);
        }

        if (beautyJoinable) {
            pthread_join(tidBeauty, &tidBeautyExitStatus);
            copyImage(&tmpTeleImage, &stDestTeleImage);
            beautyJoinable = false;
        }
        // do MI Ai filter
        if (mSceneEnable && mSceneMode != 0) {
            MLOGD(Mia2LogGroupPlugin, "ProcessAIFilter E");
            processAIFilter(tmpTeleImage, maskImage); //[in] tmpTeleImage with beauty effect
            MLOGD(Mia2LogGroupPlugin, "ProcessAIFilter X");
        }

        SAFE_FREE(maskBuffer);

        // do relighting preprocess
        LocalThreadParams threadParam;
        memset(&threadParam, 0, sizeof(threadParam));
        if (mLightingMode > 0) { // do nothing in bokehRelightingThreadProcPreprocess
            MIIMAGEBUFFER pSrcImg;
            MIIMAGEBUFFER pDstImg;
            MIIMAGEBUFFER pBokehImg;
            memset(&pSrcImg, 0, sizeof(MIIMAGEBUFFER));
            memset(&pDstImg, 0, sizeof(MIIMAGEBUFFER));
            memset(&pBokehImg, 0, sizeof(MIIMAGEBUFFER));
            prepareRelightImage(pSrcImg, tmpTeleImage);   // [in] The input image data
            prepareRelightImage(pDstImg, tmpArlDstImage); // [in] The output image data
            threadParam.pSrcImg = pSrcImg;
            threadParam.pDstImg = pDstImg;
            threadParam.orient = mFaceOrientation;
            threadParam.mode = mLightingMode;
            threadParam.pBokehImg = pBokehImg;
            if (relighting_joinable) {
                pthread_join(tidRelighting, &tidRelightingExitStatus);
                MLOGI(Mia2LogGroupPlugin,
                      "after init done , got m_hRelightingHandle:%p, m_bRearCamera:%d",
                      mRelightingHandle, mRearCamera);
                relighting_joinable = false;
                if (mRelightingHandle) {
                    threadParam.mRelightingHandle = &mRelightingHandle;
                    pthread_create(&tidRelighting, NULL, bokehRelightingThreadProcPreprocess,
                                   &threadParam);
                    relighting_joinable = true;
                }
            }
        }

        if (mSceneMode != 0 || mBeautyEnabled) {
            copyImage(&tmpTeleImage, &stDestImage);
            MLOGD(Mia2LogGroupPlugin, "output to stDestImage with beauty and filter ");
        }

        if (ret == MIA_RETURN_OK) { // depth run successfull and no memory issue
            // do bokeh
            tmpMainImage =
                Image(tmpTeleImage.pi32Pitch[0], tmpTeleImage.i32Height, tmpTeleImage.ppu8Plane[0],
                      tmpTeleImage.ppu8Plane[1], NULL, tmpTeleImage.u32PixelArrayFormat,
                      Image::DT_8U, wa::Image::ROTATION_0);
            tmpMainImage.setImageWH(tmpTeleImage.i32Width, tmpTeleImage.i32Height);
            // light spot parameters
            tmpMainImage.setExifIso(mFrameParams.isoValue);
            tmpMainImage.setExifExpTime(mFrameParams.expTime);
            tmpMainImage.setExifAeGain(mFrameParams.ispGain);
            mRefocus->setImages(tmpMainImage, depthImage);

            bokehImage =
                Image(stDestImage.pi32Pitch[0], stDestImage.i32Height, stDestImage.ppu8Plane[0],
                      stDestImage.ppu8Plane[1], NULL, stDestImage.u32PixelArrayFormat, Image::DT_8U,
                      wa::Image::ROTATION_0);
            bokehImage.setImageWH(stDestImage.i32Width, stDestImage.i32Height);

            if (facesNum == 0) {
                result = mRefocus->setFaces(NULL, 0, (Image::Rotation)mFrameParams.i32Orientation);
            } else {
                result = mRefocus->setFaces(mFaces, facesNum,
                                            (Image::Rotation)mFrameParams.i32Orientation);
            }
            if (result != 1) {
                ret = MIA_RETURN_INVALID_PARAM;
                MLOGE(Mia2LogGroupPlugin, "depth setFaces Failed: %d, Exit!!!", result);
                // break;
            }

            waFNumber = mFNum;
            MLOGD(Mia2LogGroupPlugin, "mRefocus run param focusX:%d focusY:%d,m_fNumber=%f",
                  mFrameParams.refocus.ptFocus.x, mFrameParams.refocus.ptFocus.y, waFNumber);

            t2 = PluginUtils::nowMSec();
            result = mRefocus->run(mFrameParams.refocus.ptFocus.x, mFrameParams.refocus.ptFocus.y,
                                   waFNumber, bokehImage);
            t3 = PluginUtils::nowMSec();
            MLOGD(Mia2LogGroupPlugin, "mRefocus run costtime = %.2f,ret = %d X", t3 - t2, result);
            if (result != 1) {
                ret = MIA_RETURN_UNKNOWN_ERROR;
                copyImage(&tmpTeleImage, &stDestImage);
                // break;
            } else {
                MLOGD(Mia2LogGroupPlugin, "mRefocus run succ, output depthimage");

                DisparityFileHeader header;
                memset(&header, 0, sizeof(DisparityFileHeader));
                header.i32Header = 0x80;
                header.i32HeaderLength = sizeof(DisparityFileHeader);
                header.i32PointX = mFrameParams.refocus.ptFocus.x;
                header.i32PointY = mFrameParams.refocus.ptFocus.y;
                header.i32BlurLevel = (int32_t)(waFNumber * 10);
                header.i32DataLength = depthSize;
                header.reserved[0] = depthWidth;
                header.reserved[1] = depthHeight;
                memcpy(stDepthImage.ppu8Plane[0], &header, sizeof(DisparityFileHeader));
                MInt32 planeSize = stDepthImage.i32Height * stDepthImage.pi32Pitch[0] -
                                   sizeof(DisparityFileHeader);
                if (depthSize > planeSize || depthImage.total() > planeSize) {
                    MLOGE(Mia2LogGroupPlugin, "disparity size =%d plane_size %d Failed, Exit!!!",
                          depthSize, planeSize);
                    ret = MIA_RETURN_INVALID_PARAM;
                    break;
                }
                memcpy(stDepthImage.ppu8Plane[0] + sizeof(DisparityFileHeader), disparityData,
                       depthSize);
            }
        }

        SAFE_FREE(disparityData);

        // do MI relight
        if (relighting_joinable) {
            pthread_join(tidRelighting, &tidRelightingExitStatus);
            relighting_joinable = false;
        }
        if (mRelightingHandle && mLightingMode > 0) {
            if (result == 1) {
                copyImage(&stDestImage, &tmpTeleImage);
            }
            LPWAOFFSCREEN tmpBokehImg = &tmpTeleImage;
            result = processRelighting(tmpTeleImage, tmpArlDstImage, *tmpBokehImg,
                                       NULL);             // relighting full image,  mask is unused
            if (result == 0) {                            // success
                copyImage(&tmpArlDstImage, &stDestImage); // update outimage with beauty, bokeh,
                                                          // filter and relighting effect
            }
        }
    } while (false);

    if (relighting_joinable) {
        pthread_join(tidRelighting, &tidRelightingExitStatus);
        relighting_joinable = false;
        MLOGD(Mia2LogGroupPlugin, "destory tidRelighting");
        if (mRelightingHandle != NULL) {
            AILAB_PLI_Uninit(&mRelightingHandle);
            mRelightingHandle = NULL;
            MLOGD(Mia2LogGroupPlugin, "destory mRelightingHandle");
        }
    }
    if (beautyJoinable) {
        pthread_join(tidBeauty, &tidBeautyExitStatus);
        beautyJoinable = false;
        copyImage(&tmpTeleImage, &stDestTeleImage);
        MLOGD(Mia2LogGroupPlugin, "destory tidBeauty");
    }
    releaseImage(&tmpTeleImage);
    releaseImage(&tmpArlDstImage);
    SAFE_FREE(disparityData);
    SAFE_FREE(maskBuffer);

    mTotalCost = PluginUtils::nowMSec() - t0;
    MLOGI(Mia2LogGroupPlugin, "Total costtime = %.2f X", mTotalCost);

    return ret;
}

bool MiaNodeIntSenseCapBokeh::readFile(const char *path, unsigned char **data, unsigned long *size)
{
    MLOGI(Mia2LogGroupPlugin, "ReadFile file %s .", path);

    if (NULL == path || NULL == size) {
        MLOGE(Mia2LogGroupPlugin, "ReadFile ERROR: invalid parameters!\n");
        return false;
    }

    FILE *pfile = fopen(path, "rb");
    if (NULL == pfile) {
        MLOGE(Mia2LogGroupPlugin, "ReadFile ERROR: open file: %s failed, detail: %s\n", path,
              strerror(errno));
        return false;
    }

    if (NULL == *data) {
        // CREATE NEW BUFFER !!!
        fseek(pfile, 0, SEEK_END);
        *size = ftell(pfile);
        if (*size <= 0) {
            MLOGE(Mia2LogGroupPlugin, "ReadFile ERROR: invalid file length(%lu)", *size);
            fclose(pfile);
            return false;
        }
        *data = reinterpret_cast<unsigned char *>(malloc(*size));
    }

    fseek(pfile, 0, SEEK_SET);
    const size_t read_size = fread(*data, 1, *size, pfile);
    if (read_size != static_cast<size_t>(*size)) {
        MLOGE(Mia2LogGroupPlugin,
              "ReadFile ERROR: readed size(%zu) and define size(%lu) does not match", read_size,
              *size);
        fclose(pfile);
        return false;
    } else {
        MLOGE(Mia2LogGroupPlugin, "ReadFile readed size(%zu) and define size:%lu ", read_size,
              *size);
    }

    fclose(pfile);
    return true;
}

void MiaNodeIntSenseCapBokeh::copyWaImage(WAOFFSCREEN *pDstImage, WAOFFSCREEN *pSrcImage)
{
    memcpy(pDstImage, pSrcImage, sizeof(WAOFFSCREEN));

    pDstImage->ppu8Plane[0] =
        (unsigned char *)malloc(pDstImage->i32Height * pDstImage->pi32Pitch[0] * 3 / 2);
    if (pDstImage->ppu8Plane[0]) {
        pDstImage->ppu8Plane[1] =
            pDstImage->ppu8Plane[0] + pDstImage->i32Height * pDstImage->pi32Pitch[0];
        copyImage(pSrcImage, pDstImage); // copy i32width*i32hight
    } else {
        MLOGE(Mia2LogGroupPlugin, " allocCopyImage  failed X");
    }
}

void MiaNodeIntSenseCapBokeh::releaseImage(WAOFFSCREEN *pImage)
{
    if (pImage->ppu8Plane[0])
        free(pImage->ppu8Plane[0]);
    pImage->ppu8Plane[0] = NULL;
}

void MiaNodeIntSenseCapBokeh::dumpRawData(uint8_t *pData, int32_t size, uint32_t index,
                                          const char *namePrefix, long long timestamp)
{
    MLOGD(Mia2LogGroupPlugin, "  IN");

    char filename[256];
    memset(filename, 0, sizeof(char) * 256);
    snprintf(filename, sizeof(filename), "%s%llu_%s_%d_%d.pdata", PluginUtils::getDumpPath(),
             timestamp, namePrefix, size, index);

    MLOGD(Mia2LogGroupPlugin, " try to open file : %s", filename);

    int file_fd = open(filename, O_RDWR | O_CREAT, 0777);
    if (file_fd) {
        MLOGD(Mia2LogGroupPlugin, " dumpYUVData open success");
        uint32_t bytes_write;
        bytes_write = write(file_fd, pData, size);
        MLOGD(Mia2LogGroupPlugin, " pData bytes_read: %d", size);
        close(file_fd);
    } else {
        MLOGE(Mia2LogGroupPlugin, " fileopen  error: %d", errno);
    }

    MLOGD(Mia2LogGroupPlugin, " OUT");
}

bool MiaNodeIntSenseCapBokeh::copyFile(const char *src, const char *dest)
{
    std::ifstream fin(src, std::ios::in | std::ios::binary);
    if (!fin) {
        MLOGD(Mia2LogGroupPlugin, " open in file[%s] failed", src);
        return false;
    }
    std::ofstream fout(dest, std::ios::out | std::ios::binary);
    if (!fout) {
        MLOGD(Mia2LogGroupPlugin, " open out file[%s] failed, errno: %d", dest, errno);
        return false;
    }
    fout << fin.rdbuf();
    return true;
}

void MiaNodeIntSenseCapBokeh::prepareRelightImage(MIIMAGEBUFFER &dstimage, WAOFFSCREEN &srtImage)
{
    dstimage.i32Width = srtImage.i32Width;
    dstimage.i32Height = srtImage.i32Height;
    dstimage.i32Scanline = srtImage.i32Height;

    if (srtImage.u32PixelArrayFormat == wa::Image::IMAGETYPE_NV12) {
        dstimage.u32PixelArrayFormat = FORMAT_YUV_420_NV12;
    } else if (srtImage.u32PixelArrayFormat == wa::Image::IMAGETYPE_NV21) {
        dstimage.u32PixelArrayFormat = FORMAT_YUV_420_NV21;
    }

    dstimage.ppu8Plane[0] = srtImage.ppu8Plane[0];
    dstimage.ppu8Plane[1] = srtImage.ppu8Plane[1];
    dstimage.pi32Pitch[0] = srtImage.pi32Pitch[0];
    dstimage.pi32Pitch[1] = srtImage.pi32Pitch[1];
}
