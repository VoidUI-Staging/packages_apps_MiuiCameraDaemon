#pragma once

#include "mi_portrait_lighting_common.h"

#define AILAB_PLI_API __attribute__((visibility("default")))

#ifdef __cplusplus
extern "C" {
#endif

AILAB_PLI_API MRESULT AILAB_PLI_Init( // return MOK if success, otherwise fail
    MHandle *phHandle,                // [out] The algorithm engine will be initialized by this API
    MInt32 i32InitMode,               // [in] The camere mode MI_PL_INIT_MODE_*
    MInt32 i32Width,                  // [in] The image width
    MInt32 i32Height                  // [in] The image height
);

AILAB_PLI_API MRESULT AILAB_PLI_Uninit( // return MOK if success, otherwise fail
    MHandle *phHandle // [in/out] The algorithm engine will be un-initialized by this API
);

AILAB_PLI_API MRESULT AILAB_PLI_PreProcess( // return MOK if success, otherwise fail
    MHandle hHandle,                        // [in] The algorithm engine
    LPMIIMAGEBUFFER pSrcImg,                // [in] The input image data
    MInt32 orientation,                     // [in] The orientation of image data
    LPMIPLLIGHTREGION pLightRegion          // [out] The light position region
);

AILAB_PLI_API MRESULT AILAB_PLI_SetLightSource(
    MHandle hHandle,               // [in] The algorithm engine
    LPMIPLLIGHTSOURCE pLightSource // [in] The light source position
);

AILAB_PLI_API MRESULT AILAB_PLI_GetLightSource(
    MHandle hHandle,               // [in] The algorithm engine
    LPMIPLLIGHTSOURCE pLightSource // [in/out] The light source position
);

AILAB_PLI_API MRESULT AILAB_PLI_Process( // return MOK if success, otherwise fail
    MHandle hHandle,                     // [in] The algorithm engine
    LPMIPLDEPTHINFO pDepthInfo,          // [in] The Depth info, can be MNull
    LPMIIMAGEBUFFER pSrcImg,             // [in] The input image data
    LPMIPLLIGHTPARAM pLightParam,        // [in] The portrait lighting mode
    LPMIIMAGEBUFFER pDstImg              // [out] The result image data
);

#ifdef __cplusplus
}
#endif