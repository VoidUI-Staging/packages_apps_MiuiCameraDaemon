#include "MiAIFacialRepair.h"

#include "chioemvendortag.h"

#define EXCLUSION_OF_SN  2
#define EXCLUSION_OF_LLS 4

namespace mialgo2 {
MiAIFacialRepair::~MiAIFacialRepair() {}

int MiAIFacialRepair::initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface)
{
    mFrameworkCameraId = createInfo->frameworkCameraId;
    MLOGD(Mia2LogGroupPlugin, "%s:%d call Initialize... %p, mFrameworkCameraId: %d", __func__,
          __LINE__, this, mFrameworkCameraId);
    double startTime = PluginUtils::nowMSec();
    mNodeInstance = NULL;
    mEnableStatus = 0;
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.aiportraitdeblur.enabled", prop, "6");
    mEnableStatus = (int32_t)atoi(prop);
    memset(&m_facialrepairparams, 0, sizeof(aiface::MIAIParams));

    if (mEnableStatus && NULL == mNodeInstance) {
        mNodeInstance = new MiAIFaceMiui();
        mNodeInstance->init(false);
    }
    mNodeInterface = nodeInterface;
    MLOGI(Mia2LogGroupPlugin, "MiAIFacialRepair");

    return MIA_RETURN_OK;
}

void MiAIFacialRepair::destroy()
{
    MLOGI(Mia2LogGroupPlugin, "MiAIFacialRepair");
    if (NULL != mNodeInstance) {
        delete mNodeInstance;
        mNodeInstance = NULL;
    }
}

bool MiAIFacialRepair::isEnabled(MiaParams settings)
{
    bool superNightEnable = false;
    void *pData = NULL;
    VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.supernight.enabled", &pData);
    if ((mEnableStatus & EXCLUSION_OF_SN) && NULL != pData) {
        superNightEnable = *static_cast<uint8_t *>(pData);
        MLOGD(Mia2LogGroupPlugin, "MiAIFacialRepair get supernight enabled %d\n", superNightEnable);
    } else {
        MLOGD(Mia2LogGroupPlugin, "MiAIFacialRepair, get supernight status: %d\n", mEnableStatus);
    }
    if (superNightEnable) {
        MLOGD(Mia2LogGroupPlugin, "MiAIFacialRepair bypass, superNightEnable=%d\n",
              superNightEnable);
        return false;
    }
    int32_t llsEnabled = 0;
    pData = NULL;
    VendorMetadataParser::getVTagValue(settings.metadata, "com.qti.stats_control.is_lls_needed",
                                       &pData);
    if ((mEnableStatus & EXCLUSION_OF_LLS) && NULL != pData) {
        llsEnabled = *static_cast<int32_t *>(pData);
    } else {
        MLOGW(Mia2LogGroupPlugin, "MIAI_DEBLUR, llsEnabled status: %d\n", mEnableStatus);
    }
    if (llsEnabled) {
        MLOGD(Mia2LogGroupPlugin, "MiAIFacialRepair bypass, llsEnabled=%d\n", llsEnabled);
        return false;
    }
    float zoom = 1.0f;
    pData = NULL;
    VendorMetadataParser::getTagValue(settings.metadata, ANDROID_CONTROL_ZOOM_RATIO, &pData);
    if (NULL != pData) {
        zoom = *static_cast<float *>(pData);
        MLOGD(Mia2LogGroupPlugin, "get zoom =%f", zoom);
    } else {
        MLOGI(Mia2LogGroupPlugin, "get android zoom ratio failed");
    }
    if (abs(zoom - 1) > 0.1 || mFrameworkCameraId == CAM_COMBMODE_REAR_ULTRA) {
        MLOGD(Mia2LogGroupPlugin, "%s:%d MiAIFacialRepair Bypass : not 1x", __func__, __LINE__);
        return false;
    }
    return mEnableStatus;
}

ProcessRetStatus MiAIFacialRepair::processRequest(ProcessRequestInfo *pProcessRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.aiportraitdeblur.dump", prop, "0");
    int32_t iIsDumpData = (int32_t)atoi(prop);
    memset(prop, 0, sizeof(prop));
    property_get("persist.vendor.camera.aiportraitdeblur.bypass", prop, "0");
    int32_t aiportraitBypassed = (int32_t)atoi(prop);
    int inputNum = pProcessRequestInfo->phInputBuffer.size();
    MiImageBuffer inputFrame, outputFrame;
    inputFrame.format = pProcessRequestInfo->phInputBuffer[0].format.format;
    inputFrame.width = pProcessRequestInfo->phInputBuffer[0].format.width;
    inputFrame.height = pProcessRequestInfo->phInputBuffer[0].format.height;
    inputFrame.plane[0] = pProcessRequestInfo->phInputBuffer[0].pAddr[0];
    inputFrame.plane[1] = pProcessRequestInfo->phInputBuffer[0].pAddr[1];
    inputFrame.stride = pProcessRequestInfo->phInputBuffer[0].planeStride;
    inputFrame.scanline = pProcessRequestInfo->phInputBuffer[0].sliceheight;
    inputFrame.numberOfPlanes = pProcessRequestInfo->phInputBuffer[0].numberOfPlanes;

    outputFrame.format = pProcessRequestInfo->phOutputBuffer[0].format.format;
    outputFrame.width = pProcessRequestInfo->phOutputBuffer[0].format.width;
    outputFrame.height = pProcessRequestInfo->phOutputBuffer[0].format.height;
    outputFrame.plane[0] = pProcessRequestInfo->phOutputBuffer[0].pAddr[0];
    outputFrame.plane[1] = pProcessRequestInfo->phOutputBuffer[0].pAddr[1];
    outputFrame.stride = pProcessRequestInfo->phOutputBuffer[0].planeStride;
    outputFrame.scanline = pProcessRequestInfo->phOutputBuffer[0].sliceheight;
    outputFrame.numberOfPlanes = pProcessRequestInfo->phOutputBuffer[0].numberOfPlanes;

    MLOGD(Mia2LogGroupPlugin,
          "MiAI_PORTRAIT_DEBLUR %s:%d >>> input width: %d, height: %d, stride: %d, scanline: %d, "
          "numberOfPlanes: %d, output stride: %d scanline: %d, format:%d, inputNum:%d",
          __func__, __LINE__, inputFrame.width, inputFrame.height, inputFrame.stride,
          inputFrame.scanline, inputFrame.numberOfPlanes, outputFrame.stride, outputFrame.scanline,
          inputFrame.format, inputNum);

    camera_metadata_t *metadata = pProcessRequestInfo->phInputBuffer[0].pMetadata;
    camera_metadata_t *output_metadata = pProcessRequestInfo->phOutputBuffer[0].pMetadata;
    void *data = NULL;
    int rotate = 0;
    VendorMetadataParser::getTagValue(metadata, ANDROID_JPEG_ORIENTATION, &data);
    if (NULL != data) {
        int orientation = *static_cast<int32_t *>(data);
        rotate = (orientation) % 360;
        MLOGI(Mia2LogGroupPlugin,
              "MiAI_PORTRAIT_DEBLUR MiaNodeMiBokehn getMetaData jpeg orientation  %d", rotate);
    }
    int sensitivity = -1;
    data = NULL;
    VendorMetadataParser::getTagValue(metadata, ANDROID_SENSOR_SENSITIVITY, &data);
    if (NULL != data) {
        sensitivity = *static_cast<int32_t *>(data);
        MLOGI(Mia2LogGroupPlugin, "MiAIFacialRepair get sensitivity %d", sensitivity);
    } else {
        sensitivity = 100;
    }

    float ispGain = 1.0;
    data = NULL;
    VendorMetadataParser::getVTagValue((camera_metadata_t *)metadata, "com.qti.sensorbps.gain",
                                       &data);
    if (NULL != data) {
        ispGain = *static_cast<float *>(data);
        MLOGD(Mia2LogGroupPlugin, "MiaNodeSR get ispGain %f", ispGain);
    }

    int iso = (int)(ispGain * 100) * sensitivity / 100;
    int32_t perFrameEv = 0;
    data = NULL;
    VendorMetadataParser::getTagValue(metadata, ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &data);
    if (NULL != data) {
        perFrameEv = *static_cast<int32_t *>(data);
        MLOGI(Mia2LogGroupPlugin, "MiAIFacialRepair get Ev %d", perFrameEv);
    }
    float luxIndex = 0.0f;
    data = NULL;
    VendorMetadataParser::getVTagValue(metadata, "org.quic.camera2.statsconfigs.AECFrameControl",
                                       &data);
    if (NULL != data) {
        luxIndex = static_cast<AECFrameControl *>(data)->luxIndex;
    } else {
        MLOGI(Mia2LogGroupPlugin, "MiAI_PORTRAIT_DEBLUR getMetaData AECFrameControl failed");
    }

    data = NULL;
    uint8_t miaifaceEnable = 0;
    const char *miaiface = "xiaomi.aiportraitdeblur.enabled";
    VendorMetadataParser::getVTagValue(metadata, miaiface, &data);
    if (NULL != data) {
        miaifaceEnable = *static_cast<uint8_t *>(data);
        MLOGI(Mia2LogGroupPlugin, "MiAI_PORTRAIT_DEBLUR getMetaData MiAIPortraiteDeblurEnabled  %d",
              miaifaceEnable);
    } else {
        MLOGE(Mia2LogGroupPlugin,
              "MiAI_PORTRAIT_DEBLUR can not found tag \"xiaomi.aiportraitdeblur.enabled\"");
    }

    if (iIsDumpData) {
        char inputbuf[128];
        snprintf(inputbuf, sizeof(inputbuf),
                 "MIAIFace_input_%dx%d_iso_%d_s_%d_g_%.3f_lux_%.2f_ev_%d", inputFrame.width,
                 inputFrame.height, iso, sensitivity, ispGain, luxIndex, perFrameEv);
        PluginUtils::dumpToFile(inputbuf, &inputFrame);
    }

    int ret = PROCSUCCESS;
    m_facialrepairparams.config.iso = sensitivity;
    m_facialrepairparams.config.orientation = rotate;
    if ((luxIndex >= 0.0f && luxIndex <= 300.0f) && mNodeInstance != NULL && !aiportraitBypassed) {
        mNodeInstance->process(&inputFrame, &outputFrame, &m_facialrepairparams);
    } else {
        ret = PluginUtils::miCopyBuffer(&outputFrame, &inputFrame);
        MLOGD(Mia2LogGroupPlugin,
              "MiAI_PORTRAIT_DEBLUR AIFaceBypassed, status: aiportraitBypassed = %d, "
              "mNodeInstance=%p",
              aiportraitBypassed, mNodeInstance);
    }
    if (m_facialrepairparams.exif_info.FaceSRCount > 0) {
        miaifaceEnable = 1;
        if (mNodeInterface.pSetResultMetadata != NULL) {
            std::stringstream exifmsg;
            exifmsg << "mifacialrepair:{";
            exifmsg << " FREnable:" << miaifaceEnable
                    << " {fstag:" << m_facialrepairparams.exif_info.FaceSRTag;
            exifmsg << " fscount:" << m_facialrepairparams.exif_info.FaceSRCount;
            exifmsg << " fsret:";
            for (int i = 0; i < m_facialrepairparams.exif_info.FaceSRCount; i++) {
                exifmsg << "{id:" << m_facialrepairparams.exif_info.FaceSRResults[i].id_;
                exifmsg << "loc:" << m_facialrepairparams.exif_info.FaceSRResults[i].x_;
                exifmsg << "," << m_facialrepairparams.exif_info.FaceSRResults[i].y_;
                exifmsg << "state:" << m_facialrepairparams.exif_info.FaceSRResults[i].stat_ << "}";
            }
            exifmsg << "}";
            std::string results = exifmsg.str();
            mNodeInterface.pSetResultMetadata(mNodeInterface.owner, pProcessRequestInfo->frameNum,
                                              pProcessRequestInfo->timeStamp, results);
        }
    } else {
        miaifaceEnable = 0;
    }
    int portraitdeblur_tag_result =
        VendorMetadataParser::setVTagValue(output_metadata, miaiface, &miaifaceEnable, 1);
    MLOGD(Mia2LogGroupPlugin,
          "MiAI_PORTRAIT_DEBLUR setVTagValue, status: process_count = %d, "
          "portraitdeblur_tag_result=%d",
          m_facialrepairparams.exif_info.FaceSRCount, portraitdeblur_tag_result);
    if (ret == -1) {
        resultInfo = PROCFAILED;
    }
    if (iIsDumpData) {
        char outputbuf[128];
        snprintf(outputbuf, sizeof(outputbuf),
                 "MIAIFace_output_%dx%d_iso_%d_s_%d_g_%.3f_lux_%.2f_ev_%d", outputFrame.width,
                 outputFrame.height, iso, sensitivity, ispGain, luxIndex, perFrameEv);
        PluginUtils::dumpToFile(outputbuf, &outputFrame);
    }
    return resultInfo;
};

ProcessRetStatus MiAIFacialRepair::processRequest(ProcessRequestInfoV2 *pProcessRequestInfo)
{
    ProcessRetStatus resultInfo = PROCSUCCESS;
    return resultInfo;
}

int MiAIFacialRepair::flushRequest(FlushRequestInfo *flushRequestInfo)
{
    return 0;
}

} // namespace mialgo2
