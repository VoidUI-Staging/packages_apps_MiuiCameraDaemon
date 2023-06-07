#include "altekdepurple.h"

#undef LOG_TAG
#define LOG_TAG "DepurplePluginAltek"

#define MAX_VER_SIZE 256
namespace mialgo2 {

#define MAX_SUPPORT_VENDOR 6 // for 1, 2 sensor vendor
// 1.Second supplier's tunning bin name should be different one supplier,but we do not get these
// libs from jozef. so we use the same name temporarily. 2.If the same vendor is the different
// project, we will use the same tunning bin name.

const char *SensorIdString(int id)
{
    switch (id) {
    case BACK_WIDE:
        return "W";
    case BACK_TELE:
        return "T_2x";
    case BACK_UWIDE:
        return "UW";
    case BACK_TELE4X:
        return "T_4x";
    case BACK_MACRO:
        return "MACRO";
    default:
        return "W";
    }
}

const char *getModeString(CFR_MODEINFO_T &modeInfo)
{
    if (modeInfo.isHDMode)
        return "_HD";
    else if (modeInfo.isSNMode)
        return "_SN";
    else if (modeInfo.isHDRMode)
        return "_HDR";
    else if (modeInfo.isInSensorZoom)
        return "_IZOOM";
    else
        return "";
}

const char *getLevelString()
{
    int lowLevel = PluginUtils::isLowCamLevel();
    switch (lowLevel) {
    case 0:
        return "";
    case 1:
        return "_L";
    default:
        return "";
    }
}

const char *getZoomRatio(uint32_t ZoomRatio)
{
    if (ZoomRatio >= DEPURPLE_ZOOMRATIO_FOUR) {
        return "_4x";
    } else {
        return "";
    }
}

QAltekDepurple::QAltekDepurple(uint32_t width, uint32_t height)
{
    mBuffer = NULL;
    mInOutHandle = NULL;
    m_CFRInitThread = NULL;
    mInitState = CFR_UNINIT;
    misHighDefinitionSize = false;
    mInitRefCount = 0;
    mOriWidth = width;
    mOriHeight = height;
    memset(&m_modeInfo, 0, sizeof(CFR_MODEINFO_T));
}

QAltekDepurple::~QAltekDepurple()
{
    m_refCountMutex.lock();

    if (mInitRefCount != 0) {
        deinitCFR();
        mInitRefCount = 0;
    }

    m_refCountMutex.unlock();
}

void QAltekDepurple::deinitCFR()
{
    MLOGD(Mia2LogGroupPlugin, "%s mBuffer %p %p", __func__, mBuffer, mInOutHandle);

    {
        std::unique_lock<std::mutex> lk(m_initMutex);

        if (mInOutHandle) {
            if (misHighDefinitionSize) {
                alCFR_Instance_Close_SP(&mInOutHandle);
            } else {
                alCFR_Instance_Close(&mInOutHandle);
            }
        }

        if (mBuffer != NULL) {
            free(mBuffer);
            mBuffer = NULL;
        }
        mInOutHandle = NULL;
        mInitState = CFR_UNINIT;
    }

    if (m_CFRInitThread && m_CFRInitThread->joinable()) {
        m_CFRInitThread->join();
    }

    if (NULL != m_CFRInitThread) {
        delete m_CFRInitThread;
        m_CFRInitThread = NULL;
    }

    MLOGD(Mia2LogGroupPlugin, "%s deinitCFR end", __func__);
}

int QAltekDepurple::initCFR()
{
    alCFR_ERR_CODE ret = alCFR_ERR_SUCCESS;

    std::unique_lock<std::mutex> lk(m_initMutex);

    double starttime = PluginUtils::nowMSec();

    if (CFR_INIT_ENTER == mInitState) {
        int level = 1;
        char version[MAX_VER_SIZE];
        alCFR_VersionInfo_Get(version, MAX_VER_SIZE);

        ret = loadResource(m_modeInfo);
        if (ret != alCFR_ERR_SUCCESS) {
            MLOGE(Mia2LogGroupPlugin, "%s loadResource fail!", __func__);
        }

        int dTuningDataVersion = 0;
        void *pCFRPara = mBuffer;
#if defined(NUWA_CAM) || defined(FUXI_CAM) || defined(SOCRATES_CAM)
        ret |= alCFR_TuningData_VersionInfo_Get(&dTuningDataVersion, pCFRPara,
                                                mInput_tuning_data_size);
#else
        ret |= alCFR_TuningData_VersionInfo_Get(&dTuningDataVersion, pCFRPara);
#endif
        if (ret != alCFR_ERR_SUCCESS) {
            MLOGE(Mia2LogGroupPlugin, "alCFR_TuningData_VersionInfo_Get is fail");
        }
        char acBinPath[512] = "/data/vendor/camera/";
        ret |= alCFR_Instance_Set_Working_Path(acBinPath, 512);
        if (ret != alCFR_ERR_SUCCESS) {
            MLOGE(Mia2LogGroupPlugin, "alCFR_Instance_Set_Working_Path fail");
        }

        if (alCFR_ERR_SUCCESS == ret && misHighDefinitionSize) {
            MLOGI(Mia2LogGroupPlugin, "HD level = %d", level);
            ret = alCFR_Instance_Init_SP(&mInOutHandle, mOriWidth, mOriHeight, mBuffer, level);
        } else if (alCFR_ERR_SUCCESS == ret) {
            ret = alCFR_Instance_Init(&mInOutHandle, mOriWidth, mOriHeight, mBuffer);
        }
        if (ret != alCFR_ERR_SUCCESS) {
            mInitState = CFR_INIT_FAIL;
            MLOGE(Mia2LogGroupPlugin, "alCFR_Init is fail");
        } else {
            mInitState = CFR_INT_SUCCESS;
        }
        m_initCondition.notify_all();

        MLOGI(Mia2LogGroupPlugin,
              "DePurple get algorithm library version: %s dTuningDataVersion =%d, initCFR time "
              "= %.2f ms",
              version, dTuningDataVersion, PluginUtils::nowMSec() - starttime);
    }

    return ret;
}

int QAltekDepurple::incInit(CFR_MODEINFO_T modeInfo, bool incIfNoInit)
{
    alCFR_ERR_CODE ret = alCFR_ERR_SUCCESS;

    m_refCountMutex.lock();

    if (incIfNoInit && mInitRefCount) {
        MLOGI(Mia2LogGroupPlugin, "init inc reference count %d, only incIfNoInit", mInitRefCount);
    } else {
        if ((++mInitRefCount) == 1) {
            m_modeInfo = modeInfo;
            misHighDefinitionSize = modeInfo.isHDMode;
            if (NULL == m_CFRInitThread) {
                mInitState = CFR_INIT_ENTER;
                m_CFRInitThread = new std::thread([this] { initCFR(); });
                if (NULL == m_CFRInitThread) {
                    mInitState = CFR_UNINIT;
                }
            }
        }
        MLOGI(Mia2LogGroupPlugin, "init inc reference count %d", mInitRefCount);
    }

    m_refCountMutex.unlock();

    return ret;
}

void QAltekDepurple::decInit()
{
    m_refCountMutex.lock();

    if (mInitRefCount && (--mInitRefCount == 0)) {
        deinitCFR();
    }
    MLOGI(Mia2LogGroupPlugin, "init dec reference count %d", mInitRefCount);

    m_refCountMutex.unlock();
}

#if defined(NUWA_CAM) || defined(FUXI_CAM) || defined(SOCRATES_CAM)
int QAltekDepurple::process(struct MiImageBuffer *input, alCFR_FACE_T faceInfo, uint32_t exposure,
                            alCFR_RECT zoomInfo, CFR_MODEINFO_T &modeInfo)
#else
int QAltekDepurple::process(struct MiImageBuffer *input, alCFR_FACE_T faceInfo, uint32_t exposure,
                            alGE_RECT zoomInfo, CFR_MODEINFO_T &modeInfo)
#endif
{
    alCFR_ERR_CODE ret = alCFR_ERR_SUCCESS;

    GENERAL_IMAGE_INFO InImgInfo; // OutImgInfo;
    memset(&InImgInfo, 0, sizeof(GENERAL_IMAGE_INFO));

    uint32_t dCFRImgW, dCFRImgH;
    dCFRImgW = (uint32_t)input[0].width;
    dCFRImgH = (uint32_t)input[0].height;

    unsigned char *pucInputBufferNV12 = input[0].plane[0];
    unsigned char *pucOutputBufferNV12 = input[0].plane[1];

    InImgInfo.m_udFormat = IMG_FORMAT_YCC420NV12;
    InImgInfo.m_udStride_0 = input[0].stride; // dCFRImgW;
    InImgInfo.m_udStride_1 = input[0].stride; // dCFRImgW;
    InImgInfo.m_pBuf_0 = pucInputBufferNV12;
    InImgInfo.m_pBuf_1 = pucOutputBufferNV12;

    {
        std::unique_lock<std::mutex> lk(m_initMutex);

        if (CFR_INIT_ENTER == mInitState) {
            if (m_initCondition.wait_for(lk, std::chrono::seconds(1)) == std::cv_status::timeout) {
                MLOGD(Mia2LogGroupPlugin, "QAltekDepurple wait for init done timeout");
            }
        }
        double starttime = PluginUtils::nowMSec();

        if (CFR_INT_SUCCESS == mInitState) {
            loadResource(modeInfo);
            void *pCFRPara = mBuffer;
            MLOGD(Mia2LogGroupPlugin, "%s dCFRImgW=%d,dCFRImgH=%d,pCFRPara=%p", __func__, dCFRImgW,
                  dCFRImgH, pCFRPara);

            ret |= alCFR_Instance_ExposureInfo_Set(mInOutHandle, exposure);
            MLOGD(Mia2LogGroupPlugin, "QAltekDepurple ExposureInfo_Set ret %d Exposure:%u", ret,
                  exposure);

            ret |= alCFR_Instance_FaceInfo_Set(mInOutHandle, faceInfo);
            MLOGD(Mia2LogGroupPlugin, "QAltekDepurple FaceInfo_Set ret %d ", ret);

            ret |= alCFR_Instance_ZoomInfo_Set(mInOutHandle, zoomInfo, mOriWidth, mOriHeight);
            MLOGD(Mia2LogGroupPlugin, "QAltekDepurple ZoomInfo_Set ret %d ", ret);

            if (ret != alCFR_ERR_SUCCESS) {
                MLOGE(Mia2LogGroupPlugin, "alCFR_Run params error");
            }

            if (alCFR_ERR_SUCCESS == ret && misHighDefinitionSize) {
                ret = alCFR_Instance_Run_SP(&InImgInfo, mInOutHandle, dCFRImgW, dCFRImgH, pCFRPara);
            } else if (alCFR_ERR_SUCCESS == ret) {
                ret = alCFR_Instance_Run(&InImgInfo, mInOutHandle, dCFRImgW, dCFRImgH, pCFRPara);
            }

            if (ret != alCFR_ERR_SUCCESS) {
                MLOGE(Mia2LogGroupPlugin, "alCFR_Run is fail");
            }
        } else if (CFR_INIT_FAIL == mInitState) {
            ret = alCFR_ERR_NO_INITIAL;
            MLOGE(Mia2LogGroupPlugin, "alCFR_Init is fail");
        } else {
            MLOGI(Mia2LogGroupPlugin, "bypass alCFR_Run");
        }

        MLOGI(Mia2LogGroupPlugin, "depruple plugin process time is %.2f",
              (PluginUtils::nowMSec() - starttime));
    }

    return ret;
}

int QAltekDepurple::loadResource(CFR_MODEINFO_T &modeInfo)
{
    int ret = 0;

    if (CFR_INT_SUCCESS == mInitState && m_modeInfo == modeInfo) {
        return ret;
    }

    FILE *pFile = NULL;
    m_modeInfo = modeInfo;
    char calibFileName[1024];
    const char *pSensorId = SensorIdString(modeInfo.sensorId);
    const char *pMode = getModeString(modeInfo);
    const char *pLevel = getLevelString();
    const char *pZoomRatio = getZoomRatio(modeInfo.zoomRatio);
    sprintf(calibFileName, "/odm/etc/camera/CFR_para_%s_V0%d%s%s%s.bin", pSensorId,
            modeInfo.vendorId, pMode, pLevel, pZoomRatio);

    if (access(calibFileName, F_OK)) {
        MLOGI(Mia2LogGroupPlugin, "load default tuning bin , vendorId:1");
        sprintf(calibFileName, "/odm/etc/camera/CFR_para_%s_V01%s%s%s.bin", pSensorId, pMode,
                pLevel, pZoomRatio);
        if (access(calibFileName, F_OK)) {
            sprintf(calibFileName, "/odm/etc/camera/CFR_para_%s_V01%s%s.bin", pSensorId, pMode,
                    pLevel);
            if (access(calibFileName, F_OK)) {
                sprintf(calibFileName, "/odm/etc/camera/CFR_para_%s_V01.bin", pSensorId);
            }
        }
    }
    pFile = fopen(calibFileName, "rb");
    if (pFile != NULL) {
        fseek(pFile, 0, SEEK_END);
        long len = ftell(pFile);
#if defined(NUWA_CAM) || defined(FUXI_CAM) || defined(SOCRATES_CAM)
        mInput_tuning_data_size = 0;
        mInput_tuning_data_size = static_cast<unsigned int>(len);
#endif
        if (mBuffer == NULL && len > 0L) {
            mBuffer = (unsigned char *)malloc(len);
        }
        if (mBuffer == NULL) {
            MLOGE(Mia2LogGroupPlugin, "malloc mBuffer failed");
            ret = -1;
        } else {
            fseek(pFile, 0, SEEK_SET);
            fread(mBuffer, 1, len, pFile);
        }
        fclose(pFile);
        pFile = NULL;
    } else {
        MLOGE(Mia2LogGroupPlugin, "open fail %s", calibFileName);
        ret = -1;
    }
    MLOGI(Mia2LogGroupPlugin, "%s loadResource mBuffer %p vendor_id %d sensor %s calibFileName %s",
          __func__, mBuffer, modeInfo.vendorId, pSensorId, calibFileName);
    return ret;
}

} // namespace mialgo2
