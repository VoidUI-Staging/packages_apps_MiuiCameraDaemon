/*
 * Copyright (c) 2018. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include <stdio.h>
#include <utils/Log.h>
#include "miautil.h"
#include "vddualcameraclearshotapi.h"

static VDDClearshotOps_t *pvcs_ops = NULL;

int VDDClearShot_APIOps_Init(VDDClearshotOps_t *VCS_Ops)
{
    XM_CHECK_NULL_POINTER(VCS_Ops, VDErrorCodeNOK);
    pvcs_ops = VCS_Ops;
    return VDErrorCodeOK;
}

int VDDClearShot_APIOps_DeInit(void)
{
    pvcs_ops = NULL;
    return VDErrorCodeOK;
}

const char *VDDualCameraClearShotVersion(void)
{
    XM_CHECK_NULL_POINTER(pvcs_ops, NULL);
    XM_CHECK_NULL_POINTER((pvcs_ops->GetVersion), NULL);

    return pvcs_ops->GetVersion();
}

VDErrorCode VDDualCameraClearShotInitialize(VDDualCameraClearShotInitParameters_t aParams, void ** aEngine)
{

    XM_CHECK_NULL_POINTER(pvcs_ops, VDErrorCodeNOK);
    XM_CHECK_NULL_POINTER((pvcs_ops->Initialize), VDErrorCodeNOK);

    return pvcs_ops->Initialize(aParams, aEngine);
}

VDErrorCode VDDualCameraClearShotSetNRForBrightness(int aBrightnessIndex, float aNoiseReductionStrength, void * aEngine)
{
    XM_CHECK_NULL_POINTER(pvcs_ops, VDErrorCodeNOK);
    XM_CHECK_NULL_POINTER((pvcs_ops->SetNRForBrightness), VDErrorCodeNOK);

    return pvcs_ops->SetNRForBrightness(aBrightnessIndex, aNoiseReductionStrength, aEngine);
}

int VDDualCameraClearShotProcess(VDYUVMultiPlaneImage * aInputImages, VDYUVMultiPlaneImage * aOutput, VDDualCameraClearShotParameters_t aParams, int * aSelectedBaseFrame, void * aEngine)

{
    XM_CHECK_NULL_POINTER(pvcs_ops, VDErrorCodeNOK);
    XM_CHECK_NULL_POINTER((pvcs_ops->Process), VDErrorCodeNOK);

    return pvcs_ops->Process(aInputImages, aOutput, aParams, aSelectedBaseFrame, aEngine);
}

VDErrorCode VDDualCameraClearShotRelease(void ** aEngine)
{
    XM_CHECK_NULL_POINTER(pvcs_ops, VDErrorCodeNOK);
    XM_CHECK_NULL_POINTER((pvcs_ops->Release), VDErrorCodeNOK);

    return pvcs_ops->Release(aEngine);
}

