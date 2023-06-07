////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  mianodeclearshot_s.cpp
/// @brief Chi node for clearshot
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <inttypes.h>
#include <system/camera_metadata.h>
#include <utils/Timers.h>

#include "mianodeclearshot_s.h"
#include "VendorMetadataParser.h"

// NOWHINE FILE CP040: Keyword new not allowed. Use the CAMX_NEW/CAMX_DELETE functions insteads
// NOWHINE FILE NC008: Warning: - Var names should be lower camel case

#undef LOG_TAG
#define LOG_TAG "MIA_CLEARSHOT_A_S"

namespace mialgo {

////////////////////////////////////////////////////////////////////////////////////////////////////
/// MiaNodeClearShot_S::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult MiaNodeClearShot_S::Initialize(void)
{
    ALOGD("MiaNodeClearShot_S::Initialize start %s : %d", __func__, __LINE__);
    CDKResult result = MIAS_OK;

    m_pNodeInstance = new QVisidonClearshot_S();
    m_pNodeInstance->init();

    setType(NDTP_CLEARSHOT_S);
    m_processedFrame = 0;

    ALOGD("MiaNodeClearShot_S::Initialize end %s : %d", __func__, __LINE__);
    return result;
}

CDKResult MiaNodeClearShot_S::DeInit(void)
{
    CDKResult result = MIAS_OK;

    m_processedFrame = 0;

    if (m_pNodeInstance) {
        m_pNodeInstance->deinit();
        delete m_pNodeInstance;
        m_pNodeInstance = NULL;
    }

    setType(NDTP_INVALID);
    return result;
}

CDKResult MiaNodeClearShot_S::PrePareMetaData(void *meta)
{
    XM_CHECK_NULL_POINTER(meta, MIAS_INVALID_PARAM);

    m_pRequestSetting = VendorMetadataParser::allocateCopyMetadata(meta);

    return MIAS_OK;
}

CDKResult MiaNodeClearShot_S::FreeMetaData(void)
{
    if (NULL != m_pRequestSetting) {
        VendorMetadataParser::freeMetadata(m_pRequestSetting);
        m_pRequestSetting = NULL;
    } else {
        ALOGW("WARNING! Metadata is NULL");
    }

    return MIAS_OK;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
/// MiaNodeClearShot_S::ProcessRequest
////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult MiaNodeClearShot_S::ProcessRequest(NodeProcessRequestInfo* pProcessRequestInfo)
{
    //BOOL statisfied = CheckDependency(pProcessRequestInfo);
    struct MiImageBuffer inputFrame[MAX_INPUT_NUM], outputFrame;
    void* pData = NULL;
    int iso = 3200;
    int sensitivity = 0;
    int baseframe = 0;
    float ispGain = 1.0;
    char prop[PROPERTY_VALUE_MAX];
    memset(prop, 0, sizeof(prop));
    property_get("persist.camera.capture.app.clearshotdump", prop, "0");
    int iIsDumpData = (int)atoi(prop);
    property_get("persist.camera.capture.app.clearshotdbg", prop, "0");
    int iIsDbg = (int)atoi(prop);

    ALOGE("ABNER, 11111, +++++++++++++++++, inputNum %d, fmt 0x%x; outputNum %d, fmt 0x%x",
        pProcessRequestInfo->inputNum, pProcessRequestInfo->phInputBuffer[0]->format.format,
        pProcessRequestInfo->outputNum, pProcessRequestInfo->phOutputBuffer[0]->format.format);

    if (getType() != NDTP_CLEARSHOT_S) {
        ALOGE("ERR! Invalid mNodeType: %d\n", getType());
        return MIAS_INVALID_PARAM;
    }

    if (iIsDbg) {
        MiImageList_p pBuf = NULL;
        for (uint32_t i = 0; i < pProcessRequestInfo->inputNum; i++) {
            pBuf = pProcessRequestInfo->phInputBuffer[i];
            ALOGE("inbuf %d: W*H %d*%d, planeStride %d, sliceHeight %d "
                "plane_num %d, pAddr[0] %p, pAddr[1] %p, planeSize[0] %zu, planeSize[1] %zu",
                i, pBuf->format.width, pBuf->format.height,
                pBuf->planeStride, pBuf->sliceHeight, pBuf->numberOfPlanes,
                pBuf->pImageList[0].pAddr[0], pBuf->pImageList[0].pAddr[1],
                pBuf->planeSize[0], pBuf->planeSize[1]);
        }

        for (uint32_t i = 0; i < pProcessRequestInfo->outputNum; i++) {
            pBuf = pProcessRequestInfo->phOutputBuffer[i];
            ALOGE("outbuf %d: W*H %d*%d, planeStride %d, sliceHeight %d "
                "plane_num %d, pAddr[0] %p, pAddr[1] %p, planeSize[0] %zu, planeSize[1] %zu",
                i, pBuf->format.width, pBuf->format.height,
                pBuf->planeStride, pBuf->sliceHeight, pBuf->numberOfPlanes,
                pBuf->pImageList[0].pAddr[0], pBuf->pImageList[0].pAddr[1],
                pBuf->planeSize[0], pBuf->planeSize[1]);
        }

    }

    for (uint32_t i = 0; i < pProcessRequestInfo->inputNum; i++)
    {
        MiImageList_p hInput;

        hInput = pProcessRequestInfo->phInputBuffer[i];
        inputFrame[i].PixelArrayFormat = CAM_FORMAT_YUV_420_NV12;
        inputFrame[i].Width = hInput->format.width;
        inputFrame[i].Height = hInput->format.height;
        for(uint32_t j = 0; j < hInput->numberOfPlanes; j++) {
            //ABNER
            inputFrame[i].Pitch[j] = PAD_TO_SIZE(hInput->planeStride, MI_PAD_TO_64);
            inputFrame[i].Plane[j] = hInput->pImageList[0].pAddr[j];

            if (iIsDbg) {
                ALOGE("inputFrame %d: width*height %d*%d, plane %d's Pitch %d, Plane %p",
                    i, inputFrame[i].Width, inputFrame[i].Height, j, inputFrame[i].Pitch[j],
                    inputFrame[i].Plane[j]);
            }
        }
		ALOGE("huangxin iIsDumpData: %d", iIsDumpData);

        if (iIsDumpData && pProcessRequestInfo->inputNum > 1)
        {
            char inbuf[128];
            snprintf(inbuf, sizeof(inbuf), "clearshot_input_%dx%d_%d",
                    hInput->format.width, hInput->format.height, i);
            MiaUtil::DumpToFile(&(inputFrame[i]), inbuf);
         }

    }

    MiImageList_p hOutput = pProcessRequestInfo->phOutputBuffer[0];

    outputFrame.PixelArrayFormat = CAM_FORMAT_YUV_420_NV12;
    outputFrame.Width = hOutput->format.width;
    outputFrame.Height = hOutput->format.height;
    for(uint32_t j = 0; j < hOutput->numberOfPlanes; j++) {
        outputFrame.Pitch[j] = PAD_TO_SIZE(hOutput->planeStride, MI_PAD_TO_64);
        outputFrame.Plane[j] = hOutput->pImageList[0].pAddr[j];

        if (iIsDbg) {
            ALOGE("outputFrame 0: width*height %d*%d, plane %d's Pitch %d, Plane %p",
                outputFrame.Width, outputFrame.Height, j, outputFrame.Pitch[j],
                outputFrame.Plane[j]);
        }
    }

    /* Retrieve ISO */
    VendorMetadataParser::getTagValue(m_pRequestSetting, ANDROID_SENSOR_SENSITIVITY, &pData);
    if (NULL != pData)
    {
        sensitivity = *static_cast<uint32_t *>(pData);
    }

    VendorMetadataParser::getVTagValue(m_pRequestSetting, "com.qti.sensorbps.gain", &pData);
    if (NULL != pData)
    {
        ispGain = *static_cast<float*>(pData);
    }

    iso = sensitivity;
    ALOGE("MIALGO_clearshot: iso: %d",iso);

    nsecs_t e2 = systemTime();
    if (m_pNodeInstance)
        m_pNodeInstance->process(inputFrame, &outputFrame, pProcessRequestInfo->inputNum, iso, pProcessRequestInfo->cropBoundaries, &baseframe);
    nsecs_t e3 = systemTime();
    ALOGE("%s clearshot lib spent %d. ms.", __func__, (int) ((double)(e3 - e2) / 1000000000 * 1000));

    if(iIsDumpData && pProcessRequestInfo->inputNum > 1)
    {
        char outbuf[128];
        snprintf(outbuf, sizeof(outbuf), "clearshot_output_%dx%d",
                hOutput->format.width, hOutput->format.height);
        MiaUtil::DumpToFile(&outputFrame, outbuf);
    }

    m_processedFrame++;
    pProcessRequestInfo->baseFrameIndex = baseframe;
    UpdateMetaData(pProcessRequestInfo->frameNum);

    ALOGE("ABNER, 2222, -----------------");

    return MIAS_OK;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// MiaNodeClearShot_S::MiaNodeClearShot_S
////////////////////////////////////////////////////////////////////////////////////////////////////
MiaNodeClearShot_S::MiaNodeClearShot_S(): m_processedFrame(0), m_pNodeInstance(NULL), m_pRequestSetting(NULL)
{
    ALOGD("now construct -->> MiaNodeClearShot_S\n");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// MiaNodeClearShot_S::~MiaNodeClearShot_S
////////////////////////////////////////////////////////////////////////////////////////////////////
MiaNodeClearShot_S::~MiaNodeClearShot_S()
{
    ALOGD("now distruct -->> MiaNodeClearShot_S\n");
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// MiaNodeClearShot_S::UpdateMetaData
////////////////////////////////////////////////////////////////////////////////////////////////////
void MiaNodeClearShot_S::UpdateMetaData(uint64_t requestId)
{
    // NOOP
}

}
// CAMX_NAMESPACE_END
