#ifndef _ARCCAPBOKEHPLUGIN_H_
#define _ARCCAPBOKEHPLUGIN_H_

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>
//#include "MiDualCamBokehLite.h"

#include "chivendortag.h"
#include "chioemvendortag.h"

#include "BeautyShot_Image_Algorithm.h"
#include "SkinBeautifierDef.h"
#include "ammem.h"
#include "arcsoft_dualcam_image_refocus.h"
#include "asvloffscreen.h"


typedef struct tag_input_params_t
{
    ASVLOFFSCREEN mainimg;      ///< The input main image buffer
    ASVLOFFSCREEN auximage;     ///< The input aux image buffer
    ASVLOFFSCREEN outimage;     ///< The output result image buffer
    ASVLOFFSCREEN depthimage;   ///< The output depth buffer
    ASVLOFFSCREEN outteleimage; ///< The output tele buffer

    ARC_REFOCUSCAMERAIMAGE_PARAM caminfo; ///< The ISP crop image param
    ARC_DCIR_REFOCUS_PARAM refocus;       ///< The refocus param
    float luxIndex;
    int isoValue;
    int i32Orientation;
} frame_params_t;

#define BLUR_INTENSITY             35
#define BLUR_INTENSITY_NIGHT       60
#define FILTER_SHINE_THRES         2
#define FILTER_SHINE_LEVEL         70
#define FILTER_SHINE_LEVEL_REFOCUS         40
#define FILTER_SHINE_THRES_NIGHT   5
#define FILTER_SHINE_THRES_REFOCUS   5
#define FILTER_SHINE_LEVEL_NIGHT   30
#define FILTER_SHINE_THRES_OUTSIDE 2
#define FILTER_SHINE_LEVEL_OUTSIDE 20
#define FILTER_INTENSITY           70
#define CAPTURE_BOKEH_DEGREE_0     0
#define CAPTURE_BOKEH_DEGREE_270   270
#ifdef ZEUS_CAM
#define CAPTURE_BOKEH_DEGREE_WT 199
#define CAPTURE_BOKEH_DEGREE_WU 9
#elif defined(CUPID_CAM)
#define CAPTURE_BOKEH_DEGREE_WU 9
#define CAPTURE_BOKEH_DEGREE_WT 0
#elif defined(MAYFLY_CAM)
#define CAPTURE_BOKEH_DEGREE_WU 9
#define CAPTURE_BOKEH_DEGREE_WT 0
#elif defined(DITING_CAM)
#define CAPTURE_BOKEH_DEGREE_WU 9
#define CAPTURE_BOKEH_DEGREE_WT 0
#else
#define CAPTURE_BOKEH_DEGREE_WT 0
#define CAPTURE_BOKEH_DEGREE_WU 0
#endif
#define MAX_FACE_NUM      10
#define CTB_RESULT_OK     0
#define CTB_RESULT_FAILED 1
#define MAX_EXTRA_SIZE    256

#define HARDCODE_FOR_195_9_RATIO ((float)19.5 / 9)
#define HARDCODE_FOR_18_9_RATIO  ((float)18.0 / 9)
#define HARDCODE_FOR_16_9_RATIO  ((float)16.0 / 9)
#define HARDCODE_FOR_4_3_RATIO   ((float)4.0 / 3)

#define HARDCODE_BEAUTY_FACE_NUM 2

#define CTB_INPUT_TELE 0
#define CTB_INPUT_WIDE 1

//#define _HARD_CODE_FOR_PARAMS_
//

#define MI_EXAMPLE_OUTPUT_BUF_CNT 10

//redefinition by MiDualCamBokehLite.cpp
enum Region {
    BEAUTYSHOT_REGION_DEFAULT = -1,
    BEAUTYSHOT_REGION_GLOBAL = 0,
    BEAUTYSHOT_REGION_CHINA = 1,
    BEAUTYSHOT_REGION_INDIAN = 2,
    BEAUTYSHOT_REGION_BRAZIL = 3
};

class ArcCapBokehPlugin : public PluginWraper
{
public:
    static std::string getPluginName()
    {
        // Must match library name
        return "com.xiaomi.plugin.capbokeh";
    }

    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);

    int deInit();

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);

    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);

    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);

    virtual void destroy();

    virtual bool isEnabled(MiaParams settings) { return true; }

private:
    ~ArcCapBokehPlugin();

    int readCalibrationDataFromFile(const char *szFile, void *caliBuff, uint32_t dataSize);
    int processBuffer(std::vector<ImageParams> teleInputvector,
                      std::vector<ImageParams> wideInputvector,
                      struct MiImageBuffer *bokehOutput,
                      struct MiImageBuffer *teleOutput,
                      struct MiImageBuffer *depthOut);
    int iniEngine();

    int setFrameParams(struct MiImageBuffer *teleInput, struct MiImageBuffer *wideInput,
                       struct MiImageBuffer *bokehOutput, struct MiImageBuffer *teleOutput,
                       struct MiImageBuffer *depthOut,
                       const std::vector<camera_metadata_t *> &logicalMetaVector);

    void dumpYUVData(ASVLOFFSCREEN *pASLIn, uint32_t index, const char *namePrefix);
    void DumpInputData(ImageParams inputimage, const char *namePrefix, int Index);
    void dumpRawData(uint8_t *pData, int32_t size, uint32_t index, const char *namePrefix);
    void PrepareMiImageBuffer(struct MiImageBuffer *dstImage, ImageParams srcImage);
    void prepareImage(struct MiImageBuffer *image, ASVLOFFSCREEN &stImage);
    void mapRectToInputData(CHIRECT &srcRect, CHIRECT &IFERect, int &nDataWidth, int &nDataHeight,
                            CHIRECT &dstRectOnInputData);
    void convertToArcFaceInfo(FaceResult *pFaceResult, ARC_DCR_FACE_PARAM &faceinfo,
                              CHIRECT &IFERect, int &nDataWidth, int &nDataHeight);
    MRESULT CreateHDRImage(std::vector<struct MiImageBuffer *> teleInputvector,
                           LPASVLOFFSCREEN pEnhanceOffscreen, MRECT *cropRect,  uint8_t *pDisparityData);
    int doProcess(std::vector<struct MiImageBuffer *> teleInputvector);
    void allocImage(ASVLOFFSCREEN *dstImage, ASVLOFFSCREEN *src);
    void releaseImage(ASVLOFFSCREEN *image);
    int getFaceDegree(int faceOrientation);
    void copyImage(ASVLOFFSCREEN *srcImg, ASVLOFFSCREEN *dstImg);
    void mergeImage(ASVLOFFSCREEN *baseImg, ASVLOFFSCREEN *overImg, int downsize);
    void checkPersist();
    void getShineParam(MInt32 i32ISOValue, MInt32 &i32ShineLevel, MInt32 &i32ShineThres);
    void writeFileLog(char *file, const char *fmt, ...);

    void *mCalibrationFront;
    void *mCalibrationWU;
    void *mCalibrationWT;
    void *mCaptureBokeh;
    ARC_DCIR_PARAM mBokehParam;
    frame_params_t mFrameParams;
    CHIRECT mRtFocusROIOnInputData;
    //int mFocusDistance;
    //int mBV;
    bool mDumpYUV;
    bool mDumpPriv;
    int mDrawROI;
    int mViewCtrl;

    int32_t mInt32DisparityDataActiveSize;
    int mBeautyEnabled;
    int32_t mBlurLevel;
    int32_t mMappingVersion;
    int mLightingMode;
    int mOrient;
    //int mMappedOrient;
    uint32_t mFrameworkCameraId;
    int mFaceOrientation;
    bool mRearCamera; ////<The InstanceId for front or real camera
    bool mHdrEnabled;
    int mCaptureMode; ////value 1,2(Normal mode, night mode)
    int mNightLevel; ////value 1,2(night, extreme night)
    int mCalcMode; ///< normal: 0, lite: 1
    int mThermallevel; //[in] The Phone temperature 0: Low < 35; 1: 35 ~ 37;2: 41 ~43; 3:
                               // 43 ~ 45; 4: 46 ~ 48; 5:49~50
    int mMaxParallelNum;
    int mCurrentParallelNum;
    //CameraRoleType mMasterRole;
    bool mSDKMode;
    camera_metadata_t *mMainPhysicalMeta;
    camera_metadata_t *mAuxPhysicalMeta;
    OutputMetadataFaceAnalyze mFaceAnalyze;
    int mProcessCount;
    int mOutputNum;

    BeautyFeatureParams mBeautyFeatureParams;
    MiaNodeInterface mNodeInterface;
    bool mIsSetCaliFinish; ///< avoid to set cali data repeatedly
};

PLUMA_INHERIT_PROVIDER(ArcCapBokehPlugin, PluginWraper);

PLUMA_CONNECTOR
bool connect(mialgo2::ProviderManager &host)
{
    host.add(new ArcCapBokehPluginProvider());
    return true;
}

#endif // _ARCCAPBOKEHPLUGIN_H_
