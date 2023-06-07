/************************************************************************************

Filename   : dualcam_bokeh_proc.h
Content    : Interface of dual camera mdbokeh rendering library
Created    : Feb. 23, 2022
Author     : raoqiang (raoqiang@xiaomi.com)
Copyright  : Copyright 2022 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/
#pragma once
#include "mi_type.h"

enum ALGO_CAPTURE_MDBOKEH_LENS_MODE
{
    CAPTURE_MDBOKEH_OFF = 0,
    CAPTURE_MDBOKEH_SPIN = 1,
    CAPTURE_MDBOKEH_SOFT = 2,
    CAPTURE_MDBOKEH_BLACK = 3,
    CAPTURE_MDBOKEH_PORTRAIT = 4
};

typedef struct AlgoMdbokehInterface
{
    int aperture;                                                // [in]  The intensity of blur,Range is [0,24]
    int ISO;                                                     // [in]  The iso of current image.
    AlgoMultiCams::MiFaceRect fRect;                             // [in]  The face rect.
    AlgoMultiCams::MiPoint2i tp;                                 // [in]  The touch point.
} MdbokehInterface;

typedef struct AlgoMdbokehLaunchParams
{
    const char* jsonFilename;                                    // [in]  The mdbokeh json file full name
    unsigned int MainImgW;                                       // [in]  The main img width
    unsigned int MainImgH;                                       // [in]  The main img height
    bool isPerformanceMode;                                      // [in]  ture is performance mode and false is non-performance mode
    bool isHotMode;                                              // [in]  ture is hot mode and false is non-hot mode
    AlgoMultiCams::ALGO_CAPTURE_FRAME frameInfo;                 // [in]  The capture frame
    AlgoMultiCams::ALGO_CAPTURE_FOV fovInfo;                     // [in]  The capture zoom ratio
    void* simulationInfo = nullptr;                              // [in]  Useless Param in Android, pass NULL.
    ALGO_CAPTURE_MDBOKEH_LENS_MODE mdLensMode = CAPTURE_MDBOKEH_OFF;  // [in] The mode of mdbokeh lens
    unsigned char* mdLensBuf;                                 // [in]  The choosen len's effect params buffer
    int mdLensBufSize;                                        // [in]  The param buffer's size
} MdbokehLaunchParams;

typedef struct AlgoMdbokehInitParams
{
    MdbokehInterface userInterface;                           // [in]  The user interface data.
    bool isSingleCameraDepth;                                   // [in]  true is single camera depth. false is dual camera depth
    const char* dstPath = nullptr;                              // [in]  Useless Param in Android, pass NULL.
    const char* cShortName = nullptr;                           // [in]  Useless Param in Android, pass NULL.
} MdbokehInitParams;

typedef struct AlgoMdbokehEffectParams
{
    AlgoMultiCams::MiImage* imgMain;                            // [in]  The original image input for mdbokeh effect.
    AlgoMultiCams::MiImage* imgDepth;                           // [in]  The depth image input.
    AlgoMultiCams::MiImage* imgMainEV;                          // [in]  the ev- image input.
    int validDepthSize;                                         // [in] The valid size of output depth buffer.
    bool hdrTriggered;                                           // [in] If the imgMain has trigged hdr process
    bool seTriggered;                                           // [in] If the imgMain has trigged se process
    AlgoMultiCams::MiImage* imgResult;                          // [out] The mdbokeh image output.
} MdbokehEffectParams;

/************************************************************************
*the following functions are used to calculate Mdbokeh Effect in Capture
************************************************************************/
int MialgoCaptureMdbokehLaunch(                                   // return 0 if success, otherwise fail, only called once when launching
    void** phEngine,                                            // [in]  The algorithm engine
    MdbokehLaunchParams& launchParams                         // [in] The algorithm launch params
);

void MialgoCaptureMdbokehInit(                                    // no return value, called before every capture.
    void** phEngine,                                            // [in]  The algorithm engine
    MdbokehInitParams& initParams                              // [in]  The algorithm init params
);

int MialgoCaptureMdbokehProc(                                     // return 0 if success, otherwise fail.
    void** phEngine,                                            // [in]  The algorithm engine.
    MdbokehEffectParams& procParams                          // [in]  The algorithm Mdbokeh effect params
);

void MialgoCaptureMdbokehDeinit(                                  // no return value, called after every capture.
    void** phEngine                                             // [in]  The algorithm engine.
);

int MialgoCaptureMdbokehDestory(                                  // return 0 if success, otherwise fail, only called once when node deInit.
    void** phEngine                                             // [in]  The algorithm engine.
);
