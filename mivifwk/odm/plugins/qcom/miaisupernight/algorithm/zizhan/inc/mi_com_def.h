/*******************************************************************************
   Copyright(c) XiaoMI, All right reserved.
*******************************************************************************/

#ifndef MI_COM_DEF_H
#define MI_COM_DEF_H

typedef long MLONG;
typedef float MFLOAT;
typedef double MDOUBLE;
typedef unsigned char MBYTE;
typedef unsigned short MWORD;
typedef unsigned int MDWORD;
typedef char MCHAR;
typedef void MVOID;
typedef void *MPVOID;
typedef char *MPCHAR;
typedef short MSHORT;
typedef const char *MPCCHAR;
typedef signed char MINT8;
typedef unsigned char MUINT8;
typedef signed short MINT16;
typedef unsigned short MUINT16;
typedef signed int MINT32;
typedef unsigned int MUINT32;

#if !defined(M_UNSUPPORT64)
#if defined(_MSC_VER)
typedef signed __int64 MINT64;
typedef unsigned __int64 MUINT64;
#else
typedef signed long long MINT64;
typedef unsigned long long MUINT64;
#endif
#endif

typedef struct _RECT
{
    MINT32 left;
    MINT32 top;
    MINT32 right;
    MINT32 bottom;
} RECT, *PRECT;

typedef struct _POINT
{
    MINT32 x;
    MINT32 y;
} POINT, *PPOINT;

typedef struct _POINT3
{
    MINT32 x;
    MINT32 y;
    MINT32 z;
} POINT3, *PPOINT3;

typedef struct _RECTF
{
    MFLOAT left;
    MFLOAT top;
    MFLOAT right;
    MFLOAT bottom;
} RECTF, *PRECTF;

typedef struct _POINTF
{
    MFLOAT x;
    MFLOAT y;
} POINTF, *PPOINTF;

typedef struct _POINT3F
{
    MFLOAT x;
    MFLOAT y;
    MFLOAT z;
} POINT3F, *PPOINT3F;

// 错误码
#define MERR_MI_BASIC_BASE 0X0100

#define MERR_MI_INVALID_PARA (MERR_MI_BASIC_BASE + 1)

#define MERR_MI_INVALID_IMAGE_TYPE (MERR_MI_BASIC_BASE + 2)
#define MERR_MI_INVALID_IMAGE_SIZE (MERR_MI_BASIC_BASE + 3)
#define MERR_MI_INVALID_IMAGE_NUM  (MERR_MI_BASIC_BASE + 4)

#define MERR_MI_INVALID_CL_PLATFORM (MERR_MI_BASIC_BASE + 5)
#define MERR_MI_INVALID_CL_DEVICE   (MERR_MI_BASIC_BASE + 6)
#define MERR_MI_INVALID_CL_CONTEXT  (MERR_MI_BASIC_BASE + 7)
#define MERR_MI_INVALID_CL_CMDQ     (MERR_MI_BASIC_BASE + 8)
#define MERR_MI_INVALID_CL_BINARY   (MERR_MI_BASIC_BASE + 9)
#define MERR_MI_INVALID_CL_PROGRAM  (MERR_MI_BASIC_BASE + 10)

#define MERR_MI_INVALID_CPU_MEMORY (MERR_MI_BASIC_BASE + 11)
#define MERR_MI_INVALID_GPU_MEMORY (MERR_MI_BASIC_BASE + 12)

#endif
