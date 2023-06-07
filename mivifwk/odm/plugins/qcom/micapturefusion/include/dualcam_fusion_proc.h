//*****************************************************************************
//  Filename        : dualcam_fusion_proc.h
//  Content         : Interface of fusion.
//  Created         : 2020/11/13
//  Authors         : zhangrui (zhangrui32@xiaomi.com)
//  Copyright 2021 Xiaomi Mobile Software Co., Ltd. All Rights reserved.
//*****************************************************************************

#ifndef _DUALCAM_FUSION_PROC_H_
#define _DUALCAM_FUSION_PROC_H_

enum ALGO_FUSION_IMG_FORMAT {
    FUSION_FORMAT_NV12 = 1,
    FUSION_FORMAT_NV21 = 2,
};

typedef struct
{
    int x; // x coordinates of the point
    int y; // y coordinates of the point
} AlgoFusionPt2i;

typedef struct
{
    int x;      // x coordinates of the top left corner of the rectangle
    int y;      // y coordinates of the top left corner of the rectangle
    int width;  // width in pixels
    int height; // height in pixels
} AlgoFusionRect;

typedef struct
{
    int width;                     // image width
    int height;                    // image height
    int strideW;                   // image stride (width)
    int strideH;                   // image stride (height)
    void *pData;                   // image data header address
    int fd;                        //
    ALGO_FUSION_IMG_FORMAT format; // image format
} AlgoFusionImg;

typedef struct
{
    float zoomRatio; // user zoom Ratio
    float mainScale; // main camera scale level
    float auxScale;  // aux camera scale level
} AlgoFusionScaleParams;

typedef struct
{
    float distance;          // focus distance
    float luma;              // environment luma
    bool isAfStable;         // auto focus stable or not
    AlgoFusionRect cropRect; // mainImg crop region from sensor
} AlgoFusionCameraMetaData;

typedef struct
{
    AlgoFusionScaleParams scale;   // scale params
    AlgoFusionCameraMetaData main; // main camera metadata (large FOV)
    AlgoFusionCameraMetaData aux;  // aux camera metadata (small FOV)
} AlgoFusionCameraParams;

typedef struct
{
    int mainImgW;             // [in] image width
    int mainImgH;             // [in] image height
    int auxImgW;              // [in] image width
    int auxImgH;              // [in] image height
    char *jsonFileName;       // [in] json file name
    unsigned char *pCalibBuf; // [in] calib data buffer
} AlgoFusionLaunchParams;

typedef struct
{
    AlgoFusionImg *pMainImg;             // [in] input main image (large FOV)
    AlgoFusionImg *pAuxImg;              // [in] input aux image (small FOV)
    AlgoFusionCameraParams cameraParams; // [in] input camera params
    AlgoFusionPt2i fp;                   // [in] input focus point
} AlgoFusionInitParams;

/**
 * @brief Get algorithm version (no return value)
 */
void DualCamVersion_Fusion();

/**
 * @brief Init algorithm when launch camera
 * @param[in] phEngine           The algorithm engine
 * @param[in] pstLaunchParams    The algorithm launch params struct
 * @return 0 if success, otherwise fail
 */
int MIALGO_FusionInitWhenLaunch(void **phEngine, AlgoFusionLaunchParams *pstLaunchParams);

/**
 * @brief Init algorithm before every capture
 * @param[in] phEngine           The algorithm engine
 * @param[in] pstInitParams      The algorithm init params struct
 * @return 0 if success, otherwise fail
 */
int MIALGO_FusionInitEachPic(void **phEngine, AlgoFusionInitParams *pstInitParams);

/**
 * @brief Algorithm process
 * @param[in] phEngine           The algorithm engine
 * @param[out] pImgResult        Output image result
 * @return 0 if success, otherwise fail
 */
int MIALGO_FusionProc(void **phEngine, AlgoFusionImg *pImgResult);

/**
 * @brief Uninit algorithm process
 * @param[in] phEngine           The algorithm engine
 * @return 0 if success, otherwise fail
 */
int MIALGO_FusionDestoryEachPic(void **phEngine);

/**
 * @brief Uninit algorithm when close camera
 * @param[in] phEngine           The algorithm engine
 * @return 0 if success, otherwise fail
 */
int MIALGO_FusionDestoryWhenClose(void **phEngine);

#endif // !_DUALCAM_FUSION_PROC_H_
