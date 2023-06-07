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

#define alCFR_ERR_MODULE                   0x27000
#define alCFR_ERR_CODE                     alGE_ERR_CODE
#define alCFR_ERR_SUCCESS                  alGE_ERR_SUCCESS
#define alCFR_ERR_BUFFER_SIZE_TOO_SMALL    (alCFR_ERR_MODULE+0x00000001) // 159745
#define alCFR_ERR_BUFFER_IS_NULL           (alCFR_ERR_MODULE+0x00000002) // 159746
#define alCFR_ERR_INVALID_IMAGE_FORMAT     (alCFR_ERR_MODULE+0x00000003) // 159747
#define alCFR_ERR_INVALID_IMAGE_STRIDE     (alCFR_ERR_MODULE+0x00000004) // 159748
#define alCFR_ERR_INVALID_PARA_FILE        (alCFR_ERR_MODULE+0x00000005) // 159749
#define alCFR_ERR_NO_INITIAL               (alCFR_ERR_MODULE+0x00000006) // 159750
#define alCFR_ERR_INVALID_FLOW             (alCFR_ERR_MODULE+0x00000007) // 159751
#define alCFR_ERR_INVALID_HANDLE           (alCFR_ERR_MODULE+0x00000008) // 159752
#define alCFR_ERR_INVALID_ZOOM_SETTING     (alCFR_ERR_MODULE+0x00000009) // 159753
#define alCFR_ERR_INVALID_ZOOM_CENTER      (alCFR_ERR_MODULE+0x0000000a) // 159754
#define alCFR_ERR_INVALID_FACE_SETTING     (alCFR_ERR_MODULE+0x0000000b) // 159755
#define aLCFR_ERR_DUPLICATED_RUN           (alCFR_ERR_MODULE+0x0000000c) // 159756
#define alCFR_ERR_DUPLICATED_CLOSE         (alCFR_ERR_MODULE+0x0000000d) // 159757
#define alCFR_ERR_MASK_ID_NOT_DETECTED     (alCFR_ERR_MODULE+0x0000000e) // 159758
#define alCFR_ERR_INVALID_PATH             (alCFR_ERR_MODULE+0x0000000f) // 159759
#define alCFR_ERR_INVALID_PATH_SIZE        (alCFR_ERR_MODULE+0x00000010) // 159760

#define alCFR_WOI_COUNT 10

typedef struct {
    int        m_dFaceInfoCount;
    alGE_RECT  m_tFaceInfoWOI[alCFR_WOI_COUNT];
    unsigned char m_ucFace_Orientation; // direction of bottom on image, 0: bottom, 1: right, 2: up, 3: left
} alCFR_FACE_T;

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

#ifdef __cplusplus
extern "C" {
#endif
    alCFR_ERR_CODE alCFR_VersionInfo_Get(void *a_pOutBuf, int a_dInBufMaxSize);

    alCFR_ERR_CODE alCFR_TuningData_VersionInfo_Get(int *a_pdOutTuningDataVersion, void *a_pInCFRPara);

    /// For multi-thread start
    alCFR_ERR_CODE alCFR_Instance_Init(void **a_ppInOutHandle, unsigned short a_uwInOriWidth, unsigned short a_uwInOriHeight, void *a_pCFRPara);

    alCFR_ERR_CODE alCFR_Instance_Run(GENERAL_IMAGE_INFO *a_ptInOutImage,
                                      void *a_pInHandle,
                                      unsigned short a_uwInWidth, unsigned short a_uwInHeight,
                                      void *a_pCFRPara);

    alCFR_ERR_CODE alCFR_Instance_Close(void **a_ppInHandle);

    alCFR_ERR_CODE alCFR_Instance_ExposureInfo_Set(void *a_pInHandle, unsigned int a_udInExposure);

    alCFR_ERR_CODE alCFR_Instance_FaceInfo_Set(void *a_pInHandle, alCFR_FACE_T a_tInFace);

    alCFR_ERR_CODE alCFR_Instance_ZoomInfo_Set(void *a_pInHandle, alGE_RECT a_tInWoi, unsigned short a_uwInWidth, unsigned short a_uwInHeight);

    alCFR_ERR_CODE alCFR_Instance_Init_SP(void **a_ppInOutHandle, unsigned short a_uwInOriWidth, unsigned short a_uwInOriHeight, void *a_pCFRPara, int a_dSPLevel);

    alCFR_ERR_CODE alCFR_Instance_Run_SP(GENERAL_IMAGE_INFO *a_ptInOutImage,
                                         void *a_pInHandle,
                                         unsigned short a_uwInWidth, unsigned short a_uwInHeight,
                                         void *a_pCFRPara);

    alCFR_ERR_CODE alCFR_Instance_Close_SP(void **a_ppInHandle);
    
    /// For multi-thread end

    alCFR_ERR_CODE alCFR_Instance_Init_Plus(void **a_ppInOutHandle, unsigned short a_uwInOriWidth, unsigned short a_uwInOriHeight, void *a_pCFRPara,
                                            void *a_pLCA_Param, unsigned int a_udLCA_ParamSize,
                                            void *a_pLCA_CalBin, unsigned int a_udLCA_CalBinSize);

    alCFR_ERR_CODE alCFR_Instance_Set_Working_Path(char *a_pcInWorkingPath, int a_dInWorkingPathSize);

    alCFR_ERR_CODE alCFR_Instance_Set_SubjectMask(void *a_pInHandle, unsigned char *a_pucInMaskBuf, int a_dInMaskWidth, int a_dInMaskHeight, unsigned char *a_pucInMaskID, unsigned char a_ucInMaskIDNum);


    /*alCFR Third Generation*/
    alCFR_ERR_CODE alCFR_Instance_Init_Seg( void **a_ppInOutHandle, unsigned short a_uwInOriWidth, unsigned short a_uwInOriHeight, void *a_pCFRPara,
                                            void *a_pModelBin, unsigned int a_udModelBinSize);

    alCFR_ERR_CODE alCFR_Instance_Init_SP_Seg(  void **a_ppInOutHandle, unsigned short a_uwInOriWidth, unsigned short a_uwInOriHeight, void *a_pCFRPara,
                                                int a_dSPLevel,
                                                void *a_pModelBin, unsigned int a_udModelBinSize);

    alCFR_ERR_CODE alCFR_Set_Debug_Path(char *a_pcInDebugPath, int a_dInDebugPathSize);
    alCFR_ERR_CODE alCFR_Set_Debug_Flag(alCFR_DEBUG_FLAG a_dDebugFlag);
    alCFR_ERR_CODE alCFR_Set_Debug_Persist(char* a_pcPersistString, int a_dInPStringSize);

#ifdef __cplusplus
}
#endif

#endif //__ALCFRV3_LIBRARY_HEADER_
