// clang-format off
#ifndef DUALCAMREFOCUS_INC_WA_COMMON_H_
#define DUALCAMREFOCUS_INC_WA_COMMON_H_

#include "Image.h"
#include "ammem.h"

#ifdef WA_DCIRDLL_EXPORTS
#ifdef PLATFORM_LINUX
#define WACOOM_API __attribute__((visibility ("default")))
#else
#define WACOOM_API __declspec(dllexport)
#endif
#else
#define WACOOM_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef int                 BOOL;
typedef char                CHAR;
typedef uint8_t             SBYTE;
typedef unsigned char       UCHAR;
typedef int                 INT;
typedef unsigned int        UINT;
typedef float               FLOAT;

typedef uint8_t             UINT8;
typedef int8_t              INT8;
typedef uint16_t            UINT16;
typedef int16_t             INT16;
typedef uint32_t            UINT32;
typedef int32_t             INT32;
typedef uint64_t            UINT64;
typedef int64_t             INT64;

typedef max_align_t         MAXALIGN_T;
typedef size_t              SIZE_T;
typedef void                VOID;

typedef struct _tag_WA_OFFSCREEN {
	wa::Image::ImageType u32PixelArrayFormat;
	INT	i32Width;
	INT	i32Height;
	UINT32  aWidth;
	UINT32  aHeight;
	MUInt8* ppu8Plane[4];
	MInt32	pi32Pitch[4];
}WAOFFSCREEN, *LPWAOFFSCREEN;

typedef struct _tag_WA_DCR_FACE_PARAM {
	PMRECT	   prtFaces;					   // [in]	The array of faces
	MInt32	   *pi32FaceAngles; 			   // [in]	The array of corresponding face orientations
	MInt32	   i32FacesNum; 				   // [in]	The number of faces
} WA_FACE_PARAM, *LPWA_FACE_PARAM;


typedef struct _tag_WA_DCVR_PARAM {
     MPOINT              ptFocus;                    // [in]  The focus point to decide which region should be clear
     MInt32              i32BlurLevel;               // [in]  The intensity of blur,range [1, 100]
     MBool               bRefocusOn;                 // [in]  Do Refocus or not
     WA_FACE_PARAM       faceParam;                  // [in]  The information of faces in the main image
 } WA_PARAM, *LPWA_PARAM;


typedef struct _tag_WA_REFOCUSCAMERAIMAGE_PARAM
{
     MInt32              i32MainWidth_CropNoScale;       // [in]  Width of main image without scale.
     MInt32              i32MainHeight_CropNoScale;      // [in]  Height of main image without scale.
     MInt32              i32AuxWidth_CropNoScale;        // [in]  Width of auxiliary image without scale.
     MInt32              i32AuxHeight_CropNoScale;       // [in]  Height of auxiliary image without scale.
 }WA_REFOCUSCAMERAIMAGE_PARAM, *LPWA_REFOCUSCAMERAIMAGE_PARAM;

typedef struct _tag_WA_DCIR_REFOCUS_PARAM {
	MPOINT		ptFocus;							// [in]  The focus point to decide which region should be clear
	MInt32		i32BlurIntensity;					// [in]  The intensity of blur,Range is [0,100], default as 50.
} WA_DCIR_REFOCUS_PARAM, *LPWA_DCIR_REFOCUS_PARAM;

typedef struct _tag_WA_DCIR_PARAM {
	MInt32		i32ImgDegree;					// [in]  The degree of input images
	MFloat		fMaxFOV;						// [in]  The maximum camera FOV among horizontal and vertical in degree
	WA_FACE_PARAM	faceParam;					// [in]  The information of faces in the main image
} WA_DCIR_PARAM, *LPWA_DCIR_PARAM;

#ifdef __cplusplus
}
#endif
#endif /* DUALCAMREFOCUS_INC_WA_COMMON_H_ */
// clang-format on
