#pragma once
#include <vector>
// define image orientation
#define MI_PL_OPF_0   0x1
#define MI_PL_OPF_90  0x2
#define MI_PL_OPF_270 0x3
#define MI_PL_OPF_180 0x4

#define MOK     0
#define MFAILED -1

#define MI_PL_INIT_MODE_QUALITY     0x0
#define MI_PL_INIT_MODE_PERFORMANCE 0x1

#define FORMAT_YUV_420_NV21 0x01
#define FORMAT_YUV_420_NV12 0x02

#define MI_PL_MODE_NEON_SHADOW 0x20 // 霓影
#define MI_PL_MODE_PHANTOM     0x21 // 魅影
#define MI_PL_MODE_NOSTALGIA   0x22 // 怀旧
#define MI_PL_MODE_RAINBOW     0x23 // 彩虹
#define MI_PL_MODE_WANING      0x24 // 阑珊
#define MI_PL_MODE_DAZZLE      0x25 // 炫影
#define MI_PL_MODE_GORGEOUS    0x26 // 斑斓
#define MI_PL_MODE_PURPLES     0x27 // 嫣红
#define MI_PL_MODE_DREAM       0x28 // 梦境

typedef long MLong;
typedef float MFloat;
typedef double MDouble;
typedef unsigned char MByte;
typedef unsigned short MWord;
typedef unsigned int MDWord;
typedef void *MHandle;
typedef char MChar;
typedef long MBool;
typedef void MVoid;
typedef void *MPVoid;
typedef char *MPChar;
typedef short MShort;
typedef const char *MPCChar;
typedef MLong MRESULT;
typedef MDWord MCOLORREF;
typedef signed char MInt8;
typedef unsigned char MUInt8;
typedef signed short MInt16;
typedef unsigned short MUInt16;
typedef signed int MInt32;
typedef unsigned int MUInt32;

/*Define the image format space*/
typedef struct __tag_MIIMAGEBUFFER
{
    MUInt32 u32PixelArrayFormat;
    MInt32 i32Width;
    MInt32 i32Height;
    MInt32 i32Scanline;
    MUInt8 *ppu8Plane[4];
    MInt32 pi32Pitch[4];
} MIIMAGEBUFFER, *LPMIIMAGEBUFFER;

typedef struct __tag_MIPOINT
{
    MInt32 x;
    MInt32 y;
} MIPOINT, *PMIPOINT;

typedef struct _tag_MIPLLIGHTREGION
{
    MIPOINT ptCenter;
    MInt32 i32Radius;
} MIPLLIGHTREGION, *LPMIPLLIGHTREGION;

typedef struct _tag_MIPLLIGHTPARAM
{
    MInt32 i32LightMode; // Light mode, from effect mode
} MIPLLIGHTPARAM, *LPMIPLLIGHTPARAM;

typedef struct _tag_MIPLLIGHTSOURCE
{
    MInt32 i32Width;       // The image width of setting light source
    MInt32 i32Height;      // The image height of setting light source
    MIPOINT ptLightSource; // Light source position
} MIPLLIGHTSOURCE, *LPMIPLLIGHTSOURCE;

typedef struct _tag_MIPLDEPTHINFO
{
    MInt32 i32MaskWidth;
    MInt32 i32MaskHeight;
    MUInt8 *pMaskImage;
    MIIMAGEBUFFER *pBlurImage; // blur image
} MIPLDEPTHINFO, *LPMIPLDEPTHINFO;

typedef struct _tag_MIPLFACEINFO
{
    MIPOINT pos;
    MInt32 i32Width;
    MInt32 i32Height;
    std::vector<float> landmarks; // x0,x1,...,y0,y1...
} MIPLFACEINFO, *LPMIPLMIPLFACEINFO;
