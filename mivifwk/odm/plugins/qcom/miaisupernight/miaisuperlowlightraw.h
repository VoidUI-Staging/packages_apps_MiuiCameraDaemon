#ifndef _MiaiSuperLowlightRaw_H_
#define _MiaiSuperLowlightRaw_H_

#include <chrono>
#include <condition_variable>
#include <mutex>

#include "MiaPluginUtils.h"
#include "amcomdef.h"
#include "ammem.h"
#include "asvloffscreen.h"
#include "merror.h"
#include "mi_supernight_raw.h"

#define FRAME_MIN_INPUT_NUMBER        4
#define FRAME_DEFAULT_INPUT_NUMBER    8
#define FRAME_DEFAULT_EXPOSURE_STRING "0,0,0,0,0,0,-12,-24"

#define HARDCODE_CAMERAID_REAR  0
#define HARDCODE_CAMERAID_FRONT 0x100

#define USE_INTERNAL_BUFFER        0
#define INTERNAL_BUFFER_PIXEL_SIZE 4132 * 3124 * 2

#define FLAG_FILL_ALL      0
#define FLAG_FILL_METADATA 1

#define MIAI_MAX_FACE_NUMBER      8
#define METADATA_VALUE_NUMBER_MAX 16
#define INIT_VALUE                33
#define RET_ADSP_INIT_ERROR       1002
#define RET_XIOMI_TIME_OUT        0xFFFF

static const int64_t FlushTimeoutSec = 5; // 5s
static char *AdspEnvPath =
    "ADSP_LIBRARY_PATH=vendor/lib64/camera;/vendor/etc/camera;/system/lib/rfsa/adsp;/system/vendor/"
    "lib/rfsa/adsp;/dsp";
static const char *MiaiAdspEnvPath = "vendor/lib64/camera";

typedef struct
{
    MUInt32 validFrameCount;
    MInt32 Ev[METADATA_VALUE_NUMBER_MAX];
    MFloat ISPGain[METADATA_VALUE_NUMBER_MAX];
    MFloat SensorGain[METADATA_VALUE_NUMBER_MAX];
    MFloat Shutter[METADATA_VALUE_NUMBER_MAX];
} SuperNightRawValue;

class MiaiSuperLowlightRaw
{
public:
    static MiaiSuperLowlightRaw *getInstance();
    MRESULT init(MInt32 camMode, MInt32 camState, MInt32 i32Width, MInt32 i32Height,
                 MPBASE_Version *buildVer, RECT *pZoomRect = MNull, MInt32 i32LuxIndex = 0);
    MRESULT process(MInt32 *input_fd, ORGIMAGE *input, MInt32 *output_fd, ORGIMAGE *output,
                    RECT *pOutImgRect, MInt32 *pExposure, MUInt32 input_num, MInt32 sceneMode,
                    MInt32 camState, MI_SN_RAWINFO *info, MI_SN_FACEINFO *faceinfo,
                    MInt32 *correctImageRet);

    void setInitStatus(int value) { m_initStatus = value; }
    MRESULT setSuperNightRawValue(SuperNightRawValue *pSuperNightRawValue, MUInt32 input_num);
    MRESULT uninit();
    MRESULT processAlgo(MInt32 *output_fd, ORGIMAGE *output, ORGIMAGE *normalInput, MInt32 camState,
                        MInt32 sceneMode, RECT *pOutImgRect, MI_SN_FACEINFO *faceinfo,
                        MInt32 *correctImageRet, MI_SN_RAWINFO *normalRawInfo, MInt32 i32EVoffset,
                        MInt32 i32DeviceOrientation);
    MRESULT processCorrectImage(MInt32 *output_fd, ORGIMAGE *output, MInt32 camState,
                                MI_SN_RAWINFO *normalRawInfo);
    MRESULT processPrepare(MInt32 *input_fd, ORGIMAGE *input, MInt32 camState, MInt32 inputIndex,
                           MInt32 input_Num, MI_SN_RAWINFO *rawInfo);
    void initHTP();
    void unInitHTP();
    bool getInitFlag();
    bool deteckTimeout(int64_t flushTimeoutSec);

private:
    MiaiSuperLowlightRaw();
    virtual ~MiaiSuperLowlightRaw();
    void InitInternalBuffer();
    void UninitInternalBuffer();
    void CopyImage(ORGIMAGE *pDstImg, ORGIMAGE *pSrcImg);
    void setOffScreen(ORGIMAGE *pDstImg, ORGIMAGE *pSrcImg);
    void FillInternalImage(ORGIMAGE *pDstImage, ORGIMAGE *pSrcImage, int flag = FLAG_FILL_ALL);
    void OutputInternalImage(ORGIMAGE *pDstImage, ORGIMAGE *pSrcImage, int flag = FLAG_FILL_ALL);
    void AllocCopyImage(ORGIMAGE *pDstImage, ORGIMAGE *pSrcImage);
    void ReleaseImage(ORGIMAGE *pImage);
    bool checkUserCancel();
    static MRESULT SuperNightProcessCallback(
        MLong lProgress, // Not impletement
        MLong lStatus,   // Not impletement
        MVoid *pParam    // Thre "pUserData" param of function MI_SN_Process
    );
    bool putEnv(int maxloop);

private:
    MHandle m_hSuperLowlightRawEngine;
    ORGIMAGE mImageInput[FRAME_DEFAULT_INPUT_NUMBER];
    ORGIMAGE mImageOutput;
    ORGIMAGE mImageCache;
    unsigned int u32ImagePixelSize;
    int m_initStatus;
    SuperNightRawValue m_superNightRawValue;
    bool m_isPreprocessSucess;
    static MiaiSuperLowlightRaw *m_MIAISLLRawSingleton;
    static std::mutex m_mutex;
    bool m_initFlag;
    bool m_initHTPFlag;
    std::condition_variable m_timeoutCond;
    std::mutex m_timeoutMux;
    std::mutex m_initMux;
    class Garbo
    {
    public:
        Garbo() { MLOGI(Mia2LogGroupPlugin, "Garbo construction"); }
        ~Garbo()
        {
            MLOGI(Mia2LogGroupPlugin, "Garbo destruction");
            // We can destory all the resouce here
            if (m_MIAISLLRawSingleton != NULL) {
                delete m_MIAISLLRawSingleton;
                m_MIAISLLRawSingleton = NULL;
                MLOGI(Mia2LogGroupPlugin, "MiaiSuperLowlightRaw destruction");
            }
        }
    };
    static Garbo gc;
};
#endif
