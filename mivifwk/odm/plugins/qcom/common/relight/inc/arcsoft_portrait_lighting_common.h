/*******************************************************************************
Copyright(c) ArcSoft, All right reserved.

This file is ArcSoft's property. It contains ArcSoft's trade secret, proprietary
and confidential information.

The information and code contained in this file is only for authorized ArcSoft
employees to design, create, modify, or review.

DO NOT DISTRIBUTE, DO NOT DUPLICATE OR TRANSMIT IN ANY FORM WITHOUT PROPER
AUTHORIZATION.

If you are not an intended recipient of this file, you must not copy,
distribute, modify, or take any action in reliance on it.

If you have received this file in error, please immediately notify ArcSoft and
permanently delete the original and any copy of any file and any printout
thereof.
*******************************************************************************/

#ifndef _ARCSOFT_PORTRAIT_LIGHTING_COMMON_H_
#define _ARCSOFT_PORTRAIT_LIGHTING_COMMON_H_

#ifdef ARC_PLIDLL_EXPORTS
#ifdef PLATFORM_LINUX
#define ARCPL_API __attribute__((visibility ("default")))
#else
#define ARCPL_API __declspec(dllexport)
#endif
#else
#define ARCPL_API
#endif

#include "asvloffscreen.h"
#include "merror.h"
#include "ammem.h"

#ifdef __cplusplus
extern "C" {
#endif

//Error Code
#define ARC_PL_ERR_NO_FACE				(0x00040001)
#define ARC_PL_ERR_SMALL_FACE			(0x00040002)

//Init Mode
#define ARC_PL_INIT_MODE_QUALITY			0x0
#define ARC_PL_INIT_MODE_PERFORMANCE		0x1


//define image orientation
#define ARC_PL_OPF_0					0x1
#define ARC_PL_OPF_90					0x2
#define ARC_PL_OPF_270					0x3
#define ARC_PL_OPF_180					0x4

//define mode
#define	ARC_PL_MODE_CONTOUR				0x3
#define	ARC_PL_MODE_DIFFUSEWAVE			0x10
#define	ARC_PL_MODE_SPOTWAVE			0x12
#define	ARC_PL_MODE_LEAF				0x14
#define	ARC_PL_MODE_RAINBOW				0x15
#define	ARC_PL_MODE_PORTRAIT			0x16
#define	ARC_PL_MODE_MOVIE_2				0x17
#define ARC_PL_MODE_HOIL				0x18

typedef struct _tag_ARC_PL_LIGHT_REGION {
	MPOINT ptCenter;
	MInt32 i32Radius;
} ARC_PL_LIGHT_REGION, *LPARC_PL_LIGHT_REGION;

typedef struct _tag_ARC_PL_LIGHT_PARAM {
	MInt32	i32LightMode;                 // Light mode, from ARC_PL_MODE_*
} ARC_PL_LIGHT_PARAM, *LPARC_PL_LIGHT_PARAM;

typedef struct _tag_ARC_PL_LIGHT_SOURCE {
	MInt32	i32Width;				      // The image width of setting light source
	MInt32  i32Height;					  // The image height of setting light source
	MPOINT  ptLightSource;			      // Light source position
} ARC_PL_LIGHT_SOURCE, *LPARC_PL_LIGHT_SOURCE;

typedef struct _tag_ARC_PL_DEPTHINFO {
	MInt32			lDispMapSize; 
	MVoid			*pDispMap;
	ASVLOFFSCREEN	*pBokehImg;
} ARC_PL_DEPTHINFO, *LPARC_PL_DEPTHINFO;

#ifdef __cplusplus
}
#endif

#endif /* _ARCSOFT_PORTRAIT_LIGHTING_COMMON_H_ */
