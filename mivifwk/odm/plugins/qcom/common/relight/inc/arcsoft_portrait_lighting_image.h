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

#ifndef _ARCSOFT_PORTRAIT_LIGHTING_IMAGE_H_
#define _ARCSOFT_PORTRAIT_LIGHTING_IMAGE_H_

#ifdef ARC_PLIDLL_EXPORTS
#ifdef PLATFORM_LINUX
#define ARCPLI_API __attribute__((visibility ("default")))
#else
#define ARCPLI_API __declspec(dllexport)
#endif
#else
#define ARCPLI_API
#endif

#include "arcsoft_portrait_lighting_common.h"

#ifdef __cplusplus
extern "C" {
#endif

ARCPLI_API const MPBASE_Version *ARC_PLI_GetVersion();

ARCPLI_API MRESULT ARC_PLI_Init(			// return MOK if success, otherwise fail
	MHandle			*phHandle,			    // [out] The algorithm engine will be initialized by this API
	MInt32 			i32InitMode,            // [in] The camere mode ARC_PL_INIT_MODE_*
	MInt32			i32ImgOrient		    // [in] The image orientation ARC_PL_FOC_*
);

ARCPLI_API MRESULT ARC_PLI_Uninit(	        // return MOK if success, otherwise fail
	MHandle			*phHandle               // [in/out] The algorithm engine will be un-initialized by this API
);

ARCPLI_API MRESULT ARC_PLI_PreProcess(     	   // return MOK if success, otherwise fail
	MHandle						hHandle,	   // [in] The algorithm engine
	LPASVLOFFSCREEN				pSrcImg, 	   // [in] The input image data
	LPARC_PL_DEPTHINFO			pDepthInfo,	   // [in] The depth info, can be NULL
	LPARC_PL_LIGHT_REGION		pLightRegion   // [out] The light position region
);

ARCPLI_API MRESULT ARC_PLI_GetExtraData(
	MHandle				hHandle,			   // [in] The algorithm engine
	MVoid				*pExtraData			   // [out] The extra data
);

ARCPLI_API MRESULT ARC_PLI_GetExtraDataSize(
	MHandle				hHandle,			   // [in] The algorithm engine
	MInt32				*pExtraDataSize	       // [out] The extra data size
);

ARCPLI_API MRESULT ARC_PLI_SetExtraData(
	MHandle				hHandle,			// [in] The algorithm engine
	MInt32				i32ExtraDataSize,   // [in] The extra data size
	MVoid				*pExtraData 		// [in] The extra data
);

ARCPLI_API MRESULT ARC_PLI_SetLightSource(
	MHandle					hHandle,           // [in] The algorithm engine
	LPARC_PL_LIGHT_SOURCE 	pLightSource	   // [in] The light source position
);

ARCPLI_API MRESULT ARC_PLI_GetLightSource(
	MHandle					hHandle,           // [in] The algorithm engine
	LPARC_PL_LIGHT_SOURCE 	pLightSource	   // [in/out] The light source position
);

ARCPLI_API MRESULT ARC_PLI_Process(         	// return MOK if success, otherwise fail
	MHandle			        hHandle,        	// [in] The algorithm engine
	LPARC_PL_DEPTHINFO		pDepthInfo,			// [in] The Depth info, can be MNull
	LPASVLOFFSCREEN			pSrcImg,			// [in] The input image data
	LPARC_PL_LIGHT_PARAM	pLightParam,   	 	// [in] The portrait lighting mode
	LPASVLOFFSCREEN         pDstImg         	// [out] The result image data
);

#ifdef __cplusplus
}
#endif

#endif /* _ARCSOFT_PORTRAIT_LIGHTING_IMAGE_H_ */
