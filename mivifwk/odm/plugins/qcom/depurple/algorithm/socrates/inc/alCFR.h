#ifndef __ALCFRV3_LIBRARY_HEADER_
#define __ALCFRV3_LIBRARY_HEADER_

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

#include "alGE.h"

#define IMG_FORMAT_YCC420NV12 (0x00f00) // IMG FORMAT CbCr
#define IMG_FORMAT_YCC420NV21 (0x00f01) // IMG FORMAT CrCb
#define IMG_FORMAT_YCC444SEP  (0x00f02) // IMG FORMAT YCC444SEP
#define IMG_FORMAT_YOnly      (0x00f03) // IMG FORMAT Y
#define IMG_FORMAT_YOnly_U16  (0x00f04) // IMG FORMAT Y
#define IMG_FORMAT_YCC420SEP  (0x00f05) // IMG FORMAT YCC420SEP
#define IMG_FORMAT_YV12       (0x00f06) // IMG FORMAT CrCb
#define IMG_FORMAT_YV21       (0x00f07) // IMG FORMAT CbCr
#define IMG_FORMAT_P010_MSB   (0x00f08) // IMG FORMAT NV12, 10bit
#define IMG_FORMAT_P010_LSB   (0x00f09) // IMG FORMAT NV12, 10bit

#define alCFR_ERR_MODULE                   0x27000
#define alCFR_ERR_CODE                     int
#define alCFR_ERR_SUCCESS                  0x00
#define alCFR_ERR_BUFFER_SIZE_TOO_SMALL    (alCFR_ERR_MODULE+0x00000001) // value 159745
#define alCFR_ERR_BUFFER_IS_NULL           (alCFR_ERR_MODULE+0x00000002) // value 159746
#define alCFR_ERR_INVALID_IMAGE_FORMAT     (alCFR_ERR_MODULE+0x00000003) // value 159747
#define alCFR_ERR_INVALID_IMAGE_STRIDE     (alCFR_ERR_MODULE+0x00000004) // value 159748
#define alCFR_ERR_INVALID_PARA_FILE        (alCFR_ERR_MODULE+0x00000005) // value 159749
#define alCFR_ERR_NO_INITIAL               (alCFR_ERR_MODULE+0x00000006) // value 159750
#define alCFR_ERR_INVALID_FLOW             (alCFR_ERR_MODULE+0x00000007) // value 159751
#define alCFR_ERR_INVALID_HANDLE           (alCFR_ERR_MODULE+0x00000008) // value 159752
#define alCFR_ERR_INVALID_ZOOM_SETTING     (alCFR_ERR_MODULE+0x00000009) // value 159753
#define alCFR_ERR_INVALID_ZOOM_CENTER      (alCFR_ERR_MODULE+0x0000000a) // value 159754
#define alCFR_ERR_INVALID_FACE_SETTING     (alCFR_ERR_MODULE+0x0000000b) // value 159755
#define aLCFR_ERR_DUPLICATED_RUN           (alCFR_ERR_MODULE+0x0000000c) // value 159756
#define alCFR_ERR_DUPLICATED_CLOSE         (alCFR_ERR_MODULE+0x0000000d) // value 159757
#define alCFR_ERR_MASK_ID_NOT_DETECTED     (alCFR_ERR_MODULE+0x0000000e) // value 159758
#define alCFR_ERR_INVALID_PATH             (alCFR_ERR_MODULE+0x0000000f) // value 159759
#define alCFR_ERR_INVALID_PATH_SIZE        (alCFR_ERR_MODULE+0x00000010) // value 159760

#define alCFR_WOI_COUNT 10

typedef enum {
        DEBUG_FLAG_DISABLE                  = 0x00000000,
        DEBUG_FLAG_BYPASS_MODE              = 0x00000001,
        DEBUG_FLAG_ENABLE_WATERMARK         = 0x00000002,

        DEBUG_FLAG_LOG_API_CHECKING         = 0x00000010,
        DEBUG_FLAG_LOG_PROFILE              = 0x00000020,

        DEBUG_FLAG_DUMP_INPUT_DATA          = 0x00000100,
        DEBUG_FLAG_DUMP_OUTPUT_DATA         = 0x00000200,
        DEBUG_FLAG_ENABLE_MASK              = 0xFFFFFFFF
} alCFR_DEBUG_FLAG;

typedef struct{
    short int   m_wLeft; // Left boundary of WOI
    short int   m_wTop; // Top boundary of WOI
    short int   m_wWidth; // Width of WOI
    short int   m_wHeight; // Height of WOI
} alCFR_RECT; // The structure to represent a rectangle

typedef struct{
    short int   m_wX; // Location in x axis
    short int   m_wY; // Location in y axis
} alCFR_2DPOINT; // The structure to represent a position of 2D point

typedef struct IMAGE_INFO
{
    unsigned int m_udFormat;

    unsigned int m_udStride_0;
    unsigned int m_udStride_1;
    unsigned int m_udStride_2;

    void *m_pBuf_0;
    void *m_pBuf_1;
    void *m_pBuf_2;
} GENERAL_IMAGE_INFO;

typedef struct alCFR_IMAGE_INFO
{
    unsigned int m_udFormat; // Input image format

    unsigned int m_udStride_0; // Stride of input “0” component
    unsigned int m_udStride_1; // Stride of input “1” component
    unsigned int m_udStride_2; // Stride of input “2” component

    void *m_pBuf_0; // Pointer to the input “0” component
    void *m_pBuf_1; // Pointer to the input “1” component
    void *m_pBuf_2; // Pointer to the input “2” component
} alCFR_GENERAL_IMAGE_INFO; //The structure for input image information

typedef struct {
    int        m_dFaceInfoCount; // number of face information
    alCFR_RECT  m_tFaceInfoWOI[alCFR_WOI_COUNT]; // rectangle of face information
    unsigned char m_ucFace_Orientation; // direction of bottom on image, 0: bottom, 1: right, 2: up, 3: left
} alCFR_FACE_T; //The structure to represent face information

typedef enum {
    ALCFR_MODE_FLAG_NORMAL, //every title add ALCFR_
    ALCFR_MODE_FLAG_MASK,
    ALCFR_MODE_FLAG_AICFR, //change SEG to AI
    ALCFR_MODE_FLAG_SP,
    ALCFR_MODE_FLAG_SP_AICFR
}alCFR_INSTANCE_MODE;

typedef struct{
    alCFR_INSTANCE_MODE alCFR_Instance_Mode;

    ///para for Plus mode
    void *a_pLCA_Param;
    unsigned int a_udLCA_ParamSize;
    void *a_pLCA_CalBin;
    unsigned int a_udLCA_CalBinSize;

    ///para for SP mode & SP_Seg mode
    int a_dSPLevel;

    ///para for SEG mode & SP_Seg mode
    void *a_pModelBin;
    unsigned int a_udModelBinSize;

    ///para of mask
    unsigned short input_mask_width;
    unsigned short input_mask_height;
    bool enable_mask_setting;

    ///para about the performance
    bool enable_performance_interface;

}alCFR_INIT_EXTRA_PARA; //The structure to represent the extra parameter for the init. step

#ifndef C_MODEL
#ifdef __cplusplus
extern "C" {
#endif

/// For multi-thread start
alCFR_ERR_CODE alCFR_Instance_Init(void **a_ppInOutHandle, unsigned short a_uwInOriWidth, unsigned short a_uwInOriHeight, void *a_pCFRPara);

alCFR_ERR_CODE alCFR_Instance_Run(GENERAL_IMAGE_INFO *a_ptInOutImage,
                                    void *a_pInHandle,
                                    unsigned short a_uwInWidth, unsigned short a_uwInHeight,
                                    void *a_pCFRPara);

alCFR_ERR_CODE alCFR_Instance_Close(void **a_ppInHandle);

#ifndef CFR3
alCFR_ERR_CODE alCFR_Instance_Init_Ver4(void **a_ppInOutHandle, unsigned short a_uwInOriWidth, unsigned short a_uwInOriHeight, void *a_pCFRPara,unsigned int a_pCFRPara_size, alCFR_INIT_EXTRA_PARA *a_pCFRExtraPara);

alCFR_ERR_CODE alCFR_Instance_Resource_Alloc_Ver4(void **a_ppInOutHandle, void *a_pInHandle);

alCFR_ERR_CODE alCFR_Instance_Run_Ver4(alCFR_GENERAL_IMAGE_INFO *a_ptInOutImage,
                                    void *a_pInHandle,
                                    unsigned short a_uwInWidth, unsigned short a_uwInHeight,
                                    void *a_pCFRPara);

alCFR_ERR_CODE alCFR_Instance_Resource_Release_Ver4(void *a_pInHandle);

alCFR_ERR_CODE alCFR_Instance_Close_Ver4(void **a_ppInHandle);
#endif

#ifdef CFR3
alCFR_ERR_CODE alCFR_Instance_Init_Ver3(void **a_ppInOutHandle, unsigned short a_uwInOriWidth, unsigned short a_uwInOriHeight, void *a_pCFRPara,alCFR_INIT_EXTRA_PARA *a_pCFRExtraPara);

alCFR_ERR_CODE alCFR_Instance_Resource_Alloc_Ver3(void **a_ppInOutHandle, void *a_pInHandle, unsigned char *a_pCFRPara, unsigned short a_uwInOriWidth, unsigned short a_uwInOriHeight);

alCFR_ERR_CODE alCFR_Instance_Run_Ver3(alCFR_GENERAL_IMAGE_INFO *a_ptInOutImage,
                                    void *a_pInHandle,
                                    unsigned short a_uwInWidth, unsigned short a_uwInHeight,
                                    void *a_pCFRPara);

alCFR_ERR_CODE alCFR_Instance_Close_Ver3(void **a_ppInHandle);
#endif


/// For multi-thread end

alCFR_ERR_CODE alCFR_VersionInfo_Get(void *a_pOutBuf, int a_dInBufMaxSize);

alCFR_ERR_CODE alCFR_TuningData_VersionInfo_Get(int *a_pdOutTuningDataVersion, void *a_pInCFRPara, unsigned int input_tuning_data_size);

alCFR_ERR_CODE alCFR_Instance_ExposureInfo_Set(void *a_pInHandle, unsigned int a_udInExposure);

alCFR_ERR_CODE alCFR_Instance_FaceInfo_Set(void *a_pInHandle, alCFR_FACE_T a_tInFace);

alCFR_ERR_CODE alCFR_Instance_ZoomInfo_Set(void *a_pInHandle, alCFR_RECT a_tInWoi, unsigned short a_uwInWidth, unsigned short a_uwInHeight);

alCFR_ERR_CODE alCFR_Instance_Set_SubjectMask(void *a_pInHandle, unsigned char *a_pucInMaskBuf, int a_dInMaskWidth, int a_dInMaskHeight, unsigned char *a_pucInMaskID, unsigned char a_ucInMaskIDNum);

alCFR_ERR_CODE alCFR_Instance_Set_Working_Path(char *a_pcInWorkingPath, int a_dInWorkingPathSize);

alCFR_ERR_CODE alCFR_Set_Debug_Path(char *a_pcInDebugPath, int a_dInDebugPathSize);
alCFR_ERR_CODE alCFR_Set_Debug_Flag(alCFR_DEBUG_FLAG a_dDebugFlag);
alCFR_ERR_CODE alCFR_Set_Debug_Persist(char* a_pcPersistString, int a_dInPStringSize);

///Useless function in current version, just prevent the online be crashed
alCFR_ERR_CODE alCFR_Instance_Init_SP(void **a_ppInOutHandle, unsigned short a_uwInOriWidth, unsigned short a_uwInOriHeight, void *a_pCFRPara, int a_dSPLevel);

alCFR_ERR_CODE alCFR_Instance_Run_SP(GENERAL_IMAGE_INFO *a_ptInOutImage,
                                        void *a_pInHandle,
                                        unsigned short a_uwInWidth, unsigned short a_uwInHeight,
                                        void *a_pCFRPara);

alCFR_ERR_CODE alCFR_Instance_Close_SP(void **a_ppInHandle);

alCFR_ERR_CODE alCFR_Instance_Init_Plus(void **a_ppInOutHandle, unsigned short a_uwInOriWidth, unsigned short a_uwInOriHeight, void *a_pCFRPara,
                                        void *a_pLCA_Param, unsigned int a_udLCA_ParamSize,
                                        void *a_pLCA_CalBin, unsigned int a_udLCA_CalBinSize);

alCFR_ERR_CODE alCFR_Instance_Init_Seg( void **a_ppInOutHandle, unsigned short a_uwInOriWidth, unsigned short a_uwInOriHeight, void *a_pCFRPara,
                                        void *a_pModelBin, unsigned int a_udModelBinSize);

alCFR_ERR_CODE alCFR_Instance_Init_SP_Seg(  void **a_ppInOutHandle, unsigned short a_uwInOriWidth, unsigned short a_uwInOriHeight, void *a_pCFRPara,
                                            int a_dSPLevel,
                                            void *a_pModelBin, unsigned int a_udModelBinSize);

#ifdef __cplusplus
}
#endif
#endif //this #endif is belone to C_MODEL
#endif //__ALCFRV3_LIBRARY_HEADER_
