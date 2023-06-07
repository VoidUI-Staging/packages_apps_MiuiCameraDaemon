/*
 * AIIECapture.cpp
 *
 *  Created on: 2019-11-20
 *
 */

#include "AIIECapture.h"

#include <cutils/properties.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utils/Log.h>

#include <iostream>

#undef LOG_TAG
#define LOG_TAG "AIIECapture"

AIIECapture::AIIECapture()
{
    pHandle = NULL;
}

AIIECapture::~AIIECapture()
{
    MLOGD(Mia2LogGroupPlugin, "~AIIECapture()");
    destory();
}

void AIIECapture::destory()
{
    if (pHandle != NULL) {
        ALGO_IE_CAPTURE_FREEPARAM stFreeInfo;
        memset(&stFreeInfo, 0, sizeof(stFreeInfo));
        stFreeInfo.pHandle = pHandle;
        ALGO_IE_CaptureFree(stFreeInfo);
        pHandle = NULL;
    }
}

const char *AIIECapture::getVersion()
{
    const char *version = ALGO_IE_CaptureGetVersion();

    return version;
}

int AIIECapture::CheckFiles(std::string &files)
{
    int result = -1;

    if (access(files.c_str(), F_OK & W_OK) == 0) {
        return 0;
    }

    if (0 != mkdir(files.c_str(), 0770)) {
        MLOGE(Mia2LogGroupPlugin, " AIIECapture create erro %s", files.c_str());
    } else {
        result = 0;
    }
    return result;
}

int AIIECapture::init(MiImageBuffer *inputBuffer)
{
    int result = 0;
    ALGO_IE_CAPTURE_INITPARAM stInitInfo;
    ALGO_IE_CAPTURE_INITOUT stInitOut;
    memset(&stInitInfo, 0, sizeof(stInitInfo));
    memset(&stInitOut, 0, sizeof(stInitOut));

    stInitInfo.image_width = inputBuffer->width;
    stInitInfo.image_height = inputBuffer->height;
    stInitInfo.config_path = MI_AIIEALGO_PARAMS_PATH_1;
    if (access(stInitInfo.config_path.c_str(), W_OK) != 0) {
        stInitInfo.config_path = MI_AIIEALGO_PARAMS_PATH_2;
        if (CheckFiles(stInitInfo.config_path) != 0) {
            stInitInfo.config_path = MI_AIIEALGO_PARAMS_PATH_3;
        }
    }
    stInitInfo.dla_path = MI_AIIEALGO_PARAMS_DAL_PATH;
    stInitOut.pHandle = pHandle;

    MLOGD(Mia2LogGroupPlugin, "%s:%d AIIECapture Init w = %d,h = %d", __func__, __LINE__,
          stInitInfo.image_width, stInitInfo.image_height);

    result = ALGO_IE_CaptureInit(stInitInfo, stInitOut);
    if (result != 0) {
        MLOGE(Mia2LogGroupPlugin, "%s:%d AIIECapture Init process fail", __func__, __LINE__);
        return result;
    } else {
        MLOGD(Mia2LogGroupPlugin, " AIIECapture Init process success");
    }

    pHandle = stInitOut.pHandle;
    if (pHandle == NULL) {
        MLOGE(Mia2LogGroupPlugin, "%s:%d AIIECapture Init process fail, pHandle = NULL", __func__,
              __LINE__);
    }
    return result;
}

int AIIECapture::process(struct MiImageBuffer *inputBuffer, struct MiImageBuffer *outputBuffer)
{
    int result = 0;
    ALGO_IE_CAPTURE_INPARAM stInputInfo;
    ALGO_IE_CAPTURE_RUNOUT stOutput;
    memset(&stInputInfo, 0, sizeof(stInputInfo));
    memset(&stOutput, 0, sizeof(stOutput));

    stInputInfo.iImgDataYFD = inputBuffer->fd[0];
    stInputInfo.iImgDataUVFD = inputBuffer->fd[1];
    stInputInfo.pHandle = pHandle;
    stInputInfo.iImgHeight = inputBuffer->height;
    stInputInfo.iImgWidth = inputBuffer->width;
    stInputInfo.iImgStride = inputBuffer->stride;
    stInputInfo.pImgDataY = inputBuffer->plane[0];
    stInputInfo.pImgDataUV = inputBuffer->plane[1];

    if (inputBuffer->format == 35) {
        stInputInfo.iImgFormat = 2;
    } else if (inputBuffer->format == 17) {
        stInputInfo.iImgFormat = 1;
    }

    stOutput.iOutDataYFD = outputBuffer->fd[0];
    stOutput.iOutDataUVFD = outputBuffer->fd[1];
    stOutput.iImgWidth = outputBuffer->width;
    stOutput.iImgHeight = outputBuffer->height;
    stOutput.iImgStride = outputBuffer->stride;
    stOutput.pOutDataY = outputBuffer->plane[0];
    stOutput.pOutDataUV = outputBuffer->plane[1];

    MLOGD(Mia2LogGroupPlugin,
          "ALGO_IE_CaptureProcess  stInputInfo.iImgFormat = %d, InputFD=%d,%d ,stInputInfo.pHandle "
          "= %p , stInputInfo.iImgHeight = %d ,stInputInfo.iImgWidth = %d ,stInputInfo.iImgStride "
          "= %d ，stInputInfo.pImgDataY = %p , stInputInfo.pImgDataUV = %p",

          stInputInfo.iImgFormat, stInputInfo.iImgDataYFD, stInputInfo.iImgDataUVFD,
          stInputInfo.pHandle, stInputInfo.iImgHeight, stInputInfo.iImgWidth,
          stInputInfo.iImgStride, stInputInfo.pImgDataY, stInputInfo.pImgDataUV);

    MLOGD(Mia2LogGroupPlugin,
          "ALGO_IE_CaptureProcess stOutput.iFD=%d,%d ,stOutput.pHandle = %p , stOutput.iImgHeight "
          "= %d ,stOutput.iImgWidth = %d ,stOutput.iImgStride = %d stOutput.pImgDataY = %p , "
          "stOutput.pImgDataUV = %p",

          stOutput.iOutDataYFD, stOutput.iOutDataUVFD, stInputInfo.pHandle, stOutput.iImgHeight,
          stOutput.iImgWidth, stOutput.iImgStride, stOutput.pOutDataY, stOutput.pOutDataUV);

    MLOGD(Mia2LogGroupPlugin, "begin ALGO_IE_CaptureProcess process");
    result = ALGO_IE_CaptureProcess(stInputInfo, stOutput);
    MLOGD(Mia2LogGroupPlugin, "end ALGO_IE_CaptureProcess process");
    if (result != 0) {
        MLOGE(Mia2LogGroupPlugin, "%s:%d ALGO_IE_CaptureProcess process fail %d", __func__,
              __LINE__, result);
    } else {
        MLOGD(Mia2LogGroupPlugin, "end ALGO_IE_CaptureProcess process success");
    }
    return result;
}
