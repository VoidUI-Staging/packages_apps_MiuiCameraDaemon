#include "miaifacemiui.h"

#include <cutils/properties.h>
#include <stdio.h>
#include <utils/Log.h>

#include <vector>

#undef LOG_TAG
#define LOG_TAG "MIAIFACE_MIUI"

namespace mialgo2 {
MiAIFaceMiui::MiAIFaceMiui()
{
    mPostMFacialRestorationInitThread = NULL;
    m_facialRecialRestorationInterface = NULL;
    MLOGI(Mia2LogGroupPlugin, "%s X", __func__);
}

MiAIFaceMiui::~MiAIFaceMiui()
{
    if (mPostMFacialRestorationInitThread && mPostMFacialRestorationInitThread->joinable()) {
        mPostMFacialRestorationInitThread->join();
    }
    delete mPostMFacialRestorationInitThread;
    mPostMFacialRestorationInitThread = NULL;
    if (m_facialRecialRestorationInterface) {
        m_facialRecialRestorationInterface->destroy();
        delete m_facialRecialRestorationInterface;
        m_facialRecialRestorationInterface = NULL;
    }
    MLOGI(Mia2LogGroupPlugin, "%s X", __func__);
}

void MiAIFaceMiui::initAIFace(unsigned int isPreview)
{
    if (isPreview) {
        return;
    }
    if (m_facialRecialRestorationInterface) {
        return;
    }
    {
        std::unique_lock<std::mutex> lock(mPostInitMutex);
        if (m_facialRecialRestorationInterface != NULL) {
            return;
        }
        m_facialRecialRestorationInterface = new aiface::FacialRestorationInterface();
    }
    mPostMFacialRestorationInitThread = new std::thread([this] {
        double postStartTime = PluginUtils::nowMSec();
        m_facialRecialRestorationInterface->init();
        MLOGI(Mia2LogGroupPlugin, "snapshot init with depth, costTime=%.2f",
              PluginUtils::nowMSec() - postStartTime);
    });
}

void MiAIFaceMiui::init(unsigned int isPreview)
{
    initAIFace(0);
}

void MiAIFaceMiui::process(struct MiImageBuffer *input, struct MiImageBuffer *output,
                           aiface::MIAIParams *params)
{
    if (input->width < 100 || input->height < 100) {
        MLOGE(Mia2LogGroupPlugin, "MiAIFaceMiui.cpp process data error\n");
        PluginUtils::miCopyBuffer(output, input);
        return;
    }
    if (mPostMFacialRestorationInitThread && mPostMFacialRestorationInitThread->joinable()) {
        mPostMFacialRestorationInitThread->join();
    }
    double startTime = PluginUtils::nowMSec();
    m_facialRecialRestorationInterface->process((void *)input, (void *)output, params);
    MLOGI(Mia2LogGroupPlugin,
          "MiAIFaceMiui.cpp process snapshot width=%d, height=%d, plane0=%d, pitch=%d, "
          "costTime=%.2f, process_count=%d",
          input->width, input->height, input->plane[1] - input->plane[0], input->stride,
          PluginUtils::nowMSec() - startTime, params->config.process_count);
    for (int i = 0; i < params->exif_info.FaceSRCount; i++) {
        MLOGD(Mia2LogGroupPlugin, " process facial repair FaceSrResult [%d %d %d %d]\n ",
              params->exif_info.FaceSRResults[i].id_, params->exif_info.FaceSRResults[i].stat_,
              params->exif_info.FaceSRResults[i].x_, params->exif_info.FaceSRResults[i].y_);
    }
}
} // namespace mialgo2
