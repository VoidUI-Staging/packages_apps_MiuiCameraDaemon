#ifndef _ALHLDC_ERRCODE_H_
#define _ALHLDC_ERRCODE_H_

#define alhLDC_ERR_MODULE                0x39000
#define alhLDC_ERR_CODE                  int
#define alhLDC_ERR_SUCCESS               0x00
#define alhLDC_ERR_INIT_DIM_FAIL         (alhLDC_ERR_MODULE + 0x00000001)
#define alhLDC_ERR_BUFFER_SIZE_TOO_SMALL (alhLDC_ERR_MODULE + 0x00000002)
#define alhLDC_ERR_BUFFER_IS_NULL        (alhLDC_ERR_MODULE + 0x00000003)
#define alhLDC_ERR_INVALID_ENABLE        (alhLDC_ERR_MODULE + 0x00000004)
#define alhLDC_ERR_INVALID_VALUE         (alhLDC_ERR_MODULE + 0x00000005)
#define alhLDC_ERR_FUNCTION_INVALID      (alhLDC_ERR_MODULE + 0x00000006)
#define alhLDC_ERR_INVALID_ID            (alhLDC_ERR_MODULE + 0x00000007)
#define alhLDC_ERR_TIME_EXPIRED          (alhLDC_ERR_MODULE + 0x00000008)
#define alhLDC_ERR_NO_INITIAL            (alhLDC_ERR_MODULE + 0x00000009)
#define alhLDC_ERR_CL_INITIAL            (alhLDC_ERR_MODULE + 0x0000000A)
#define alhLDC_ERR_INVALID_IN_OUT        (alhLDC_ERR_MODULE + 0x0000000B)
#define alhLDC_ERR_INVALID_OPERATION     (alhLDC_ERR_MODULE + 0x0000000C)
#define alhLDC_ERR_Set_IMAGE_FORMAT      (alhLDC_ERR_MODULE + 0x0000000D)
#define alhLDC_ERR_INVALID_PATH          (alhLDC_ERR_MODULE + 0x0000000E)
#define alhLDC_ERR_INVALID_PATH_SIZE     (alhLDC_ERR_MODULE + 0x0000000F)

#endif // _ALHLDC_ERRCODE_H_
