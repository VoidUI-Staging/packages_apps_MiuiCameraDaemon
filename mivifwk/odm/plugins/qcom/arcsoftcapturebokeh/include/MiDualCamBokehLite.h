#ifndef __MIDUALCAMBOKEH_H__
#define __MIDUALCAMBOKEH_H__

#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <system/camera_metadata.h>
#include <vector>

#include "BeautyShot_Image_Algorithm.h"
#include "SkinBeautifierDef.h"
#include "ammem.h"
#include "bokeh.h"
#include "dualcam_bokeh_proc.h"
#include "merror.h"
#include "mi_portrait_3d_lighting_image.h"
#include "mi_portrait_lighting_common.h"
// #include "xmi_high_dynamic_range.h"
#include "chivendortag.h"
#include "chioemvendortag.h"
#include "arcsoft_dynamic_blur_control.h"

#define CAPTURE_BOKEH_DEGREE_0   0
#define CAPTURE_BOKEH_DEGREE_270 270
#define CAPTURE_BOKEH_DEGREE_180 180

#define CTB_INPUT_TELE 0
#define CTB_INPUT_WIDE 1

#define HARDCODE_BEAUTY_FACE_NUM 2
#define FRAME_MAX_INPUT_NUM      3
#define MI_MAX_EV_NUM            12
#define MAX_FACE_NUM_BOKEH       10
#define MAX_LINKED_SESSIONS 2

using MIRECT = CHIRECT;

typedef struct tag_bokeh_params_t
{
    AlgoBokehRect ptFocus;
    AlgoBokehRect faceRect;
    AlgoBokehInterface Interface;
    AlgoBokehPoint tp;
    int sceneMode;
    float luxIndex;
} bokeh_params_t;

typedef struct
{
    int BlurLevel;
    int BeautyEnabled;
    int LightingMode;
    int SceneEnable;
    int SceneMode;
    int HdrEnabled;
} bokeh_lite_params;

enum Region {
    BEAUTYSHOT_REGION_DEFAULT = -1,
    BEAUTYSHOT_REGION_GLOBAL = 0,
    BEAUTYSHOT_REGION_CHINA = 1,
    BEAUTYSHOT_REGION_INDIAN = 2,
    BEAUTYSHOT_REGION_BRAZIL = 3
};
enum {
    HDR_PARAM_INDEX_REAR,
    HDR_PARAM_INDEX_FRONT,
};
typedef struct
{
    float lux_min;
    float lux_max;
    int checkermode;
    float confval_min;
    float confval_max;
    float adrc_min;
    float adrc_max;
} MiHDRCommonAEParam;

static MiHDRCommonAEParam MiHDRCommonAEParams[] = {
    ///(lux_min, lux_max, checkermode, c_min, c_max, drc_min, drc_max)
    // CommonAEParam
    {250, 300, 1, 0.6, 0.8, 4.0, 8.0},
};

typedef struct
{
    int tonelen;
    int brightness;
    int saturation;
    int contrast;
    int ev[MI_MAX_EV_NUM];
} MiHDRParam;

static MiHDRParam MiHDRParams[] = {
    //(tonelen, bright, saturation, contrast, r_ev0, r_ev-, r_ev+)
    // REAR camera
    {20, 0, 5, 50, {0, -10, 6}},
    // FRONT camera
    {1, 0, 0, 0, {0, -12, 0}},
};



class MiDualCamBokeh
{
public:

    virtual ~MiDualCamBokeh();


    int Init(uint32_t FrameworkCameraId);

    int ProcessLiteBuffer(std::vector<struct MiImageBuffer *> teleInputvector,
                                        std::vector<struct MiImageBuffer *>wideInputvector,
                                        struct MiImageBuffer *bokehOutput,
                                        struct MiImageBuffer *teleOutput,
                                        struct MiImageBuffer *depthOut,
                                        bokeh_lite_params &liteParam);
                                        //const std::vector<camera_metadata_t *> &logicalMetaVector);
    int deInit();

    camera_metadata_t *m_pLogicalMetaData;

    // AEC static definitions
    static const uint8_t ExposureIndexShort = 0; ///< Exposure setting index for short exposure
    static const uint8_t ExposureIndexLong = 1;  ///< Exposure setting index for long exposure
    static const uint8_t ExposureIndexSafe = 2; ///< Exposure setting index for safe exposure (between short and long)

private:
    

    int ReadCalibrationDataFromFile(const char *szFile, void *pCaliBuff, uint32_t dataSize);
    void PrepareImage(struct MiImageBuffer *image, AlgoBokehImg &stImage);
    void PrepareScreenImage(struct MiImageBuffer *image, ASVLOFFSCREEN &stImage);
    void PrepareRelightImage(MIIMAGEBUFFER &dstimage, ASVLOFFSCREEN &srtImage);
    void CopyToAlgoImage(struct MiImageBuffer *pSrcImg, AlgoBokehImg *pDstImg);
    void CopyToMiImage(AlgoBokehImg *pSrcImg, struct MiImageBuffer *pDstImg);
    void CopyScreenToMiImage(ASVLOFFSCREEN *pSrcImg, struct MiImageBuffer *pDstImg);
    void ReleaseImage(AlgoBokehImg *pImage);
    void ReleaseScreenImage(ASVLOFFSCREEN *pImage);
    int SetFrameParams(struct MiImageBuffer *teleInput, struct MiImageBuffer *wideInput,
                       camera_metadata_t *metaData);
    void MapRectToInputData(MIRECT &rtSrcRect, MIRECT &rtIFERect, int &nDataWidth, int &nDataHeight,
                            MIRECT &rtDstRectOnInputData);
    void ConvertToFaceInfo(FDMetadataResults &fdMeta, AlgoBokehRect &faceinfo);
    int DoProcess(std::vector<struct MiImageBuffer *> teleInputvector,
                  std::vector<struct MiImageBuffer *> wideInputvector,
                  struct MiImageBuffer *bokehOutput, struct MiImageBuffer *teleOutput,
                  struct MiImageBuffer *depthOut);
    int processRelighting(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, ASVLOFFSCREEN *bokehImg);
    void DrawPoint(struct MiImageBuffer *pASLIn, AlgoBokehPoint xPoint, MInt32 nPointWidth,
                   MInt32 iType);
    void DrawRectROI(struct MiImageBuffer *pOffscreen, MIRECT rt, MInt32 nLineWidth, MInt32 iType);
    void DrawRect(struct MiImageBuffer *pOffscreen, MRECT RectDraw, MInt32 nLineWidth,
                  MInt32 iType);
    MRECT intersection(const MRECT &a, const MRECT &d);
    void GetImageScale(unsigned int width, unsigned int height, ALGO_CAPTURE_BOKEH_FRAME &Scale);

    MVoid *m_pCalibrationWU;
    MVoid *m_pCalibrationWT;
    MVoid *m_pCalibrationFront;
    void *m_hCaptureBokeh;

    int m_inputDataWidth;
    int m_inputDataHeight;
    int m_focusDistance;
    int m_isInitFaild;
    int m_bDrawROI;
    int m_nViewCtrl;
    int m_HdrMode;
    int m_BeautyEnabled;
    int m_lightingMode;
    // int m_orient;
    int m_mapped_orient;
    int m_faceOrientation;
    int m_sceneMode;
    int m_sceneEnable;
    int m_OutputNum;

    uint8_t *m_pDisparityData;
    int32_t m_i32DisparityDataSize;
    int32_t m_i32DisparityDataActiveSize;
    int32_t m_blurLevel;
    uint32_t m_CameraId;

    bool m_hdrEnabled;
    bool m_bRearCamera; ////<The InstanceId for front or real camera
    bool m_bDumpYUV;
    bool m_Initialized;
    bool m_flustStatus;

    MIRECT m_fovRectIFEForTele;
    MIRECT m_fovRectIFEForWide;
    MIRECT m_activeArraySizeWide;
    MIRECT m_activeArraySizeTele;
    MIRECT m_rtFocusROIOnInputData;

    MHandle m_hRelightingHandle;
    camera_metadata_t *m_plogicalMeta;
    bokeh_params_t m_bokehframParams;
    BeautyFeatureParams mBeautyFeatureParams;
    MiaNodeInterface m_NodeInterface;
    // OutputMetadataFaceAnalyze m_faceAnalyze;
    ALGO_CAPTURE_BOKEH_ZOOM_RATIO m_zoomRatio;
    ALGO_CAPTURE_BOKEH_DEBUG m_DebugMode;

};

#endif //__MIDUALCAMBOKEH_H__