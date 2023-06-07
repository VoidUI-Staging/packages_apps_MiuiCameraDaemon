//*****************************************************************************
//  Filename        : capture_ldc_proc.h
//  Content         : Interface of capture LDC.
//  Created         : 2021/04/12
//  Authors         : zhangrui (zhangrui32@xiaomi.com)
//  Copyright 2021 Xiaomi Mobile Software Co., Ltd. All Rights reserved.
//*****************************************************************************

#ifndef _CAPTURE_LDC_PROC_H_
#define _CAPTURE_LDC_PROC_H_

#define CAPTURE_LDC_FACE_MAX                (10)

enum CAPTURE_LDC_IMG_FORMAT
{
    CAPTURE_LDC_IMG_NV12 = 0,
    CAPTURE_LDC_IMG_NV21 = 1,
    CAPTURE_LDC_IMG_P010 = 2,
};

enum CAPTURE_LDC_MODE
{
    CAPTURE_LDC_MODE_DEFAULT = 0,
    CAPTURE_LDC_MODE_HYBRID  = 1
};

typedef struct
{
    void *pData;                            // image data header address
    int fd;                                 // ion buffer fd information

} CaptureLDCImg;

typedef struct
{
    unsigned short x;                       // x coordinates of the top left corner of the rectangle
    unsigned short y;                       // y coordinates of the top left corner of the rectangle
    unsigned short width;                   // width in pixels
    unsigned short height;                  // height in pixels
} CaptureLDCRect;

typedef struct
{
    int score;                              // face score
    CaptureLDCRect roi;                     // face rect
} CaptureLDCFaceRectEx;

typedef struct
{
    unsigned char fNum;                     // face number
    CaptureLDCFaceRectEx face[CAPTURE_LDC_FACE_MAX];
} CaptureLDCFaceInfo;

enum CAPTURE_LDC_ORIENTATION
{
    CAPTURE_LDC_ORIENTATION_0   = 0,
    CAPTURE_LDC_ORIENTATION_90  = 90,
    CAPTURE_LDC_ORIENTATION_180 = 180,
    CAPTURE_LDC_ORIENTATION_270 = 270
};

typedef struct
{
    int imgWidth;                           // [in] image width
    int imgHeight;                          // [in] image height
    int inImgStrideW;                       // [in] input image stride (width)
    int inImgStrideH;                       // [in] input image stride (height)
    int outImgStrideW;                      // [in] output image stride (width)
    int outImgStrideH;                      // [in] output image stride (height)
    unsigned char *pCalibBuf;               // [in] calib data buffer
    char *jsonFileName;                     // [in] json file name
    CAPTURE_LDC_IMG_FORMAT inImgFmt;        // [in] input image format
} CaptureLDCLaunchParams;

typedef struct
{
    int maskWidth;                          // [in] input mask width
    int maskHeight;                         // [in] input mask height
    CaptureLDCImg *pImgInY;                 // [in] input image Y channel
    CaptureLDCImg *pImgInUV;                // [in] input image UV channel
    CaptureLDCRect zoomWOI;                 // [in] WOI rect of current zoom level
    CaptureLDCFaceInfo faceInfo;            // [in] face data struct
    CAPTURE_LDC_ORIENTATION imgOrientation; // [in] input image orientation, 0 if Default mode
    CaptureLDCImg *pPotraitMask;            // [in] potrait mask for HybridLDC, NULL if Default mode
} CaptureLDCInitParams;

/**
* @brief Get algorithm version (no return value)
*/
void MIALGO_CaptureLDCVersionGet();

/**
* @brief Init algorithm when launch camera
* @param[in] phEngine           The algorithm engine
* @param[in] pstLaunchParams    The algorithm launch params struct
* @return 0 if success, otherwise fail
*/
int MIALGO_CaptureLDCInitWhenLaunch( void **phEngine, CaptureLDCLaunchParams *pstLaunchParams );

/**
* @brief Init algorithm before every capture
* @param[in] phEngine           The algorithm engine
* @param[in] pstInitParams      The algorithm init params struct
* @return no return value
*/
void MIALGO_CaptureLDCInitEachPic( void **phEngine, CaptureLDCInitParams *pstInitParams );

/**
* @brief Algorithm process
* @param[in] phEngine           The algorithm engine
* @param[out] pImgOutY          Output image Y channel
* @param[out] pImgOutUV         Output image UV channel
* @return 0 if success, otherwise fail
*/
int MIALGO_CaptureLDCProc( void **phEngine, CaptureLDCImg *pImgOutY, CaptureLDCImg *pImgOutUV );

/**
* @brief Uninit algorithm process
* @param[in] phEngine           The algorithm engine
* @return no return value
*/
void MIALGO_CaptureLDCDestoryEachPic( void **phEngine );

/**
* @brief Uninit algorithm when close camera
* @param[in] phEngine           The algorithm engine
* @return 0 if success, otherwise fail
*/
int MIALGO_CaptureLDCDestoryWhenClose( void **phEngine );

#endif // !_CAPTURE_LDC_PROC_H_

