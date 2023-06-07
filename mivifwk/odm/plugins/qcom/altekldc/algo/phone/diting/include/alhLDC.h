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

#define alhLDC_ERR_MODULE                   0x39000
#define alhLDC_ERR_CODE                     int
#define alhLDC_ERR_SUCCESS                  0x00
#define alhLDC_ERR_INIT_DIM_FAIL            (alhLDC_ERR_MODULE+0x00000001)
#define alhLDC_ERR_BUFFER_SIZE_TOO_SMALL    (alhLDC_ERR_MODULE+0x00000002)
#define alhLDC_ERR_BUFFER_IS_NULL           (alhLDC_ERR_MODULE+0x00000003)
#define alhLDC_ERR_INVALID_ENABLE           (alhLDC_ERR_MODULE+0x00000004)
#define alhLDC_ERR_INVALID_VALUE            (alhLDC_ERR_MODULE+0x00000005)
#define alhLDC_ERR_FUNCTION_INVALID         (alhLDC_ERR_MODULE+0x00000006)
#define alhLDC_ERR_INVALID_ID               (alhLDC_ERR_MODULE+0x00000007)
#define alhLDC_ERR_TIME_EXPIRED             (alhLDC_ERR_MODULE+0x00000008)
#define alhLDC_ERR_NO_INITIAL               (alhLDC_ERR_MODULE+0x00000009)
#define alhLDC_ERR_CL_INITIAL               (alhLDC_ERR_MODULE+0x0000000A)
#define alhLDC_ERR_INVALID_IN_OUT           (alhLDC_ERR_MODULE+0x0000000B)
#define alhLDC_ERR_INVALID_OPERATION        (alhLDC_ERR_MODULE+0x0000000C)

#ifndef ALHLDCV2
#define ALHLDCV2

#define alhLDC_ALFPC_FACE_MAX_ROI 10

typedef struct{
    unsigned short x;
    unsigned short y;
    unsigned short width;
    unsigned short height;
} alhLDC_RECT;

typedef struct {
    alhLDC_RECT  roi;
    int         score;
    int         id;
} alhLDC_FACE_RECT_T;

typedef struct  {
    unsigned char                  count;                      /*!< [IN] Number of valid face region*/
    unsigned char                  priority;                   /*!< [IN] indicate if region priority is enabled for 3A*/
    unsigned short                 src_img_width;              /*!< [IN] Source image width*/
    unsigned short                 src_img_height;             /*!< [IN] Source image height*/
    alhLDC_FACE_RECT_T              face[alhLDC_ALFPC_FACE_MAX_ROI];   /*!< [IN] Separate face region information*/
} alhLDC_FACE_INFO;

typedef struct {
    int width;
    int height;
    unsigned long bufferSize;
    unsigned char * bufferPrt;
} alhLDC_TABLE_T;

typedef enum{
  ALHLDC_LEVEL_0 = 0x00, // normal     (Only Version number)
  ALHLDC_LEVEL_1 = 0x01, // api_check  (Print all input parameters of public API)
  ALHLDC_LEVEL_2 = 0x02, // profile    (Print performance log for public API call)
}alhLDC_SET_LOGLV;

typedef enum{
    ALHLDC_METHOD_DEFAULT = 0, // default kernel method
    ALHLDC_METHOD_MEM_OPT,     // kernel method of memory optimization
}alhLDC_SET_KERNEL_METHOD;

#endif

#ifdef __cplusplus
extern "C" {
#endif
    alhLDC_ERR_CODE alhLDC_VersionInfo_Get(void *a_pOutBuf, int a_dInBufMaxSize);

    alhLDC_ERR_CODE alhLDC_Set_LogLevel(alhLDC_SET_LOGLV a_udFlag);

    alhLDC_ERR_CODE alhLDC_Set_KernelMethod(alhLDC_SET_KERNEL_METHOD a_dFlag);

    alhLDC_ERR_CODE alhLDC_Init(
        int a_dInImgW, int a_dInImgH,
        int a_dOutWidth, int a_dOutHeight,
        int a_dStrideWidth, int a_dStrideHeight,
        int a_dOutStrideWidth, int a_dOutStrideHeight,
        unsigned char *a_pLDCBuffer, unsigned int a_udLDCBufferSize,
        unsigned char *a_pFPCBuffer, unsigned int a_udFPCBufferSize);

    alhLDC_ERR_CODE alhLDC_Run(
        unsigned char *a_pucInImage_Y, unsigned char *a_pucInImage_UV,
        unsigned char *a_pucOutImage_Y, unsigned char *a_pucOutImage_UV,
        int a_dInputRotate, float *a_pfMvpMatrix,
        alhLDC_RECT *a_ptInZoomWOI, alhLDC_FACE_INFO *a_ptInFaceInfo);

    alhLDC_ERR_CODE alhLDC_Deinit();


#ifdef __cplusplus
    }
#endif

#endif  //ALHLDC
