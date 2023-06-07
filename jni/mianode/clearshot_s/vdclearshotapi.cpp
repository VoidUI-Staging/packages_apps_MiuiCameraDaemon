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
#include "vdclearshotapi.h"

static VDClearshotOps_t *pvcs_ops = NULL;

int VDClearShot_APIOps_Init(VDClearshotOps_t *VCS_Ops)
{
    XM_CHECK_NULL_POINTER(VCS_Ops, VD_NOK);
    pvcs_ops = VCS_Ops;
    return VD_OK;
}

int VDClearShot_APIOps_DeInit(void)
{
    pvcs_ops = NULL;
    return VD_OK;
}

const char *VDClearShot_GetVersion(void)
{
    XM_CHECK_NULL_POINTER(pvcs_ops, NULL);
    XM_CHECK_NULL_POINTER((pvcs_ops->GetVersion), NULL);

    return pvcs_ops->GetVersion();
}

VDErrorType VDClearShot_Initialize(VDClearShotInitParameters initparams, int numberOfCores,
                                   int activeCPUMask, void **engine)
{

    XM_CHECK_NULL_POINTER(pvcs_ops, VD_NOK);
    XM_CHECK_NULL_POINTER((pvcs_ops->Initialize), VD_NOK);

    return pvcs_ops->Initialize(initparams, numberOfCores, activeCPUMask, engine);
}

int VDClearShot_SetNRForBrightness(int brightnessIndex, float strength, void *engine)
{
    XM_CHECK_NULL_POINTER(pvcs_ops, VD_NOK);
    XM_CHECK_NULL_POINTER((pvcs_ops->SetNRForBrightness), VD_NOK);

    return pvcs_ops->SetNRForBrightness(brightnessIndex, strength, engine);
}

int VDClearShot_Process(VDYUVMultiPlaneImage *inputImages, VDYUVMultiPlaneImage *output,
                        VDClearShotRuntimeParams params, int *selectedBaseFrame, void *engine)
{
    XM_CHECK_NULL_POINTER(pvcs_ops, VD_NOK);
    XM_CHECK_NULL_POINTER((pvcs_ops->Process), VD_NOK);

    return pvcs_ops->Process(inputImages, output, params, selectedBaseFrame, engine);
}

VDErrorType VDClearShot_Release(void **engine)
{
    XM_CHECK_NULL_POINTER(pvcs_ops, VD_NOK);
    XM_CHECK_NULL_POINTER((pvcs_ops->Release), VD_NOK);

    return pvcs_ops->Release(engine);
}

