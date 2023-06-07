#include "misegmentmiui.h"

#include <vector>

#undef LOG_TAG
#define LOG_TAG "MISEGMENT_MIUI"

using namespace mialgo2;

MiSegmentMiui::MiSegmentMiui()
{
    MLOGI(Mia2LogGroupPlugin, "MiSegment get algorithm library version: %s", MIBOKEH_SO_VERSION);
    mPostMiSegmentInitThread = NULL;
    mPreviewMiSegmentInitThread = NULL;
    mPostMiSegment = NULL;
    mPreviewMiSegment = NULL;
    mPreviewInitFinish = false;
    mCanAlgoDoProcess = false;
    memset(&mAnalyzeBuffer, 0, sizeof(struct MiImageBuffer));
}

MiSegmentMiui::~MiSegmentMiui()
{
    if (mPreviewMiSegmentInitThread && mPreviewMiSegmentInitThread->joinable()) {
        mPreviewMiSegmentInitThread->join();
    }
    if (mPreviewMiSegmentInitThread) {
        delete mPreviewMiSegmentInitThread;
        mPreviewMiSegmentInitThread = NULL;
    }

    if (mPostMiSegmentInitThread && mPostMiSegmentInitThread->joinable()) {
        mPostMiSegmentInitThread->join();
    }

    if (mPostMiSegmentInitThread) {
        delete mPostMiSegmentInitThread;
        mPostMiSegmentInitThread = NULL;
    }

    if (mPostMiSegment) {
        delete mPostMiSegment;
        mPostMiSegment = NULL;
    }

    if (mPreviewMiSegment) {
        delete mPreviewMiSegment;
        mPreviewMiSegment = NULL;
    }
    PluginUtils::miFreeBuffer(&mAnalyzeBuffer);
    MLOGI(Mia2LogGroupPlugin, "%s X", __func__);
}

void MiSegmentMiui::initMiSegment(unsigned int isPreview)
{
    if (isPreview) {
        {
            if (mPreviewMiSegment) {
                return;
            }
            std::unique_lock<std::mutex> lock(mPreviewInitMutex);
            if (mPreviewMiSegment) {
                return;
            }
            mPreviewMiSegment = bokeh::MiBokehFactory::NewBokeh();
        }

        const bokeh::BokehConfLoader *conf_loader =
            bokeh::MiBokehFactory::LoadConf("/system/etc/camera/mibokeh-conf.txt");
        mPreviewMiSegmentInitThread = new std::thread([this, conf_loader] {
            double previewStartTime = PluginUtils::nowMSec();
            mPreviewMiSegment->DisableBlur();
            bokeh::BokehConf preview_conf =
                conf_loader->ConfByName(bokeh::BokehConfName::BOKEH_PREVIEW);
            mPreviewMiSegment->ChangeModel(bokeh::BokehModelShape::MODEL_184_184, preview_conf);
            if (0 != mPreviewMiSegment->Init(preview_conf, NULL)) {
                mCanAlgoDoProcess = false;
                MLOGD(Mia2LogGroupPlugin, "MiSegmentMiui.cpp, preview init fail, costTime=%.2f",
                      PluginUtils::nowMSec() - previewStartTime);
            } else {
                mCanAlgoDoProcess = true;
                MLOGD(Mia2LogGroupPlugin, "MiSegmentMiui.cpp, preview init ok, costTime=%.2f",
                      PluginUtils::nowMSec() - previewStartTime);
            }
            mPreviewInitFinish = true;
        });
    } else {
        {
            if (mPostMiSegment) {
                return;
            }
            std::unique_lock<std::mutex> lock(mPostInitMutex);
            if (mPostMiSegment) {
                return;
            }
            mPostMiSegment = bokeh::MiBokehFactory::NewBokeh();
        }

        const bokeh::BokehConfLoader *conf_loader =
            bokeh::MiBokehFactory::LoadConf("/system/etc/camera/mibokeh-conf.txt");
        mPostMiSegmentInitThread = new std::thread([this, conf_loader] {
            double postStartTime = PluginUtils::nowMSec();
            mPostMiSegment->DisableBlur();
            bokeh::BokehConf post_conf = conf_loader->ConfByName(bokeh::BokehConfName::BOKEH_MOVIE);
            mPostMiSegment->ChangeModel(bokeh::BokehModelShape::MODEL_184_256_V1, post_conf);
            if (0 != mPostMiSegment->Init(post_conf, NULL)) {
                mCanAlgoDoProcess = false;
                MLOGD(Mia2LogGroupPlugin, "MiSegmentMiui.cpp, snapshot init fail, costTime=%.2f",
                      PluginUtils::nowMSec() - postStartTime);
            } else {
                mCanAlgoDoProcess = true;
                MLOGD(Mia2LogGroupPlugin, "MiSegmentMiui.cpp, snapshot init ok, costTime=%.2f",
                      PluginUtils::nowMSec() - postStartTime);
            }
        });
    }
}

void MiSegmentMiui::init(unsigned int isPreview)
{
    (void)isPreview;
    // initMiSegment(isPreview);
}

void MiSegmentMiui::process(struct MiImageBuffer *miBufMain, struct MiImageBuffer *output,
                            unsigned int isPreview, int rotate, int sceneflag, int64_t timestamp)
{
    initMiSegment(isPreview);

    if (miBufMain->width < 100 || miBufMain->height < 100) {
        MLOGD(Mia2LogGroupPlugin, "MiSegmentMiui.cpp process data error\n");
        PluginUtils::miCopyBuffer(output, miBufMain);
        return;
    }

    double startTime = PluginUtils::nowMSec();

    if (isPreview) {
        if (!mPreviewInitFinish || !mCanAlgoDoProcess) {
            PluginUtils::miCopyBuffer(output, miBufMain);
            MLOGE(Mia2LogGroupPlugin,
                  "MiSegmentMiui.cpp process preview int not ready(%d) or init fail(%d) "
                  "costTime=%.2f",
                  mPreviewInitFinish, mCanAlgoDoProcess, PluginUtils::nowMSec() - startTime);
            return;
        }

#if 0
        char str1[128];
        char str2[128];
        MiPrintBufferInfo(output, str1, "mibokeh, MiImageBuffer ouput");
        MiPrintBufferInfo(miBufMain, str2, "mibokeh, MiImageBuffer main");
        MLOGD(Mia2LogGroupPlugin, "MiSegmentMiui.cpp preview %s, %s", str1, str2);
#endif

        if (miBufMain->width != mAnalyzeBuffer.width ||
            miBufMain->height != mAnalyzeBuffer.height) {
            PluginUtils::miFreeBuffer(&mAnalyzeBuffer);
            PluginUtils::miAllocBuffer(&mAnalyzeBuffer, miBufMain->width, miBufMain->height,
                                       CAM_FORMAT_YUV_420_NV21);
        }

        PluginUtils::miCopyBuffer(&mAnalyzeBuffer, miBufMain);
        mPreviewMiSegment->UpdateSizeIn(mAnalyzeBuffer.width, mAnalyzeBuffer.height, rotate,
                                        mAnalyzeBuffer.plane[1] - mAnalyzeBuffer.plane[0],
                                        mAnalyzeBuffer.stride, false);
        mPreviewMiSegment->SetAiScenePreview(sceneflag, timestamp);
        mPreviewMiSegment->Bokeh(mAnalyzeBuffer.plane[0], mAnalyzeBuffer.plane[0]);
        PluginUtils::miCopyBuffer(output, &mAnalyzeBuffer);

        // MLOGD(Mia2LogGroupPlugin, "MiSegmentMiui.cpp process preview, width=%d, height=%d,
        // plane0=%d, pitch0=%d, plane1=%d, pitch1=%d, plane2=%d, pitch2=%d, pitch3=%d, "
        //    "costTime=%.2f", miBufMain->width, miBufMain->height, miBufMain->plane[1] -
        //    miBufMain->plane[0], miBufMain->Pitch[0], miBufMain->plane[2] - miBufMain->plane[1],
        //    miBufMain->Pitch[1], miBufMain->plane[3] - miBufMain->plane[2], miBufMain->Pitch[2],
        //    miBufMain->Pitch[3], PluginUtils::nowMSec() - startTime);
    } else {
        if (mPostMiSegmentInitThread && mPostMiSegmentInitThread->joinable()) {
            mPostMiSegmentInitThread->join();
        }
        if (!mCanAlgoDoProcess) {
            PluginUtils::miCopyBuffer(output, miBufMain);
            MLOGI(Mia2LogGroupPlugin, "MiSegmentMiui.cpp algo init fail to do memcpy costTime=%.2f",
                  PluginUtils::nowMSec() - startTime);
            return;
        }
        PluginUtils::miCopyBuffer(output, miBufMain);
        mPostMiSegment->UpdateSizeIn(output->width, output->height, rotate,
                                     output->plane[1] - output->plane[0], output->stride, false);
        mPostMiSegment->AiSceneToSceneCategory(sceneflag, 0);
        mPostMiSegment->Prepare(output->plane[0]);
        mPostMiSegment->Segment();
        mPostMiSegment->BlurAndBlend(output->plane[0], output->plane[0]);

        MLOGI(Mia2LogGroupPlugin,
              "MiSegmentMiui.cpp process snapshot width=%d, height=%d, plane0=%ld, pitch=%d, "
              "orientation=%d, costTime=%.2f",
              miBufMain->width, miBufMain->height, miBufMain->plane[1] - miBufMain->plane[0],
              miBufMain->stride, rotate, PluginUtils::nowMSec() - startTime);
    }
}
