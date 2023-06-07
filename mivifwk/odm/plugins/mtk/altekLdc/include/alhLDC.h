#ifndef ALHLDC
#define ALHLDC

/*
PROPRIETARY STATEMENT:
THIS FILE (OR DOCUMENT) CONTAINS INFORMATION CONFIDENTIAL AND
PROPRIETARY TO ALTEK CORPORATION AND SHALL NOT BE REPRODUCED
OR TRANSFERRED TO OTHER FORMATS OR DISCLOSED TO OTHERS.
ALTEK DOES NOT EXPRESSLY OR IMPLIEDLY GRANT THE RECEIVER ANY
RIGHT INCLUDING INTELLECTUAL PROPERTY RIGHT TO USE FOR ANY
SPECIFIC PURPOSE, AND THIS FILE (OR DOCUMENT) DOES NOT
CONSTITUTE A FEEDBACK TO ANY KIND.
*/

#include "alhLDC_Err.h"
#include "alhLDC_def.h"

#define alhLDC_ALFPC_FACE_MAX_ROI 10

typedef struct
{
    alhLDC_RECT roi;
    int score;
    int id;
} alhLDC_FACE_RECT_T;

typedef struct
{
    unsigned char count;           /*!< [IN] Number of valid face region*/
    unsigned char priority;        /*!< [IN] indicate if region priority is enabled for 3A*/
    unsigned short src_img_width;  /*!< [IN] Source image width*/
    unsigned short src_img_height; /*!< [IN] Source image height*/
    alhLDC_FACE_RECT_T face[alhLDC_ALFPC_FACE_MAX_ROI]; /*!< [IN] Separate face region information*/
} alhLDC_FACE_INFO;

typedef struct
{
    int width;
    int height;
    unsigned long bufferSize;
    unsigned char *bufferPrt;
} alhLDC_TABLE_T;

typedef enum {
    alhLDC_DEBUG_FLAG_DISABLE = 0x00000000,
    alhLDC_DEBUG_FLAG_BYPASS_MODE = 0x00000001,
    alhLDC_DEBUG_FLAG_ENABLE_WATERMARK = 0x00000002,

    alhLDC_DEBUG_FLAG_LOG_API_CHECKING = 0x00000010,
    alhLDC_DEBUG_FLAG_LOG_PROFILE = 0x00000020,

    alhLDC_DEBUG_FLAG_DUMP_INPUT_DATA = 0x00000100,
    alhLDC_DEBUG_FLAG_DUMP_OUTPUT_DATA = 0x00000200,
    alhLDC_DEBUG_FLAG_ENABLE_MASK = 0xFFFFFFFF
} alhLDC_DEBUG_FLAG;

#ifdef __cplusplus
extern "C" {
#endif
alhLDC_ERR_CODE alhLDC_VersionInfo_Get(void *a_pOutBuf, int a_dInBufMaxSize);

alhLDC_ERR_CODE alhLDC_Set_LogLevel(alhLDC_SET_LOGLV a_udFlag);

alhLDC_ERR_CODE alhLDC_Set_KernelMethod(alhLDC_SET_KERNEL_METHOD a_dFlag);

alhLDC_ERR_CODE alhLDC_Set_ImageFormat(alhLDC_IMG_FORMAT a_img_fmt);

alhLDC_ERR_CODE alhLDC_Init(int a_dInImgW, int a_dInImgH, int a_dOutWidth, int a_dOutHeight,
                            int a_dStrideWidth, int a_dStrideHeight, int a_dOutStrideWidth,
                            int a_dOutStrideHeight, unsigned char *a_pLDCBuffer,
                            unsigned int a_udLDCBufferSize, unsigned char *a_pFPCBuffer,
                            unsigned int a_udFPCBufferSize);

alhLDC_ERR_CODE alhLDC_Run(unsigned char *a_pucInImage_Y, unsigned char *a_pucInImage_UV,
                           unsigned char *a_pucOutImage_Y, unsigned char *a_pucOutImage_UV,
                           int a_dInputRotate, float *a_pfMvpMatrix, alhLDC_RECT *a_ptInZoomWOI,
                           alhLDC_FACE_INFO *a_ptInFaceInfo);

alhLDC_ERR_CODE alhLDC_Deinit();

// these function sould call before alhLDC_Init
alhLDC_ERR_CODE alhLDC_Set_Debug_Path(char *a_pcInDebugPath, int a_dInDebugPathSize);
alhLDC_ERR_CODE alhLDC_Set_Debug_Flag(alhLDC_DEBUG_FLAG a_dDebugFlag);
alhLDC_ERR_CODE alhLDC_Set_Debug_Persist(char *a_pcPersistString, int a_dInPStringSize);

#ifdef __cplusplus
}
#endif

#endif // ALHLDC