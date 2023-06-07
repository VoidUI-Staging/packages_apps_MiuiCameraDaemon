/*******************************************************************************
This file contains parameters and functions of image enahnce capture.
If you have received this file in error, please immediately notify algo-ai group.
*******************************************************************************/
#ifndef __ALGO_IE_H__
#define __ALGO_IE_H__
// #define ALGO_IE_API __attribute__((visibility ("default")))

typedef int MIALGOResult;

#ifdef __cplusplus
extern "C" {
#endif

#if defined _WIN32 || defined __CYGWIN__
#ifdef BUILDING_MOD
#ifdef __GNUC__
#define ALGO_IE_API __attribute__((dllexport))
#else
#define ALGO_IE_API __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
#endif
#else
#ifdef __GNUC__
#define ALGO_IE_API __attribute__((dllimport))
#else
#define ALGO_IE_API __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
#endif
#endif
#define MOD_LOCAL
#else
#if __GNUC__ >= 4
#define ALGO_IE_API __attribute__((visibility("default")))
#define MOD_LOCAL   __attribute__((visibility("hidden")))
#else
#define ALGO_IE_API
#define MOD_LOCAL
#endif
#endif

typedef struct _tag_ALGO_IE_CAPTURE_INITPARAM
{
    int image_width;
    int image_height;
    std::string config_path; // assert path
    std::string dla_path;    // model dla path
} ALGO_IE_CAPTURE_INITPARAM;

typedef struct _tag_ALGO_IE_CAPTURE_INITOUT
{
    void *pHandle;
} ALGO_IE_CAPTURE_INITOUT;

typedef struct _tag_ALGO_IE_CAPTURE_INPARAM
{
    void *pHandle;
    int iImgWidth;             // image width
    int iImgHeight;            // image height
    int iImgFormat;            // image format
    int iImgStride;            // image stride
    unsigned char *pImgDataY;  // input image data Y pointer
    unsigned char *pImgDataUV; // input image data UV pointer
    int iImgDataYFD;           // input y buffer fd
    int iImgDataUVFD;          // input uv buffer fd
    float fReserved[4];        // reserved 4 float
} ALGO_IE_CAPTURE_INPARAM;

typedef struct _tag_ALGO_IE_CAPTURE_RUNOUT
{
    int iImgWidth;             // image width
    int iImgHeight;            // image height
    int iImgStride;            // image stride
    unsigned char *pOutDataY;  // output image data Y pointer
    unsigned char *pOutDataUV; // output image data Y pointer
    int iOutDataYFD;           // output y buffer fd
    int iOutDataUVFD;          // output UV buffer fd
} ALGO_IE_CAPTURE_RUNOUT;

typedef struct _tag_ALGO_IE_CAPTURE_FREEPARAM
{
    void *pHandle;
} ALGO_IE_CAPTURE_FREEPARAM;

/*enum ALGO_IE_YUVformat{
    NV21 = 1,
    NV12 = 2,
    OTHERS = 3,
};*/

/************************************************************************
 * This function is used to get version information of library
 ************************************************************************/
ALGO_IE_API const char *ALGO_IE_CaptureGetVersion();

/************************************************************************
 * return value:
 *         0 everything is ok
 *        -1 unknown/unspecified error
 *        -2 insufficient memory
 *        -3 null pointer
 *        -4 input param is bad
 *        -5 malloc memory error
 *        -6 size error
 *        -7 engine error
 *        -8 render error
 *        -9 external memory interference
 ************************************************************************/

/************************************************************************
 * init DL Image Enhancement engine
 ************************************************************************/
ALGO_IE_API MIALGOResult ALGO_IE_CaptureInit(const ALGO_IE_CAPTURE_INITPARAM &stInitInfo,
                                             ALGO_IE_CAPTURE_INITOUT &stInitOut);

/************************************************************************
 * Run DL Image Enhancement engine
 ************************************************************************/
ALGO_IE_API MIALGOResult ALGO_IE_CaptureProcess(const ALGO_IE_CAPTURE_INPARAM &stInputInfo,
                                                ALGO_IE_CAPTURE_RUNOUT &stOutput);

/************************************************************************
 *  Free DL Image Enhancement engine
 ************************************************************************/
ALGO_IE_API MIALGOResult ALGO_IE_CaptureFree(const ALGO_IE_CAPTURE_FREEPARAM &stFreeInfo);

#ifdef __cplusplus
}
#endif

#endif
