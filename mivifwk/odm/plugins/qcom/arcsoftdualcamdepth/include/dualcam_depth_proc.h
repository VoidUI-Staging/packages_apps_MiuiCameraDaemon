/************************************************************************************

Filename   : dualcam_depth_proc.h
Content    : Interface of dual camera depth calculation library
Created    : Feb. 23, 2022
Author     : raoqiang (raoqiang@xiaomi.com)
Copyright  : Copyright 2022 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/
#pragma once
#include "mi_type.h"

typedef struct AlgoDepthInterface
{
    int AfCode;                                             // [in]  The lens position info.
    int aperture;                                           // [in]  The intensity of blur,Range is [0,24]
    int ISO;                                                // [in]  The iso of current image.
    int TOF;                                                // [in]  The dot tof
    int motionFlag = 0;                                     // [in]  The trigger motion flag for serial scenes, new scene as 0, same as 1.
    int phoneOrientation;                                   // [in]  The orientation angle of phone
    AlgoMultiCams::MiFaceRect fRect;                        // [in]  The face rect.
    AlgoMultiCams::MiPoint2i tp;                            // [in]  The touch point.
} AlgoDepthInterface;

typedef struct AlgoDepthLaunchParams
{
    unsigned char* calibBuffer;                                          // [in]  The calibration buffer
    int calibBufferSize;                                                 // [in]  The calibration buffer's size
    const char* jsonFilename;                                            // [in]  The depth json file full name
    unsigned int MainImgW;                                               // [in]  The main img width
    unsigned int MainImgH;                                               // [in]  The main img height
    unsigned int AuxImgW;                                                // [in]  The aux img width
    unsigned int AuxImgH;                                                // [in]  The aux img height
    bool isPerformanceMode;                                              // [in]  ture is performance mode and false is non-performance mode
    bool isHotMode;                                                      // [in]  ture is hot mode and false is non-hot mode
    bool isMirroredInput;                                                // [in]  true is mirrored input
    AlgoMultiCams::ALGO_CAPTURE_FRAME frameInfo;                         // [in]  The capture frame
    AlgoMultiCams::ALGO_CAPTURE_FOV FovInfo;                             // [in]  The capture zoom ratio
    AlgoMultiCams::ALGO_CAPTURE_SENSOR_COMBINATION sensorCombination;    // [in]  main and aux camera conbination
    AlgoMultiCams::ALGO_CAPTURE_SENSOR_MODE mainSensorMode;              // [in]  main camera mode(binning or remosaic)
    AlgoMultiCams::ALGO_CAPTURE_SENSOR_MODE auxSensorMode;               // [in]  aux camera mode(binning or remosaic)
    void* simulationInfo = nullptr;                                      // [in]  Useless Param in Android, pass NULL
} AlgoDepthLaunchParams;


typedef struct AlgoDepthInitParams
{
    AlgoMultiCams::MiImage* imgMian;                         // [in]  The main image input. Generally, use smaller FOV sensor as main.
    AlgoMultiCams::MiImage* imgAux;                          // [in]  The auxiliary image input.
    AlgoDepthInterface userInterface;                        // [in]  The user interface data.
    bool isSingleCameraDepth;                                // [in]  true is single camera depth. false is dual camera depth
    const char* dstPath = nullptr;                           // [in]  Useless Param in Android, pass NULL.
    const char* cShortName = nullptr;                        // [in]  Useless Param in Android, pass NULL.
} AlgoDepthInitParams;

typedef struct AlgoDepthParams
{
    AlgoMultiCams::MiImage* depthOutput;                     // [out] The depth output image(same size with main image)
    int *validDepthSize;                                     // [out] The valid size of output depth buffer.
} AlgoDepthParams;


/************************************************************************
*the following functions are used to calculate Bokeh Effect in Capture
************************************************************************/
int MialgoCaptureDepthLaunch(                                // return 0 if success, otherwise fail, only called once when launching
    void** phEngine,                                         // [in]  The algorithm engine
    AlgoDepthLaunchParams& launchParams                      // [in]  The algorithm launch params
);

void MialgoCaptureDepthInit(                                 // no return value, called before every capture.
    void** phEngine,                                         // [in]  The algorithm engine
    AlgoDepthInitParams& initParams                          // [in]  The algorithm init params
);

int MialgoCaptureDepthProc(                                  // return 0 if success, otherwise fail.
    void** phEngine,                                         // [in]  The algorithm engine.
    AlgoDepthParams& depthParams                             // [in]  The algorithm depth params.
);

void MialgoCaptureDepthDeinit(                               // no return value, called after every capture.
    void** phEngine                                          // [in]  The algorithm engine.
);

int MialgoCaptureDepthDestory(                               // return 0 if success, otherwise fail, only called once when node deInit.
    void** phEngine                                          // [in]  The algorithm engine.
);