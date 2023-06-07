#ifndef __MI_NODE_INTSENSE_CAPTUREBOKEH__
#define __MI_NODE_INTSENSE_CAPTUREBOKEH__

#include <MiaMetadataUtils.h>
#include <MiaPluginUtils.h>
#include <MiaPluginWraper.h>
#include <MiaProvider.h>
#include <VendorMetadataParser.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <system/camera_metadata.h>

#include <fstream>
//#include "mianode.h"
//#include "miaimage.h"
//#include "ammem.h"
#include "Depth.h"
// #include "Dof.h"
#include "Refocus.h"
#include "bokeh.h"
#include "bokeh_base.h"
#include "bokeh_conf.h"
#include "dynamic_blur_control.h"
#include "wa_common.h"

//#include "merror.h"
#include "BeautyShot_Image_Algorithm.h"
#include "mi_portrait_3d_lighting_image.h"
#include "mi_portrait_lighting_common.h"
#include "skinbeautifierdef.h"

using namespace wa;

using namespace mialgo2;

#define BLUR_INTENCITY       65
#define CAPTURE_BOKEH_DEGREE 0
#define MAX_FACE_NUM         10
#define CTB_RESULT_OK        0
#define CTB_RESULT_FAILED    1
#define MAX_EXTRA_SIZE       256

#define CTB_INPUT_TELE 0
#define CTB_INPUT_WIDE 1

//#define _HARD_CODE_FOR_PARAMS_
#define RELIGHT_REENTRANT

enum Region {
    BEAUTYSHOT_REGION_DEFAULT = -1,
    BEAUTYSHOT_REGION_GLOBAL = 0,
    BEAUTYSHOT_REGION_CHINA = 1,
    BEAUTYSHOT_REGION_INDIAN = 2,
    BEAUTYSHOT_REGION_BRAZIL = 3
};

typedef struct tag_is_input_params_t
{
    WAOFFSCREEN mainimg;      ///< The input main image buffer
    WAOFFSCREEN auximage;     ///< The input aux image buffer
    WAOFFSCREEN outimage;     ///< The output result image buffer
    WAOFFSCREEN depthimage;   ///< The output depth buffer
    WAOFFSCREEN outteleimage; ///< The output tele buffer

    WA_REFOCUSCAMERAIMAGE_PARAM caminfo; ///< The ISP crop image param
    WA_DCIR_REFOCUS_PARAM refocus;       ///< The refocus param
    float luxIndex;
    int isoValue;
    int i32Orientation;
    float expTime;
    float ispGain;
} is_frame_params_t;

#define MI_EXAMPLE_OUTPUT_BUF_CNT 10

// disable rtdof on capturing
#define RTDOF_DISABLE_STATE "vendor.camera.bokeh.disable"
#define RTDOF_PAUSE         "1"
#define RTDOF_RESUME        "0"

class MiaNodeIntSenseCapBokeh
{
public:
    MiaNodeIntSenseCapBokeh();
    virtual ~MiaNodeIntSenseCapBokeh();
    // virtual
    int init(uint32_t cameraId);
    int deInit();
    static int processImage(void *data);
    int processBuffer(struct MiImageBuffer *teleImage, struct MiImageBuffer *wideImage,
                      struct MiImageBuffer *outputBokeh, struct MiImageBuffer *outputTele,
                      struct MiImageBuffer *outputDepth, camera_metadata_t *meta,
                      std::string &results);

private:
    void checkPersist();
    int iniEngine();
    int readCalibrationDataFromFile(const char *szFile, void *pCaliBuff, uint32_t dataSize);
    void dumpYUVData(WAOFFSCREEN *pASLIn, uint32_t index, const char *namePrefix,
                     long long timestamp);
    void dumpRawData(uint8_t *pData, int32_t size, uint32_t index, const char *namePrefix,
                     long long timestamp);
    void prepareImage(struct MiImageBuffer *image, WAOFFSCREEN &stImage);
    int setFrameParams(struct MiImageBuffer *input_tele, struct MiImageBuffer *input_wide,
                       struct MiImageBuffer *output_bokeh, struct MiImageBuffer *output_tele,
                       struct MiImageBuffer *output_depth, camera_metadata_t *metaData);
    void mergeImage(WAOFFSCREEN *pBaseImg, WAOFFSCREEN *pOverImg, int downsize);
    void mapRectToInputData(MIRECT &rtSrcRect, MIRECT &rtIFERect, int &nDataWidth, int &nDataHeight,
                            MIRECT &rtDstRectOnInputData);
    void mapFaceRectToInputData(MIRECT &rtSrcRect, MIRECT &rtIFERect, int &nDataWidth,
                                int &nDataHeight, MIRECT &rtDstRectOnInputData);
    void convertToWaFaceInfo(FDMetadataResults &fdMeta, WA_FACE_PARAM &faceinfo);
    int doProcess();
    void releaseImage(WAOFFSCREEN *pImage);
    int getFaceDegree(int faceOrientation);
    bool copyFile(const char *src, const char *dest);
    void prepareRelightImage(MIIMAGEBUFFER &dstimage, WAOFFSCREEN &srtImage);
    void processAIFilter(WAOFFSCREEN &inputTeleImage, Image &rMask);
    int processRelighting(WAOFFSCREEN &input, WAOFFSCREEN &output, WAOFFSCREEN &bokehImg,
                          Image *rMask);
    void prepareArcImage(ASVLOFFSCREEN &dstimage, WAOFFSCREEN &srtImage);
    void copyImage(WAOFFSCREEN *pSrcImg, WAOFFSCREEN *pDstImg);
    void copyWaImage(WAOFFSCREEN *pDstImage, WAOFFSCREEN *pSrcImage);
    bool readFile(const char *path, unsigned char **data, unsigned long *size);
    void updateExifInfo(std::string &results);
    Depth *mDepth = NULL;
    Refocus *mRefocus = NULL;
    bokeh::MIBOKEH_FILTER_DEPTHINFO *mDepthinfo = NULL;
    unsigned char *mCalibration;
    unsigned long mCaliDataSz;
    unsigned char *mMecpData;
    unsigned long mMecpDataSz;
    WA_DCIR_PARAM mBokehParam;
    is_frame_params_t mFrameParams;
    int mInputDataWidth;
    int mInputDataHeight;
    MIRECT mFovRectIFEForTele;
    MIRECT mFovRectIFEForWide;
    MIRECT mActiveArraySizeWide;
    MIRECT mActiveArraySizeTele;
    MIRECT mRtFocusROIOnInputData;
    bool mDumpPriv;
    int mDrawROI;
    int mViewCtrl;

    float mFNum;
    WFace mFaces[MaxFaceROIs];

    int mSoftenLevel;
    int32_t mBlurLevel;
    int mOrient;
    int mMappedOrient;
    uint32_t mCameraId;
    int mFaceOrientation;
    bool mRearCamera; ////<The InstanceId for front or real camera
    int mSceneMode;
    int mSceneEnable;
    MHandle mRelightingHandle;
    CameraRoleType mMasterRole;

    OutputMetadataFaceAnalyze mFaceAnalyze;

    int mLightingMode;
    char mModuleInfo[PROPERTY_VALUE_MAX];

    BeautyFeatureParams mBeautyFeatureParams;
    int mBeautyEnabled;
    double mDumpTimeStamp;

    int32_t mIsNonCalibAlgo;
    int32_t mForceNonCalibAlgo;
    int mProcessCount;
    double mTotalCost;
};

#endif //__MI_NODE_CAPTUREBOKEH__
