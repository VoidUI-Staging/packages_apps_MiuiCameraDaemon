#ifndef ALAILDC
#define ALAILDC

#define alAILDC_ERR_MODULE                   0x38000
#define alAILDC_ERR_CODE                     int
#define alAILDC_ERR_SUCCESS                  0x00
#define alAILDC_ERR_INIT_DIM_FAIL            (alAILDC_ERR_MODULE+0x00000001)
#define alAILDC_ERR_BUFFER_SIZE_TOO_SMALL    (alAILDC_ERR_MODULE+0x00000002)
#define alAILDC_ERR_BUFFER_IS_NULL           (alAILDC_ERR_MODULE+0x00000003)
#define alAILDC_ERR_INVALID_ENABLE           (alAILDC_ERR_MODULE+0x00000004)
#define alAILDC_ERR_INVALID_VALUE            (alAILDC_ERR_MODULE+0x00000005)
#define alAILDC_ERR_INVALID_OPERATION        (alAILDC_ERR_MODULE+0x00000006)
#define alAILDC_ERR_INVALID_ID               (alAILDC_ERR_MODULE+0x00000007)
#define alAILDC_ERR_TIME_EXPIRED             (alAILDC_ERR_MODULE+0x00000008)
#define alAILDC_ERR_FUNCTION_NOT_READY       (alAILDC_ERR_MODULE+0x00000009)

#define ALAIFPC_FACE_MAX_ROI 20

typedef struct{
    unsigned short x;
    unsigned short y;
    unsigned short width;
    unsigned short height;
} alAILDC_RECT;

typedef struct {
    alAILDC_RECT  roi;
    int            score;
    int            id;
} alAILDC_FACE_RECT_T;

typedef struct  {
    unsigned char                  count;                        /*!< [IN] Number of valid face region*/
    unsigned char                  priority;                     /*!< [IN] indicate if region priority is enabled for 3A*/
    unsigned short                 src_img_width;                /*!< [IN] Source image width*/
    unsigned short                 src_img_height;               /*!< [IN] Source image height*/
    alAILDC_FACE_RECT_T            face[ALAIFPC_FACE_MAX_ROI];   /*!< [IN] Separate face region information*/
} alAILDC_FACE_INFO;

typedef struct {
    alAILDC_FACE_RECT_T srcRoi;
    alAILDC_FACE_RECT_T ldcRoi;
} alAILDC_ROI_SET_T;

typedef enum {
    alAILDC_MAP_ROI_SOURCE_TO_LDC = 0x01,
    alAILDC_MAP_ROI_LDC_TO_SOURCE = 0x02
} alAILDC_MAP_ROI_FLAG_T;

typedef struct {
    int                  count;
    alAILDC_ROI_SET_T * elements;
} alAILDC_ROI_MAPPING_T;

typedef struct {
    int width;
    int height;
    unsigned long bufferSize;
    unsigned char * bufferPrt;
} alAILDC_TABLE_T;

typedef struct {
    int flag;// output table set 1
    alAILDC_TABLE_T tLDCTable;
    alAILDC_RECT fullRec;
    alAILDC_RECT orgRec;
    alAILDC_RECT cropRec;
    double focalLength;
    double LDCCenterX;
    double LDCCenterY;
} alAILDC_LDC_INFO_T;

typedef enum {
    alAILDC_IMAGE_FORMAT_NV21 = 0,
    alAILDC_IMAGE_FORMAT_NV12 = 1,    
    alAILDC_IMAGE_FORMAT_P010_MSB = 2,
    alAILDC_IMAGE_FORMAT_P010_LSB = 3
} alAILDC_IMG_FORMAT;

typedef enum{
    alAILDC_LEVEL_0 = 0x00, // normal     (Only Version number)
    alAILDC_LEVEL_1 = 0x01, // api_check  (Print all input parameters of public API)
    alAILDC_LEVEL_2 = 0x02, // profile    (Print performance log for public API call)
} alAILDC_SET_FLAG;

typedef enum{
    alAILDC_SCREEN_ANGLE_0 = 0,
    alAILDC_SCREEN_ANGLE_90,
    alAILDC_SCREEN_ANGLE_180,
    alAILDC_SCREEN_ANGLE_270
} alAILDC_SET_SCREEN_ORIENTATION;

typedef enum {
    alAILDC_PARA_EDIT_TYPE_NULL = 0,                                  /*No effect*/
    alAILDC_PARA_EDIT_TYPE_CORRECTION_STRENGTH = 1,                /*SIZE_FLOAT, 1.0~180.0*/
    alAILDC_PARA_EDIT_TYPE_BACKGROUND_DISTORTION_TOLERANCE = 2, /*SIZE_FLOAT, 1.0~180.0, <= correction strength*/
    alAILDC_PARA_EDIT_TYPE_MAX
} alAILDC_SET_TUNE_PARA;

typedef enum {
    alAILDC_DEBUG_FLAG_DISABLE                   = 0x00000000,
    alAILDC_DEBUG_FLAG_BYPASS_MODE               = 0x00000001, /*Bypass all API processing*/
    alAILDC_DEBUG_FLAG_ENABLE_WATERMARK          = 0x00000002, /*Injection watermark in output image*/
    alAILDC_DEBUG_FLAG_LOG_API_CHECKING          = 0x00000010, /*Print all input parameters and API name*/
    alAILDC_DEBUG_FLAG_LOG_PROFILE               = 0x00000020, /*Print all API processing time*/
    alAILDC_DEBUG_FLAG_DUMP_INPUT_DATA           = 0x00000100, /*Dump input image, tuning data, model data, external mask and parameters*/
    alAILDC_DEBUG_FLAG_DUMP_OUTPUT_DATA          = 0x00000200, /*Dump processed output image only*/
    alAILDC_DEBUG_FLAG_ENABLE_MASK               = 0xFFFFFFFF
} alAILDC_DEBUG_FLAG;


#ifdef __cplusplus
extern "C" {
#endif

alAILDC_ERR_CODE alAILDC_VersionInfo_Get(void *a_pOutBuf, int a_dInBufMaxSize);

/* alAILDC_Set_Working_Path
* Set working path for GPU program binary file
*/
alAILDC_ERR_CODE alAILDC_Set_Working_Path(char *a_pcInWorkingPath, int a_dInWorkingPathSize);

/* alAILDC_Set_Debug_Path
* Set working path for debug information and dump data
*/
alAILDC_ERR_CODE alAILDC_Set_Debug_Path(char *a_pcInDebugPath, int a_dInDebugPathSize);

/* alAILDC_Set_Debug_Persist
* Set debug persist
*/
alAILDC_ERR_CODE alAILDC_Set_Debug_Persist(char *a_pcInDebugPersist, int a_dInDebugPersistSize);

/* alAILDC_Buffer_Init
* To initialize AILDC library (using subject mask from user setting)
* a_dID: instance id
* a_dInImgW, a_dInImgH: non-strid image size
* a_dOutWidth, a_dOutHeight: strided image size
* a_dStrideWidth, a_dStrideHeight: strided image size
* a_dOutStrideWidth, a_dOutStrideHeight: output strided image size
* a_nImgFormat: to set input image (when run)'s format, refers to enum alAILDC_IMG_FORMAT
* a_pLDCBuffer, a_udLDCBufferSize: LDC packdata and data size.
* a_pFPCBuffer, a_udFPCBufferSize: FPC's tuning data and data size.
*/
alAILDC_ERR_CODE alAILDC_Buffer_Init(
        int a_dID,
        int a_dInImgW, int a_dInImgH,
        int a_dOutWidth, int a_dOutHeight,
        int a_dStrideWidth, int a_dStrideHeight,
        int a_dOutStrideWidth, int a_dOutStrideHeight,
        alAILDC_IMG_FORMAT a_nImgFormat,
        unsigned char *a_pLDCBuffer, unsigned int a_udLDCBufferSize,
        unsigned char *a_pFPCBuffer, unsigned int a_udFPCBufferSize);

/* alAILDC_Buffer_Init_v2
* To initialize AILDC library (using subject mask which is generated by AILDC library)
* a_dID: instance id
* a_dInImgW, a_dInImgH: non-strid image size
* a_dOutWidth, a_dOutHeight: strided image size
* a_dStrideWidth, a_dStrideHeight: strided image size
* a_dOutStrideWidth, a_dOutStrideHeight: output strided image size
* a_nImgFormat: to set input image (when run)'s format, refers to enum alAILDC_IMG_FORMAT
* a_pLDCBuffer, a_udLDCBufferSize: LDC packdata and data size.
* a_pFPCBuffer, a_udFPCBufferSize: FPC's tuning data and data size.
* a_pModelBuffer, a_udModelBufferSize: Model's data and data size.
*/
alAILDC_ERR_CODE alAILDC_Buffer_Init_v2(
        int a_dID,
        int a_dInImgW, int a_dInImgH,
        int a_dOutWidth, int a_dOutHeight,
        int a_dStrideWidth, int a_dStrideHeight,
        int a_dOutStrideWidth, int a_dOutStrideHeight,
        alAILDC_IMG_FORMAT a_nImgFormat,
        unsigned char *a_pLDCBuffer, unsigned int a_udLDCBufferSize,
        unsigned char *a_pFPCBuffer, unsigned int a_udFPCBufferSize,
        unsigned char *a_pModelBuffer, unsigned int a_udModelBufferSize);

/* alAILDC_Buffer_Run
* To run LDC and get result images.
* a_dID: instance id
* a_pucInImage_Y, a_pucInImage_UV: Input Image buffer the format in which was set when initializaiton.
* a_pucOutImaghe_Y, a_pucOutImage_UV: Output reslut image buffer. the format is similar to input.
* a_dInputRotate: set 0 if input image is not rotated. 1 for rotated input image.
* a_pfMvpMatrix: set nullptr if the output result is not expected to rotate. Otherwise, input the matrix.
* a_ptInZoomWOI: Input the WOI of current Zoom level. refers to woi structure alAILDC_RECT.
* a_ptInFaceInfo: Input face data by strcuture alAILDC_FACE_INFO.
* a_ptOutLdcInfo: Get output of LDC table information in strucuture alAILDC_LDC_INFO_v3_T.
*                 Set flag=1 if expected to get lastest table in this run. And will get LDC table buffer point.
*/
alAILDC_ERR_CODE alAILDC_Buffer_Run(
        int a_dID,
        unsigned char *a_pucInImage_Y, unsigned char *a_pucInImage_UV,
        unsigned char *a_pucOutImage_Y, unsigned char *a_pucOutImage_UV,
        int a_dInputRotate, float *a_pfMvpMatrix,
        alAILDC_RECT *a_ptInZoomWOI, alAILDC_FACE_INFO *a_ptInFaceInfo,
        alAILDC_LDC_INFO_T *a_ptOutLdcInfo);

/* alAILDC_Set_SubjectMask
* To set subject mask buffer
* a_dID           : instance id
* a_pucMaskImage_Y: subject mask buffer
* a_dMaskWidth, a_dMaskHeight: image size of subject mask
*/
alAILDC_ERR_CODE alAILDC_Set_SubjectMask(int a_dID, unsigned char *a_pucMaskImage_Y, int a_dMaskWidth, int a_dMaskHeight);

/* alAILDC_Buffer_Deinit
* To release all souces and memory of alAILDC
* a_dID: instance id
*/
alAILDC_ERR_CODE alAILDC_Buffer_Deinit(int a_dID);

/* alAILDC_Map_Location
* To Map input roi by the table a_dID camera used, get output roi
* a_dID       : instance id
* a_ptInOutRoi: input source roi and get output mapped roi.
* a_nflag: function avaliable selection, refers to alAILDC_MP_ROI_FALG_T.
*/
alAILDC_ERR_CODE alAILDC_Map_Location(int a_dID, alAILDC_ROI_MAPPING_T * a_ptInOutRoi, alAILDC_MAP_ROI_FLAG_T a_nflag);

/* alAILDC_Set_Flag
* To set specific mode, function is refered to alAILDC_SET_FLAG
* a_udFlag: 0 : log-off
            1:  api_check
            2:  profile log
*/
alAILDC_ERR_CODE alAILDC_Set_Flag(unsigned int a_udFlag);

/* alAILDC_Set_Debug_Flag
* To set debug flag
*/
alAILDC_ERR_CODE alAILDC_Set_Debug_Flag(alAILDC_DEBUG_FLAG a_dDebugFlag);

/* alAILDC_Set_ScreenOrientation
* To set screen orientation mode which is referred to alAILDC_SET_SCREEN_ORIENTATION
* a_dID  : instance id
* a_dFlag:  0: screen angle = 0
            1: screen angle = 90
            2: screen angle = 180
            3: screen angle = 270
*/
alAILDC_ERR_CODE alAILDC_Set_ScreenOrientation(int a_dID, alAILDC_SET_SCREEN_ORIENTATION a_dFlag);

/* alAILDC_Set_TuningParameters
* To set tuning parameters which is referred to alAILDC_SET_TUNE_PARA
* a_dID  : instance id
* a_dFlag:  0: no effect
            1: set correction strength
            2: set background distortion tolerance
* a_Value:  setting value
*/
alAILDC_ERR_CODE alAILDC_Set_TuningParameters(int a_dID, alAILDC_SET_TUNE_PARA a_dFlag, void *a_Value);

#ifdef __cplusplus
}
#endif

#endif //ALAILDC