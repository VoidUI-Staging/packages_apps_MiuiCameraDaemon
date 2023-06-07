#include "mianchorsync.h"

void LumaScreen::calculateVarAndIsolatedPoint(std::vector<float> &luma, std::vector<int> &indexs,
                                              float &var, int &isolatedIndex)
{
    float sum = 0, squaresum = 0;
    int lens = indexs.size();
    std::for_each(indexs.begin(), indexs.end(), [&](int i) {
        sum += luma[i];
        squaresum += luma[i] * luma[i];
    });
    float mean = sum / lens;
    var = (squaresum - sum * mean) / lens;
    isolatedIndex = 0;
    int i = indexs[0];
    float sumWithoutLeft = sum - luma[i];
    float squaresumWithoutLeft = squaresum - luma[i] * luma[i];
    float varWithoutLeft =
        (squaresumWithoutLeft - sumWithoutLeft * sumWithoutLeft / (lens - 1)) / (lens - 1);
    i = indexs[lens - 1];
    float sumWithoutRight = sum - luma[i];
    float squaresumWithoutRight = squaresum - luma[i] * luma[i];
    float varWithoutRight =
        (squaresumWithoutRight - sumWithoutRight * sumWithoutRight / (lens - 1)) / (lens - 1);
    isolatedIndex = varWithoutLeft < varWithoutRight ? 0 : lens - 1;
}

std::vector<int> LumaScreen::operator()(std::vector<float> &luma)
{
    std::vector<int> indexs(luma.size());
    for (int i = 0; i < indexs.size(); i++) {
        indexs[i] = i;
    }

    std::sort(indexs.begin(), indexs.end(), [&](int a, int b) { return luma[a] > luma[b]; });

    for (int i = luma.size(); i > minFrame; i--) {
        float var;
        int isolatedIndex;
        calculateVarAndIsolatedPoint(luma, indexs, var, isolatedIndex);

        if (var < VarThreshold) {
            return indexs;
        } else {
            indexs.erase(indexs.begin() + isolatedIndex);
        }
    }

    return indexs;
}

SharpnessMiAlgo *SharpnessMiAlgo::GetInstance()
{
    static SharpnessMiAlgo *pSharpnessMiAlgo = new SharpnessMiAlgo;
    return pSharpnessMiAlgo;
}

SharpnessMiAlgo::SharpnessMiAlgo()
{
    mRefCount = 0;
    mFlag = 0;
    mBufSize = 0;
    mState = Uninit;
    mHandle = NULL;
    mAligned[0] = 0;
    mAligned[1] = 0;
    mAligned[2] = 16;
    memset(&mConfig, 0, sizeof(mConfig));
}
int SharpnessMiAlgo::AddRef()
{
    std::unique_lock<std::mutex> lock(mAlgoMutex);
    mRefCount++;
    MLOGD(Mia2LogGroupPlugin, "mRefCount %d state %d", mRefCount, mState);
    if (mRefCount == 1) {
        mState = Initing;
        std::thread Initlib([this] {
            double InitlibStartTime = PluginUtils::nowMSec();
            this->AnchorInitAlgoLib();
            MLOGD(Mia2LogGroupPlugin, "anchor init costTime=%.2f",
                  PluginUtils::nowMSec() - InitlibStartTime);
        });
        Initlib.detach();
    }
    return 0;
}
void SharpnessMiAlgo::AnchorInitAlgoLib()
{
    MLOGD(Mia2LogGroupPlugin, "anchorsync init begin");
    MI_S32 ret = MIALGO_OK;
    MialgoRFSInitParam init_param;
    init_param.num_threads = 4;
    init_param.target =
        static_cast<MialgoRFSTargetType>(property_get_int32("persist.vendor.camera.anchoralgo", 2));
    init_param.device = MIALGO_RFS_DEVICE_NONE;

    MialgoEngine algo = NULL;
    const MI_CHAR *version = NULL;

    // init basic
    MialgoBasicInit basic_init;
    memset(&basic_init, 0, sizeof(basic_init));

    basic_init.log = MIALGO_LOG_LV_DEBUG;
    basic_init.num_threads = init_param.num_threads;
    basic_init.cdsp_param.enable = 1;

    if ((ret = MialgoBasicLibInit(&basic_init)) != MIALGO_OK) {
        MLOGI(Mia2LogGroupPlugin, "error(%d) error_str(%s)\n", ret, MialgoErrorStr(ret));
    } else if ((version = MialgoBasicLibGetVer()) != NULL) {
        MLOGI(Mia2LogGroupPlugin, "mialgo basic lib : version(%s)\n", version);
    }

    if ((algo = MialgoRFSInit(&init_param, &ret)) == NULL) {
        MLOGI(Mia2LogGroupPlugin, "error(%d) error_str(%s)\n", ret, MialgoErrorStr(ret));
    } else if ((version = MialgoRFSGetVer(algo)) != NULL) {
        MLOGI(Mia2LogGroupPlugin, "mialgo rfs lib : version(%s)\n", version);
    }
    MialgoSetLogOutput(MIALGO_LOG_OUTPUT_LOGCAT);
    if (MIALGO_OK == ret) {
        std::unique_lock<std::mutex> lock(mAlgoMutex);
        mHandle = algo;
        mState = Idle;
        mWaitAlgoCond.notify_all();
    } else {
        std::unique_lock<std::mutex> lock(mAlgoMutex);
        mState = Uninit;
        mRefCount = 0;
        mWaitAlgoCond.notify_all();
    }
    MLOGD(Mia2LogGroupPlugin, "anchorsync init end");
}

int SharpnessMiAlgo::ReleaseRef()
{
    std::unique_lock<std::mutex> lock(mAlgoMutex);
    if (mRefCount > 0) {
        mRefCount--;
    } else {
        MLOGD(Mia2LogGroupPlugin, "mRefCount already 0");
    }
    if (mRefCount == 0 && mHandle) {
        MLOGI(Mia2LogGroupPlugin, "start Unit");
        while (mState == Runing) {
            mWaitAlgoCond.wait(lock);
        }
        MialgoRFSUnit(&mHandle);
        mHandle = NULL;
        MialgoBasicLibUnit();
        mState = Uninit;
    }
    mWaitAlgoCond.notify_all();
    return 0;
}

void SharpnessMiAlgo::Preprocess(AnchorParamInfo *pAnchorParamInfo)
{
    memset(&mConfig, 0, sizeof(mConfig));
    mConfig.xratio = 4;
    mConfig.yratio = 4;
    mConfig.way_select = 0;
    mConfig.face_roi_enable = 0;
    mConfig.face_angle_shift = 90;
    mConfig.min_face_num = MIALGO_RFS_MAX_FACE_NUM;
    mConfig.mean_enable = 1;
    mConfig.frame0_mean_luma = 0.0;
    mConfig.roi_enable = 0;
    mConfig.roi_rect.x = 0;
    mConfig.roi_rect.y = 0;
    mConfig.roi_rect.width = 0;
    mConfig.roi_rect.height = 0;

    switch (pAnchorParamInfo->bayerPattern) {
    case ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_RGGB:
        mConfig.bayer_pattern = MIALGO_BAYER_RGGB;
        break;
    case ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GRBG:
        mConfig.bayer_pattern = MIALGO_BAYER_GRBG;
        break;
    case ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_GBRG:
        mConfig.bayer_pattern = MIALGO_BAYER_GBRG;
        break;
    case ANDROID_SENSOR_INFO_COLOR_FILTER_ARRANGEMENT_BGGR:
        mConfig.bayer_pattern = MIALGO_BAYER_BGGR;
        break;
    default:
        break;
    }
    ImageParams imageparam = (*(pAnchorParamInfo->pBufferInfo))[0];
    uint32_t format = imageparam.format.format;

    if (MiaPixelFormat::CAM_FORMAT_RAW16 == format) {
        mConfig.bit = 12; // todo how to distinguish packedraw and mipiraw
    } else if (MiaPixelFormat::CAM_FORMAT_RAW10 == format) {
        mConfig.bit = 10;
    } else if (MiaPixelFormat::CAM_FORMAT_RAW12 == format) {
        mConfig.bit = 16;
    } else {
        MLOGE(Mia2LogGroupPlugin, "unsupport format %d", format);
    }

    mSizes[0] = 1;
    mSizes[1] = imageparam.format.height;
    switch (mConfig.bit) {
    case 10:
        mSizes[2] = imageparam.format.width * 5 / 4;
        break;
    case 12:
        mSizes[2] = imageparam.format.width * 2;
        break;
    case 16:
        mSizes[2] = imageparam.format.width * 3 / 2;
        break;
    case 18:
        mSizes[2] = imageparam.format.width * 2;
        break;
    default:
        mSizes[2] = imageparam.format.width * 5 / 4;
        break;
    }

    mBufSize = mSizes[1] * AlignGeneric32(mSizes[2], 16);
    mAligned[2] = AlignGeneric32(mSizes[2], 16);
    mFlag = MIALGO_MAT_FLAG_IMG_MAT | MIALGO_MAT_FLAG_CH_LAST;
    MLOGD(Mia2LogGroupPlugin,
          "AnchorParamInfo: {aecLux: %f,  "
          "cropRegion:[x:%d, y:%d, width: %d, height: %d], "
          "faceInfostride: faceNum:%d, [x:%d, y:%d, width: %d, height: %d], "
          "QuadCFASensor: %d, bayerPattern:%d]",
          pAnchorParamInfo->aecLux, pAnchorParamInfo->cropRegion.x, pAnchorParamInfo->cropRegion.y,
          pAnchorParamInfo->cropRegion.width, pAnchorParamInfo->cropRegion.height,
          pAnchorParamInfo->faceInfo.faceNum, pAnchorParamInfo->faceInfo.faceRectInfo.x,
          pAnchorParamInfo->faceInfo.faceRectInfo.y, pAnchorParamInfo->faceInfo.faceRectInfo.width,
          pAnchorParamInfo->faceInfo.faceRectInfo.height, pAnchorParamInfo->isQuadCFASensor,
          pAnchorParamInfo->bayerPattern);

    MiCropRect pCrop = pAnchorParamInfo->cropRegion;
    mConfig.roi_enable = 0;
    if (pCrop.x || pCrop.y) {
        float widthScalar = imageparam.format.width / (float)(pCrop.x * 2 + pCrop.width);
        float heightScalar = imageparam.format.height / (float)(pCrop.y * 2 + pCrop.height);

        mConfig.roi_enable = 1;
        mConfig.roi_rect.x = AlignGeneric32(pCrop.x * widthScalar, 4);
        mConfig.roi_rect.y = AlignGeneric32(pCrop.y * heightScalar, 2);
        mConfig.roi_rect.width = AlignGeneric32(pCrop.width * widthScalar, 4);
        mConfig.roi_rect.height = AlignGeneric32(pCrop.height * heightScalar, 2);
        MLOGD(Mia2LogGroupPlugin, "enablecrop:[x%d y%d w%d h%d] orignal:[x%d y%d w%d h%d]",
              mConfig.roi_rect.x, mConfig.roi_rect.y, mConfig.roi_rect.width,
              mConfig.roi_rect.height, pCrop.x, pCrop.y, pCrop.width, pCrop.height);
    }
}

int SharpnessMiAlgo::Calculate(AnchorParamInfo *pAnchorParamInfo)
{
    std::unique_lock<std::mutex> lock(mAlgoMutex);
    while (mState == Initing || mState == Runing) {
        mWaitAlgoCond.wait(lock);
    }
    if (mHandle == NULL) {
        MLOGW(Mia2LogGroupPlugin, "handle = NULL");
        mWaitAlgoCond.notify_all();
        lock.unlock();
        return MIALGO_ERROR;
    }
    mState = Runing;
    lock.unlock();

    Preprocess(pAnchorParamInfo);

    MI_S32 result = MIALGO_OK;
    if (MIALGO_OK == result) {
        struct timeval start, end;
        std::vector<ImageParams> &imageparam = *pAnchorParamInfo->pBufferInfo;
        for (int frameIndex = 0; frameIndex < pAnchorParamInfo->inputNum; frameIndex++) {
            mConfig.frame_idx = frameIndex;
            if (0 == mConfig.roi_enable) {
                faceParamUpdata(pAnchorParamInfo, frameIndex);
            }

            void *pHostptr = NULL;
            MI_U64 sharpnessresult;
            MialgoMat mat_src[1];
            MialgoMemInfo mem_info;
            int fd;
            if (MIALGO_OK == result) {
                pHostptr = imageparam[frameIndex].pAddr[0];
                fd = imageparam[frameIndex].fd[0];
                if (NULL == pHostptr || -1 == fd) {
                    result = MIALGO_ERROR;
                    MLOGE(Mia2LogGroupPlugin, "fail to get hostptr");
                }
            }
            if (MIALGO_OK == result) {
                mem_info = mialgoIonMemInfo(mBufSize, 0, fd);
                result =
                    MialgoInitMat(mat_src, 3, mSizes, MIALGO_MAT_U8, mAligned, mFlag, pHostptr);
            }
            if (MIALGO_OK == result) {
                result = MialgoMatSetMemInfo(mat_src, &mem_info, 1);
            }
            if (MIALGO_OK == result) {
                gettimeofday(&start, NULL);

                result = MialgoRFSCalSharpness(mHandle, mat_src, &sharpnessresult, &mConfig);
                pAnchorParamInfo->sharpness[frameIndex] = sharpnessresult;
                if (MIALGO_OK != result) {
                    MLOGW(Mia2LogGroupPlugin,
                          "MialgoRFSCalSharpness faild, sharpnessresult: %llu %d %d %d",
                          sharpnessresult, result, mConfig.bayer_pattern, mConfig.bit);
                } else {
                    gettimeofday(&end, NULL);
                    int timeuse =
                        1000000 * (end.tv_sec - start.tv_sec) + end.tv_usec - start.tv_usec;
                    MLOGI(Mia2LogGroupPlugin,
                          "CalculateSharpnessMiAlgo time: %d "
                          "sharpnessresult[%d]: %llu bayerpattern: %d "
                          "bit: %d stride: %d face_roi_enable: %d",
                          timeuse, mConfig.frame_idx, sharpnessresult, mConfig.bayer_pattern,
                          mConfig.bit, mAligned[2], mConfig.face_roi_enable);
                }
            }

            if (MIALGO_OK != result) {
                MLOGE(Mia2LogGroupPlugin, "fail to Calculate Sharpness");
                break;
            }
        }
    }

    if (MIALGO_OK != result) {
        char backtrace[1024];
        MialgoErrorBacktrace(result, backtrace, 1024);
        MLOGD(Mia2LogGroupPlugin, "%s", backtrace);
    }
    {
        std::unique_lock<std::mutex> lock(mAlgoMutex);
        if (mState == Runing) {
            mState = Idle;
        }
        mWaitAlgoCond.notify_all();
    }
    return result;
}

void SharpnessMiAlgo::faceParamUpdata(const AnchorParamInfo *pAnchorParamInfo, const int &i)
{
    static uint8_t faceWeight =
        static_cast<uint8_t>(property_get_int32("persist.vendor.camera.faceweight", 20));
    int numFaces = pAnchorParamInfo->faceInfo.faceNum;
    MiFaceRect tmp = pAnchorParamInfo->faceInfo.faceRectInfo;
    float lux = pAnchorParamInfo->aecLux;
    int scale = (1 == pAnchorParamInfo->isQuadCFASensor) ? 2 : 1;
    mConfig.face_roi_enable = 0;
    if (lux > 335.0f && 1 == numFaces) {
        mConfig.face_roi[i].face_num = numFaces;
        mConfig.face_weight = faceWeight;
        mConfig.face_roi[i].angle[0] = 0;
        mConfig.face_roi[i].face_roi[0].x = tmp.x / scale;
        mConfig.face_roi[i].face_roi[0].y = tmp.y / scale;
        int width = tmp.width / scale;
        int height = tmp.height / scale;
        mConfig.face_roi[i].face_roi[0].width = width - mConfig.face_roi[i].face_roi[0].x + 1;
        mConfig.face_roi[i].face_roi[0].height = height - mConfig.face_roi[i].face_roi[0].y + 1;
        MLOGD(Mia2LogGroupPlugin, "face num%d x%d y%d w%d h%d angle%f scale%d Lux: %f", 0,
              mConfig.face_roi[i].face_roi[0].x, mConfig.face_roi[i].face_roi[0].y,
              mConfig.face_roi[i].face_roi[0].width, mConfig.face_roi[i].face_roi[0].height,
              mConfig.face_roi[i].angle[0], scale, lux);

        if (mConfig.face_roi[i].face_roi[0].width > 300 &&
            mConfig.face_roi[i].face_roi[0].height > 300) {
            mConfig.face_roi_enable = 1;
        }
    }
}

uint32_t SharpnessMiAlgo::AlignGeneric32(uint32_t operand, unsigned int alignment)
{
    unsigned int remainder = (operand % alignment);
    return (0 == remainder) ? operand : operand - remainder + alignment;
}
