#ifndef ALLDC_V3
#define ALLDC_V3

#include "pthread.h"

#define alLDC_ERR_MODULE                   0x38000
#define alLDC_ERR_CODE                     int
#define alLDC_ERR_SUCCESS                  0x00
#define alLDC_ERR_INIT_DIM_FAIL            (alLDC_ERR_MODULE+0x00000001)
#define alLDC_ERR_BUFFER_SIZE_TOO_SMALL    (alLDC_ERR_MODULE+0x00000002)
#define alLDC_ERR_BUFFER_IS_NULL           (alLDC_ERR_MODULE+0x00000003)
#define alLDC_ERR_INVALID_ENABLE           (alLDC_ERR_MODULE+0x00000004)
#define alLDC_ERR_INVALID_VALUE            (alLDC_ERR_MODULE+0x00000005)
#define alLDC_ERR_FUNCTION_INVALID         (alLDC_ERR_MODULE+0x00000006)
#define alLDC_ERR_INVALID_ID               (alLDC_ERR_MODULE+0x00000007)
#define alLDC_ERR_TIME_EXPIRED             (alLDC_ERR_MODULE+0x00000008)
#define alLDC_ERR_FUNCTION_NOT_READY       (alLDC_ERR_MODULE+0x00000009)
#define alLDC_ERR_INVALID_DEINIT           (alLDC_ERR_MODULE+0x0000000A)
#define alLDC_ERR_REDUNDANT_INIT           (alLDC_ERR_MODULE+0x0000000B)
#define alLDC_ERR_BYPASS                   (alLDC_ERR_MODULE+0x0000000C)
#define alLDC_ERR_INVALID_PATH             (alLDC_ERR_MODULE+0x0000000D)
#define alLDC_ERR_INVALID_PATH_SIZE        (alLDC_ERR_MODULE+0x0000000E)

#define ALLDC_FPC_FACE_MAX_ROI 10
#define ALLDC_ZOOM_WOI_COUNT 20
#define ALLDC_FPC_PACKDATA_SIZE 21

typedef void (*ALLDC_BUFFER_CTRL)(void* handle, int type);

typedef enum{
    ALLDC_BUFFER_RELEASE = 0,
    ALLDC_BUFFER_USE = 1,
}ALLDC_BUFFER_STATUS;

typedef enum {
    ALLDC_DISABLE = 0,
    ALLDC_ENABLE = 1
} ALLDC_SWITCH_FLAG_T;

typedef struct{
        short x;
        short y;
        unsigned short width;
        unsigned short height;
} ALLDC_RECT;

typedef struct {
    int         count;
    ALLDC_RECT  woi[ALLDC_ZOOM_WOI_COUNT];
} ALLDC_ZOOM_WOI_T;

typedef struct {
    ALLDC_RECT  roi;
    int         score;
    int         id;
} ALLDC_FACE_RECT_T;

typedef struct  {
    unsigned char                  count;                      /*!< [IN] Number of valid face region*/
    unsigned char                  priority;                   /*!< [IN] indicate if region priority is enabled for 3A*/
    unsigned short                 src_img_width;              /*!< [IN] Source image width*/
    unsigned short                 src_img_height;             /*!< [IN] Source image height*/
    ALLDC_FACE_RECT_T              face[ALLDC_FPC_FACE_MAX_ROI];   /*!< [IN] Separate face region information*/
} ALLDC_FACE_INFO;

typedef struct {
    ALLDC_FACE_RECT_T srcRoi;
    ALLDC_FACE_RECT_T ldcRoi;
} ALLDC_ROI_SET_T;

typedef enum {
    ALLDC_MAP_ROI_SOURCE_TO_LDC = 0x01,
    ALLDC_MAP_ROI_LDC_TO_SOURCE = 0x02
} ALLDC_MAP_ROI_FLAG_T;

typedef struct {
    int                 count;
    ALLDC_ROI_SET_T * elements;
} ALLDC_ROI_MAPPING_T;

typedef struct {
    double fpcRatio;
    double fpcViewCropRatio;
    double fpcPara[ALLDC_FPC_PACKDATA_SIZE];
} ALLDC_FPC_DATA_T;

typedef enum
{
        ALLDC_CLIENT_VIDEO = 0,
        ALLDC_CLIENT_VIDEO_PREVIEW,
        ALLDC_RESERVE
} ALLDC_CLIENTID_T;

typedef struct {
    pthread_mutex_t mutex;
    int count;
    ALLDC_BUFFER_CTRL cb;

    int width;
    int height;
    unsigned long bufferSize;
    unsigned char * bufferPrt;

    ALLDC_RECT fullRec;
    ALLDC_RECT orgRec;
    ALLDC_RECT cropRec;

    // int texture_id;
    // int attach;
    // void* pContext;

} ALLDC_TABLE_T;

typedef struct {
    int flag;// output table set 1
    ALLDC_TABLE_T* ptLDCTable;

    double focalLength;
    double LDCCenterX;
    double LDCCenterY;
} ALLDC_LDC_INFO_v3_T;

typedef struct {
    ALLDC_TABLE_T tLDCTable;
    ALLDC_RECT fullRec;
    ALLDC_RECT orgRec;
    ALLDC_RECT cropRec;
    double focalLength;
    double LDCCenterX;
    double LDCCenterY;
} ALLDC_LDC_INFO_T;

typedef enum
{
    ALLDC_TEXTURE_2D = 0,
    ALLDC_TEXTURE_EXTERNAL_OES
}ALLDC_TEXTURE_FORMAT;

typedef enum
{
    ALLDC_IMAGE_FORMAT_NV21 = 0,
    ALLDC_IMAGE_FORMAT_P010
}ALLDC_IMG_FORMAT;

typedef enum{
  ALLDC_LEVEL_0 = 0x00, // normal     (Only Version number)
  ALLDC_LEVEL_1 = 0x01, // api_check  (Print all input parameters of public API)
  ALLDC_LEVEL_2 = 0x02, // profile    (Print performance log for public API call)
}ALLDC_SET_FLAG;

typedef enum{
  ALLDC_SET_PARAM_NONE =          0x00,
  ALLDC_SET_PARAM_TABLE_SIZE =    0x01, // for set Output Table Size
  ALLDC_SET_PARAM_BIN_PATH =      0x02,
}ALLDC_SET_PARAM;

typedef enum {
        DEBUG_FLAG_DISABLE                  = 0x00000000,
        DEBUG_FLAG_BYPASS_MODE              = 0x00000001,
        DEBUG_FLAG_ENABLE_WATERMARK         = 0x00000002,

        DEBUG_FLAG_LOG_API_CHECKING         = 0x00000010,
        DEBUG_FLAG_LOG_PROFILE              = 0x00000020,

        DEBUG_FLAG_DUMP_INPUT_DATA          = 0x00000100,
        DEBUG_FLAG_DUMP_OUTPUT_DATA         = 0x00000200,
        DEBUG_FLAG_ENABLE_MASK              = 0xFFFFFFFF
} ALLDC_DEBUG_FLAG;


#pragma pack(push,4)
typedef struct{
        ALLDC_SET_PARAM type;

    union{
        int m_outTableSize;
        char m_cBinPath[512];
    }u;

}ALLDC_SET_PARAMETER_T;
#pragma pack(pop)


#ifdef __cplusplus
extern "C" {
#endif

alLDC_ERR_CODE alLDC_VersionInfo_Get(void *a_pOutBuf, int a_dInBufMaxSize);

alLDC_ERR_CODE alLDC_Set_Parameters(ALLDC_SET_PARAMETER_T *para);

// texture input / output
alLDC_ERR_CODE alLDC_Texture_Init(
    int a_dInImgW, int a_dInImgH,
    int a_dOutWidth, int a_dOutHeight,
    unsigned char *a_pLDCBuffer, unsigned int a_udLDCBufferSize,
    unsigned char *a_pFPCBuffer, unsigned int a_udFPCBufferSize);

alLDC_ERR_CODE alLDC_Texture_Mode_Init(
        int a_dInImgW, int a_dInImgH,
        int a_dOutWidth, int a_dOutHeight,
        int a_input_texture_mode, int a_output_texture_mode,
        unsigned char *a_pLDCBuffer, unsigned int a_udLDCBufferSize,
        unsigned char *a_pFPCBuffer, unsigned int a_udFPCBufferSize);

alLDC_ERR_CODE alLDC_Texture_Run(
    int a_dInputTexId, int a_dOutputTexId,
    int a_dInputRotate, float *a_pfMvpMatrix,
    ALLDC_RECT *a_dInZoomWOI, ALLDC_FACE_INFO *a_ptInFaceInfo);

alLDC_ERR_CODE alLDC_Texture_BindFBO(int a_dOutputTexId);

alLDC_ERR_CODE alLDC_Texture_Deinit();


// buffer input / output
/* alLDC_Buffer_Init
* To initialize LDC library
* a_dID: Id of Camera thread. 0 for video, 1 for preview.
* a_dInImgW, a_dInImgH: non-strid image size
* a_dOutWidth, a_dOutHeight: strided image size
* a_dStrideWidth, a_dStrideHeight: strided image size
* a_dOutStrideWidth, a_dOutStrideHeight: output strided image size
* a_nImgFormat: to set input image (when run)'s format, refers to enum ALLDC_IMG_FORMAT
* a_pLDCBuffer, a_udLDCBufferSize: LDC packdata and data size.
* a_pFPCBuffer, a_udFPCBufferSize: FPC's tuning data and data size.
*/
alLDC_ERR_CODE alLDC_Buffer_Init(
        int a_dID,
        int a_dInImgW, int a_dInImgH,
        int a_dOutWidth, int a_dOutHeight,
        int a_dStrideWidth, int a_dStrideHeight,
        int a_dOutStrideWidth, int a_dOutStrideHeight,
        ALLDC_IMG_FORMAT a_nImgFormat,
        unsigned char *a_pLDCBuffer, unsigned int a_udLDCBufferSize,
        unsigned char *a_pFPCBuffer, unsigned int a_udFPCBufferSize);

/* alLDC_Buffer_Run
* To run LDC and get result images.
* a_dID: Id of Camera thread, Make sure the ID is initialize by alLDC_Buffer_Init
* a_pucInImage_Y, a_pucInImage_UV: Input Image buffer the format in which was set when initializaiton.
* a_pucOutImaghe_Y, a_pucOutImage_UV: Output reslut image buffer. the format is similar to input.
* a_dInputRotate: set 0 if input image is not rotated. 1 for rotated input image.
* a_pfMvpMatrix: set nullptr if the output result is not expected to rotate. Otherwise, input the matrix.
* a_ptInZoomWOI: Input the WOI of current Zoom level. refers to woi structure ALLDC_RECT.
* a_ptInFaceInfo: Input face data by strcuture ALLDC_FACE_INFO.
* a_ptOutLdcInfo: Get output of LDC table information in strucuture ALLDC_LDC_INFO_v3_T.
*                 Set flag=1 if expected to get lastest table in this run. And will get LDC table buffer point.
*/
alLDC_ERR_CODE alLDC_Buffer_Run(
        int a_dID,
        unsigned char *a_pucInImage_Y, unsigned char *a_pucInImage_UV,
        unsigned char *a_pucOutImage_Y, unsigned char *a_pucOutImage_UV,
        int a_dInputRotate, float *a_pfMvpMatrix,
        ALLDC_RECT *a_ptInZoomWOI, ALLDC_FACE_INFO *a_ptInFaceInfo,
        ALLDC_LDC_INFO_v3_T *a_ptOutLdcInfo);

/* alLDC_Buffer_Deinit
* To release all souces and memory of alLDC
* a_dID: Id of Camera thread, Make sure the ID is initialize by alLDC_Buffer_Init
*/
alLDC_ERR_CODE alLDC_Buffer_Deinit(int a_dID);

/* alLDC_Map_Location_v3
* To Map input roi by the table a_dID camera used, get output roi
* a_dID: Id of Camera thread, Make sure this camera ID is alread be run
* a_ptInOutRoi: input source roi and get output mapped roi.
* a_nflag: function avaliable selection, refers to alLDC_MP_ROI_FALG_T.
*/
alLDC_ERR_CODE alLDC_Map_Location_v3(int a_dID, ALLDC_ROI_MAPPING_T * a_ptInOutRoi, ALLDC_MAP_ROI_FLAG_T a_nflag);

/* alLDC_Get_Current_FPC_Ratio_v3
* To get lastest FPC information of a_dID camera thread after alLDC_Buffer_Run.
* a_dID: Id of Camera thread, Make sure this camera ID is alread be run
* a_ptOutFpcData: Output latest fpc information after alLDC_Buffer_Run, refers to ALLDC_FPC_DATA_T
*/
alLDC_ERR_CODE alLDC_Get_Current_FPC_Ratio_v3(int a_dID, ALLDC_FPC_DATA_T * a_ptOutFpcData);

/* alLDC_Manual_FPC_Ratio_v3
* To manual mode for inputing fpc ratio manually, always used when tuning.
* a_dID: Id of Camera thread, Make sure this camera ID is alread be run
* a_nflag: 0 for disable manual mode, 1 for enable manual mode.
* a_eInFpcRatio: alLDC will use this value when manual fpc ratio is enable (a_nflag = 1)
*/
alLDC_ERR_CODE alLDC_Manual_FPC_Ratio_v3(int a_dID, ALLDC_SWITCH_FLAG_T a_nflag, double a_eInFpcRatio);

/* alLDC_Set_Flag
* To set specific mode, function is refered to ALLDC_SET_FLAG
* a_udFlag: 0 : log-off
            1:  api_check
            2:  profile log
*/
alLDC_ERR_CODE alLDC_Set_Flag(unsigned int a_udFlag);



/** Backward compatible fucntions **/

alLDC_ERR_CODE alLDC_Video_Init_v2(int a_dID, int a_dInImgW, int a_dInImgH, int a_dInImgW_stride, int a_dInImgH_stride,
                                    int a_dOutWidth, int a_dOutHeight, ALLDC_ZOOM_WOI_T *a_ptInZoomWoi, int a_dInDefaultZoomLevel,
                                    unsigned char *a_pLDCBuffer, unsigned int a_udLDCBufferSize,
                                    unsigned char *a_pFPCBuffer, unsigned int a_udFPCBufferSize);

alLDC_ERR_CODE alLDC_Video_Run_v2(int a_dID, unsigned char *a_pucInputYImage, unsigned char *a_pucInputCbCrImage,
                                unsigned char *a_pucOutputYImage, unsigned char *a_pucOutputCbCrImage,
                                int a_dRotation, float *a_pfMvpMatrix, int a_dInZoomLevel, ALLDC_FACE_INFO *a_ptInFaceInfo);

alLDC_ERR_CODE alLDC_Video_Deinit_v2(int a_dID);


alLDC_ERR_CODE alLDC_Video_Init_v2_SAT(int a_dID, int a_dInImgW, int a_dInImgH, int a_dInImgW_stride, int a_dInImgH_stride,
                                int a_dOutWidth, int a_dOutHeight, ALLDC_ZOOM_WOI_T *a_ptInZoomWoi, int a_dInDefaultZoomLevel,
                                unsigned char *a_pLDCBuffer, unsigned int a_udLDCBufferSize,
                                unsigned char *a_pFPCBuffer, unsigned int a_udFPCBufferSize);

alLDC_ERR_CODE alLDC_Video_Run_v2_SAT(int a_dID, unsigned short *a_pucInputYImage, unsigned short *a_pucInputCbCrImage,
                                unsigned short *a_pucOutputYImage, unsigned short *a_pucOutputCbCrImage,
                                int a_dRotation, float *a_pfMvpMatrix, int a_dInZoomLevel, ALLDC_FACE_INFO *a_ptInFaceInfo,
                                ALLDC_LDC_INFO_T * a_ptOutLdcInfo);

alLDC_ERR_CODE alLDC_Video_Deinit_v2_SAT(int a_dID);


alLDC_ERR_CODE alLDC_Map_Location(ALLDC_ROI_MAPPING_T * a_ptInOutRoi, ALLDC_MAP_ROI_FLAG_T flag);

alLDC_ERR_CODE alLDC_Get_Current_FPC_Ratio(ALLDC_FPC_DATA_T * a_ptOutFpcData);

alLDC_ERR_CODE alLDC_Manual_FPC_Ratio(ALLDC_SWITCH_FLAG_T flag, double a_eInFpcRatio);

alLDC_ERR_CODE alLDC_Map_Location_SAT(ALLDC_ROI_MAPPING_T * a_ptInOutRoi, ALLDC_MAP_ROI_FLAG_T flag);

alLDC_ERR_CODE alLDC_Get_Current_FPC_Ratio_SAT(ALLDC_FPC_DATA_T * a_ptOutFpcData);

alLDC_ERR_CODE alLDC_Manual_FPC_Ratio_SAT(ALLDC_SWITCH_FLAG_T flag, double a_eInFpcRatio);

alLDC_ERR_CODE alLDC_Set_Debug_Path(char *a_pcInDebugPath, int a_dInDebugPathSize);
alLDC_ERR_CODE alLDC_Set_Debug_Flag(ALLDC_DEBUG_FLAG a_dDebugFlag);
alLDC_ERR_CODE alLDC_Set_Debug_Persist(char* a_pcPersistString, int a_dInPStringSize);


#ifdef __cplusplus
}
#endif

#endif //ALLDC_V3
