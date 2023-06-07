#ifndef _ALGE_H_
#define _ALGE_H_

#include "mtype.h"
#define alGE_ERR_CODE int
#define alGE_ERR_SUCCESS 0x00

#if 0
typedef struct{
    short int   m_wLeft;
    short int   m_wTop;
    short int   m_wWidth;
    short int   m_wHeight;
} alGE_RECT;
#endif

typedef struct{
    short int   m_wX;
    short int   m_wY;
} alGE_2DPOINT;

#define IMG_FORMAT_YCC420NV12 (0x00f00) // CbCr
#define IMG_FORMAT_YCC420NV21 (0x00f01) // CrCb
#define IMG_FORMAT_YCC444SEP  (0x00f02)
#define IMG_FORMAT_YOnly      (0x00f03)
#define IMG_FORMAT_YOnly_U16  (0x00f04)
#define IMG_FORMAT_YCC420SEP  (0x00f05)
#define IMG_FORMAT_YV12       (0x00f06) //CrCb
#define IMG_FORMAT_YV21       (0x00f07) //CbCr
#define IMG_FORMAT_P010_MSB   (0x00f08) // NV12, 10bit
#define IMG_FORMAT_P010_LSB   (0x00f09) // NV12, 10bit

#if 0
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
#endif

#endif // _ALGE_H_
