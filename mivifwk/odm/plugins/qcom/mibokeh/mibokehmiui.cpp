#include "mibokehmiui.h"

#include <cutils/properties.h>
#include <stdio.h>
#include <utils/Log.h>

#include <vector>

#include "dynamic_blur_control.h"

#undef LOG_TAG
#define LOG_TAG "MIBOKEH_MIUI"

namespace mialgo2 {

MiBokehMiui::MiBokehMiui()
{
    MLOGI(Mia2LogGroupPlugin, "MiBokeh get algorithm library version: %s", MIBOKEH_SO_VERSION);
    mPostMiBokehInitThread = NULL;
    mPreviewMiBokehInitThread = NULL;
    mPostMiBokeh = NULL;
    mPreviewMiBokeh = NULL;
    mPreviewInitFinish = false;
    mCanAlgoDoProcess = false;
    MLOGI(Mia2LogGroupPlugin, "%s X", __func__);
}

MiBokehMiui::~MiBokehMiui()
{
    if (mPreviewMiBokehInitThread && mPreviewMiBokehInitThread->joinable()) {
        mPreviewMiBokehInitThread->join();
    }
    delete mPreviewMiBokehInitThread;
    mPreviewMiBokehInitThread = NULL;
    if (mPostMiBokehInitThread && mPostMiBokehInitThread->joinable()) {
        mPostMiBokehInitThread->join();
    }
    delete mPostMiBokehInitThread;
    mPostMiBokehInitThread = NULL;
    delete mPostMiBokeh;
    mPostMiBokeh = NULL;
    delete mPreviewMiBokeh;
    mPreviewMiBokeh = NULL;
    MLOGI(Mia2LogGroupPlugin, "%s X", __func__);
}

void MiBokehMiui::initMiBokeh(unsigned int isPreview)
{
    if (isPreview) {
        {
            if (mPreviewMiBokeh) {
                return;
            }
            std::unique_lock<std::mutex> lock(mPreviewInitMutex);
            if (mPreviewMiBokeh) {
                return;
            }
            mPreviewMiBokeh = bokeh::MiBokehFactory::NewBokeh();
        }

        const bokeh::BokehConfLoader *conf_loader =
            bokeh::MiBokehFactory::LoadConf("/system/etc/camera/mibokeh-conf.txt");
        mPreviewMiBokehInitThread = new std::thread([this, conf_loader] {
            double previewStartTime = PluginUtils::nowMSec();
            bokeh::BokehConf preview_conf =
                conf_loader->ConfByName(bokeh::BokehConfName::BOKEH_PREVIEW);
            mPreviewMiBokeh->ChangeModel(bokeh::BokehModelShape::MODEL_184_184, preview_conf);
            if (0 != mPreviewMiBokeh->Init(preview_conf, NULL)) {
                mCanAlgoDoProcess = false;
                MLOGE(Mia2LogGroupPlugin, "MiBokehMiui.cpp, preview init fail, costTime=%.2f",
                      PluginUtils::nowMSec() - previewStartTime);
            } else {
                mCanAlgoDoProcess = true;
                MLOGE(Mia2LogGroupPlugin, "MiBokehMiui.cpp, preview init ok, costTime=%.2f",
                      PluginUtils::nowMSec() - previewStartTime);
            }
            mPreviewInitFinish = true;
        });
    } else {
        if (mPostMiBokeh) {
            return;
        }
        {
            std::unique_lock<std::mutex> lock(mPostInitMutex);
            if (mPostMiBokeh != NULL) {
                return;
            }
            mPostMiBokeh = bokeh::MiBokehFactory::NewBokeh();
        }

        const bokeh::BokehConfLoader *conf_loader =
            bokeh::MiBokehFactory::LoadConf("/system/etc/camera/mibokeh-conf.txt");
        mPostMiBokehInitThread = new std::thread([this, conf_loader] {
            double postStartTime = PluginUtils::nowMSec();
            bokeh::BokehConf post_conf = conf_loader->ConfByName(bokeh::BokehConfName::BOKEH_MOVIE);
#ifdef FLAGS_MIBOKEH_855
            mPostMiBokeh->ChangeModel(bokeh::BokehModelShape::MODEL_512_512_V4, post_conf);
#else
            mPostMiBokeh->ChangeModel(bokeh::BokehModelShape::MODEL_512_512_MN_V2, post_conf);
#endif
            if (0 != mPostMiBokeh->Init(post_conf, NULL)) {
                mCanAlgoDoProcess = false;
                MLOGI(Mia2LogGroupPlugin,
                      "MiBokehMiui.cpp, snapshot init with depth fail, costTime=%.2f",
                      PluginUtils::nowMSec() - postStartTime);
            } else {
                mCanAlgoDoProcess = true;
                MLOGI(Mia2LogGroupPlugin,
                      "MiBokehMiui.cpp, snapshot init with depth ok, costTime=%.2f",
                      PluginUtils::nowMSec() - postStartTime);
            }
        });
    }
}

void MiBokehMiui::init(unsigned int isPreview)
{
    (void)isPreview;
    // initMiBokeh(isPreview);
}

void MiBokehMiui::GetRawDepthImage(void *depthImage, bool flip)
{
    mPostMiBokeh->GetRawDepthImage(depthImage, flip);
}

void MiBokehMiui::process(struct MiImageBuffer *miBufMain, struct MiImageBuffer *output,
                          unsigned int isPreview, int rotate, int64_t timestamp,
                          float fNumberApplied, int sceneflag)
{
    initMiBokeh(isPreview);

    if (miBufMain->width < 100 || miBufMain->height < 100) {
        MLOGE(Mia2LogGroupPlugin, "MiBokehMiui.cpp process data error\n");
        PluginUtils::miCopyBuffer(output, miBufMain);
        return;
    }

    double startTime = PluginUtils::nowMSec();

    if (isPreview) {
        if (!mPreviewInitFinish || !mCanAlgoDoProcess) {
            PluginUtils::miCopyBuffer(output, miBufMain);
            MLOGE(Mia2LogGroupPlugin,
                  "MiBokehMiui.cpp preview init not ready(%d) or fail(%d) costTime=%.2f",
                  mPreviewInitFinish, mCanAlgoDoProcess, PluginUtils::nowMSec() - startTime);
            return;
        }
        mPreviewMiBokeh->UpdateSizeIn(miBufMain->width, miBufMain->height, rotate,
                                      miBufMain->plane[1] - miBufMain->plane[0], miBufMain->stride,
                                      false);
        mPreviewMiBokeh->SetAiScenePreview(sceneflag, timestamp);
        mPreviewMiBokeh->UpdateAperature(fNumberApplied);
        mPreviewMiBokeh->Bokeh(miBufMain->plane[0], miBufMain->plane[0]);
        PluginUtils::miCopyBuffer(output, miBufMain);
        // MLOGE(Mia2LogGroupPlugin, "MiBokehMiui.cpp process preview, width=%d, height=%d,
        // plane0=%d, pitch0=%d, plane1=%d, pitch1=%d, plane2=%d, pitch2=%d, pitch3=%d, "
        //    "costTime=%.2f", miBufMain->width, miBufMain->height, miBufMain->plane[1] -
        //    miBufMain->plane[0], miBufMain->stride[0], miBufMain->plane[2] - miBufMain->plane[1],
        //    miBufMain->stride[1], miBufMain->plane[3] - miBufMain->plane[2], miBufMain->stride[2],
        //    miBufMain->stride[3], PluginUtils::nowMSec() - startTime);
    } else {
        if (mPostMiBokehInitThread && mPostMiBokehInitThread->joinable()) {
            mPostMiBokehInitThread->join();
        }
        if (!mCanAlgoDoProcess) {
            PluginUtils::miCopyBuffer(output, miBufMain);
            MLOGE(Mia2LogGroupPlugin, "MiBokehMiui.cpp snapshot init fail to memcpy, costTime=%.2f",
                  PluginUtils::nowMSec() - startTime);
            return;
        }
        mPostMiBokeh->UpdateSizeIn(miBufMain->width, miBufMain->height, rotate,
                                   miBufMain->plane[1] - miBufMain->plane[0], miBufMain->stride,
                                   false);
        mPostMiBokeh->AiSceneToSceneCategory(sceneflag, 0);
        mPostMiBokeh->UpdateAperature(fNumberApplied);
        mPostMiBokeh->Bokeh(miBufMain->plane[0], output->plane[0]);
        MLOGI(Mia2LogGroupPlugin,
              "MiBokehMiui.cpp process snapshot width=%d, height=%d, plane0=%d, stride=%d, "
              "orientation=%d, costTime=%.2f",
              miBufMain->width, miBufMain->height, miBufMain->plane[1] - miBufMain->plane[0],
              miBufMain->stride, rotate, PluginUtils::nowMSec() - startTime);
    }
}
/*begin add for front single camera portrait 3d light*/
MInt32 MiBokehMiui::process(struct MiImageBuffer *miBufMain, struct MiImageBuffer *output,
                            MUInt8 *outputDepth, unsigned int isPreview, int rotate,
                            int64_t timestamp, unsigned int currentFlip, float fNumberApplied,
                            int sceneflag)
{
    MInt32 depthSize = 0;
    initMiBokeh(isPreview);

    if (miBufMain->width < 100 || miBufMain->height < 100) {
        MLOGE(Mia2LogGroupPlugin, "MiBokehMiui.cpp process data error\n");
        PluginUtils::miCopyBuffer(output, miBufMain);
        return 0;
    }

    double startTime = PluginUtils::nowMSec();

    if (isPreview) {
        if (!mPreviewInitFinish || !mCanAlgoDoProcess) {
            PluginUtils::miCopyBuffer(output, miBufMain);
            MLOGE(Mia2LogGroupPlugin,
                  "MiBokehMiui.cpp preview init not ready(%d) or fail(%d), costTime=%.2f",
                  mPreviewInitFinish, mCanAlgoDoProcess, PluginUtils::nowMSec() - startTime);
            return 0;
        }
        mPreviewMiBokeh->UpdateSizeIn(miBufMain->width, miBufMain->height, rotate,
                                      miBufMain->plane[1] - miBufMain->plane[0], miBufMain->stride,
                                      false);
        mPreviewMiBokeh->SetAiScenePreview(sceneflag, timestamp);
        mPreviewMiBokeh->UpdateAperature(fNumberApplied);
        mPreviewMiBokeh->Bokeh(miBufMain->plane[0], miBufMain->plane[0]);
        PluginUtils::miCopyBuffer(output, miBufMain);
    } else {
        if (mPostMiBokehInitThread && mPostMiBokehInitThread->joinable()) {
            mPostMiBokehInitThread->join();
        }
        if (!mCanAlgoDoProcess) {
            PluginUtils::miCopyBuffer(output, miBufMain);
            MLOGE(Mia2LogGroupPlugin,
                  "MiBokehMiui.cpp snapshot algo init fail to do memcpy, costTime=%.2f",
                  PluginUtils::nowMSec() - startTime);
            return 0;
        }
        mPostMiBokeh->UpdateSizeIn(miBufMain->width, miBufMain->height, rotate,
                                   miBufMain->plane[1] - miBufMain->plane[0], miBufMain->stride,
                                   false);
        mPostMiBokeh->UpdateAperature(fNumberApplied);
        mPostMiBokeh->AiSceneToSceneCategory(sceneflag, 0);
        mPostMiBokeh->Bokeh(miBufMain->plane[0], miBufMain->plane[0]);
        PluginUtils::miCopyBuffer(output, miBufMain);
        MLOGI(Mia2LogGroupPlugin,
              "MiBokehMiui.cpp process snapshot width=%d, height=%d, plane0=%d, stride=%d, "
              "orientation=%d, costTime=%.2f",
              miBufMain->width, miBufMain->height, miBufMain->plane[1] - miBufMain->plane[0],
              miBufMain->stride, rotate, PluginUtils::nowMSec() - startTime);

        // depth data begin
        bokeh::MIBOKEH_DEPTHINFO *img = new bokeh::MIBOKEH_DEPTHINFO;
        mPostMiBokeh->GetMiDepthImage(img, currentFlip);

        disparity_fileheader_t header;
        memset(&header, 0, sizeof(disparity_fileheader_t));
        header.i32Header = 0x80;
        header.i32HeaderLength = sizeof(disparity_fileheader_t);
        header.i32PointX = miBufMain->width;
        header.i32PointY = miBufMain->height;
        header.i32BlurLevel = findBlurLevelByFNumber(fNumberApplied, false);
        header.i32DataLength = img->lDepthDataSize;
        depthSize = header.i32DataLength;
        memcpy(outputDepth, (void *)&header, sizeof(disparity_fileheader_t));
        memcpy(outputDepth + sizeof(disparity_fileheader_t), img->pDepthData, img->lDepthDataSize);
        MLOGI(Mia2LogGroupPlugin,
              "MiBokehMiui.cpp, memcpy depth data end.i32HeaderLength %d i32PointX [%d %d] size=%d "
              "depthSize %d pDepthData %p outputDepthdata %p",
              header.i32HeaderLength, header.i32PointX, header.i32PointY, img->lDepthDataSize,
              depthSize, img->pDepthData, outputDepth + sizeof(disparity_fileheader_t));

        free(img->pDepthData);
        delete img;
        // depth data end
    }
    return depthSize;
}
/*end add for front single camera portrait 3d light*/

} // namespace mialgo2
