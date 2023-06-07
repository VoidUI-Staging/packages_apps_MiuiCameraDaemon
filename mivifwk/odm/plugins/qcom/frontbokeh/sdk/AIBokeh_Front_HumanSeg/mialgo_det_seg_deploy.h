/*******************************************************************
 *Filename: mialgo_det_ldc.h
 *Copyright:   XiaoMi Co., Ltd.
 *
 *Description: the pipeline of det and ldc model
 *
 *@author:     Huangwei14@xiaomi.com
 *@concat:     15800758732
 *@verson:     2021-12-21
 ******************************************************************/
#pragma once
#include "mialgo_layer.h"
typedef struct {
    std::string det_model_name;
    std::string seg_model_name;
    std::string asset_path;
    int srcImageWidth;
    int srcImageHeight;
    int dstImageWidth = -1;
    int dstImageHeight = -1;
    int modelType;  // 0->rgb, 1->nv12, nv21, 2->LDC(指定)
    bool use_detector;
    MialgoAiVisionRuntime_AIO detRuntime;
    MialgoAiVisionPriority_AIO detPriority;
    MialgoAiVisionPerformance_AIO detPerformance;
    MialgoAiVisionRuntime_AIO segRuntime;
    MialgoAiVisionPriority_AIO segPriority;
    MialgoAiVisionPerformance_AIO segPerformance;
} ModelInitParams;  // model init params

typedef struct {
    int rotate_angle = 0;
    MialgoImg_AIO potraitMask;
    MialgoImg_AIO srcImage;
} ModelProcParams;  // model proc params

int ModelLaunch(void** ImageEngine, ModelInitParams* launchparams);
int ModelRun(void** ImageEngine, ModelProcParams* procparams);
int ModelRelease(void** ImageEngine);
