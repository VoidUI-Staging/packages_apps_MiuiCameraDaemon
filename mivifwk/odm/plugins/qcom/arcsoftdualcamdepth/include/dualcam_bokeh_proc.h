/************************************************************************************

Filename   : dualcam_bokeh_proc.h
Content    : Interface of dual camera bokeh rendering library
Created    : Feb. 23, 2022
Author     : raoqiang (raoqiang@xiaomi.com)
Copyright  : Copyright 2022 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/
#pragma once
#include "mi_type.h"

typedef struct AlgoBokehInterface
{
    int aperture;                                                // [in]  The intensity of blur,Range is [0,24]
    int ISO;                                                     // [in]  The iso of current image.
    AlgoMultiCams::MiFaceRect fRect;                             // [in]  The face rect.
    AlgoMultiCams::MiPoint2i tp;                                 // [in]  The touch point.
} AlgoBokehInterface;

typedef struct AlgoBokehLaunchParams
{
    const char* jsonFilename;                                    // [in]  The bokeh json file full name
    unsigned int MainImgW;                                       // [in]  The main img width
    unsigned int MainImgH;                                       // [in]  The main img height
    bool isPerformanceMode;                                      // [in]  ture is performance mode and false is non-performance mode
    bool isHotMode;                                              // [in]  ture is hot mode and false is non-hot mode
    AlgoMultiCams::ALGO_CAPTURE_FRAME frameInfo;                 // [in]  The capture frame
    AlgoMultiCams::ALGO_CAPTURE_FOV FovInfo;                     // [in]  The capture zoom ratio
    void *simulationInfo = nullptr;                              // [in]  Useless Param in Android, pass NULL.
} AlgoBokehLaunchParams;


typedef struct AlgoBokehInitParams
{
    AlgoBokehInterface userInterface;                           // [in]  The user interface data.
    bool isSingleCameraDepth;                                   // [in]  true is single camera depth. false is dual camera depth
    const char* dstPath = nullptr;                              // [in]  Useless Param in Android, pass NULL.
    const char* cShortName = nullptr;                           // [in]  Useless Param in Android, pass NULL.
} AlgoBokehInitParams;


typedef struct AlgoBokehEffectParams
{
    AlgoMultiCams::MiImage *imgMain;                            // [in]  The original image input for bokeh effect.
    AlgoMultiCams::MiImage* imgDepth;                           // [in]  The depth image input.
    int validDepthSize;                                         // [in] The valid size of output depth buffer.
    AlgoMultiCams::MiImage *imgResult;                          // [out] The bokeh image output.
} AlgoBokehEffectParams;

/************************************************************************
*the following functions are used to calculate Bokeh Effect in Capture
************************************************************************/
int MialgoCaptureBokehLaunch(                                   // return 0 if success, otherwise fail, only called once when launching
    void **phEngine,                                            // [in]  The algorithm engine
    AlgoBokehLaunchParams& launchParams                         // [in] The algorithm launch params
);

void MialgoCaptureBokehInit(                                    // no return value, called before every capture.
    void **phEngine,                                            // [in]  The algorithm engine
    AlgoBokehInitParams& initParams                              // [in]  The algorithm init params
);

int MialgoCaptureBokehProc(                                     // return 0 if success, otherwise fail.
    void **phEngine,                                            // [in]  The algorithm engine.
    AlgoBokehEffectParams& bokehParams                          // [in]  The algorithm bokeh effect params
);

void MialgoCaptureBokehDeinit(                                  // no return value, called after every capture.
    void **phEngine                                             // [in]  The algorithm engine.
);

int MialgoCaptureBokehDestory(                                  // return 0 if success, otherwise fail, only called once when node deInit.
    void **phEngine                                             // [in]  The algorithm engine.
);
