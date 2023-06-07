/*********************************************************************
 *  Filename:    mialgo_seg_ldc.h
 *  Copyright:   XiaoMi Co., Ltd.
 *
 *  Description: mialgo seg header file. Main functions for segmentation
 *  includes model init, model process, model release
 *
 *  @author:     yangguide@xiaomi.com
 *  @version     2021-12-09
 *********************************************************************/

#ifndef INCLUDE_MIALGO_SEG_LDC_H_
#define INCLUDE_MIALGO_SEG_LDC_H_

#include "mialgo_layer.h"

typedef struct {
    std::string config_path;
    int srcImageWidth;
    int srcImageHeight;
    MialgoAiVisionRuntime_AIO runtime;
    MialgoAiVisionPriority_AIO priority;
    MialgoAiVisionPerformance_AIO performance;
} LDCSegModelInitParams;  // model init params

typedef struct {
    int rotation_angle;
    MialgoDetSegImg srcImage;
    MialgoDetSegImg potraitMask;
} LDCSegModelProcParams;  // model process params

/*
Get aivs version
Get Mialgo engine vesion
Maybe no return value
*/
const char *MIALGO_LDCSegModelVersionGet();

// model init, return false when failed
int MIALGO_LDCSegInitWhenLaunch(void **SegEngine,
                                LDCSegModelInitParams *pstModelInitParams);

// model process, return false when failed
int MIALGO_LDCSegProc(void **SegEngine,
                      LDCSegModelProcParams *pstModelProcParams);

// model release, return false when failed
int MIALGO_LDCSegDestory(void **SegEngine);

#endif  // INCLUDE_MIALGO_SEG_LDC_H_
