/*******************************************************************************
Copyright(c) ArcSoft, All right reserved.

This file is ArcSoft's property. It contains ArcSoft's trade secret, proprietary
and confidential information.

The information and code contained in this file is only for authorized ArcSoft
employees to design, create, modify, or review.

DO NOT DISTRIBUTE, DO NOT DUPLICATE OR TRANSMIT IN ANY FORM WITHOUT PROPER
AUTHORIZATION.

If you are not an intended recipient of this file, you must not copy,
distribute, modify, or take any action in reliance on it.

If you have received this file in error, please immediately notify ArcSoft and
permanently delete the original and any copy of any file and any printout
thereof.
*******************************************************************************/

#ifndef _ARCSOFTDISTORTIONCORRECTION_H_
#define _ARCSOFTDISTORTIONCORRECTION_H_

#include "MiaPluginUtils.h"
#include "arcsoft_distortion_correction.h"

using namespace mialgo2;

#define HARDCODE_CAMERAID_REAR  0
#define HARDCODE_CAMERAID_FRONT 0x100

#define RUNTIMEMODE_SNAPSHOT 0x0
#define RUNTIMEMODE_PREVIEW  0x1
#define RUNTIMEMODE_RECORD   0x2

//#define USE_INTERNAL_BUFFER            1
#define INTERNAL_BUFFER_PIXEL_SIZE 4132 * 3124 * 3 / 2

#define FLAG_FILL_ALL      0
#define FLAG_FILL_METADATA 1

#define CALIBRATIONDATA_LEN 256
#define FAKE_ARC_CALIBRATION
#ifdef FAKE_ARC_CALIBRATION
#define CAL_DATA_PATH "/data/vendor/camera/arcsoft_dc_calibration.bin"
#endif

class ArcsoftDistortionCorrection
{
public:
    ArcsoftDistortionCorrection(MInt32 dsmode);
    virtual ~ArcsoftDistortionCorrection();
    void init(unsigned int camMode, unsigned int mode);
    void setcalibrationdata(const unsigned char *pCalib, unsigned int size);
    void process(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, ARC_DC_FACE *pDCFace, int camMode,
                 unsigned int rMode);
    void uninit();

private:
    void InitInternalBuffer();
    void UninitInternalBuffer();
    void CopyImage(ASVLOFFSCREEN *pSrcImg, ASVLOFFSCREEN *pDstImg);
    void setOffScreen(ASVLOFFSCREEN *pDstImg, ASVLOFFSCREEN *pSrcImg);
    void FillInternalImage(ASVLOFFSCREEN *pDstImage, ASVLOFFSCREEN *pSrcImage,
                           int flag = FLAG_FILL_ALL);
    void OutputInternalImage(ASVLOFFSCREEN *pDstImage, ASVLOFFSCREEN *pSrcImage,
                             int flag = FLAG_FILL_ALL);
    void AllocCopyImage(ASVLOFFSCREEN *pDstImage, ASVLOFFSCREEN *pSrcImage);
    void ReleaseImage(ASVLOFFSCREEN *pImage);
#ifdef FAKE_ARC_CALIBRATION
    int ReadCalibrationDataFromFile(const char *szFile, unsigned char *pCaliBuff, int dataSize);
#endif
private:
    MHandle m_hDistortionCorrectionEngine;
    ASVLOFFSCREEN mImageInput;
    ASVLOFFSCREEN mImageOutput;
    unsigned int m_u32ImagePixelSize;
    unsigned char *m_pCalibration;
    ARC_DC_INITPARAM m_DCInitParam;
    MInt32 m_dsmode;
};
#endif
