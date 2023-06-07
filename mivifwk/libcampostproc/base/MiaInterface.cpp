/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiaInterface.h"

#include <pthread.h>
#include <utils/Condition.h>
#include <utils/List.h>
#include <utils/Mutex.h>

#include "LogUtil.h"
#include "MiPostProcServiceIntf.h"

namespace mialgo {

uint32_t gMiaLogLevel = 0;

static const int32_t gMiaPreProcess =
    property_get_int32("persist.vendor.camera.miaintf.preprocess", 1);

static void miDynamicLogSwitch()
{
    char cam_stitch[PROPERTY_VALUE_MAX];
    property_get("persist.vendor.camera.miaintf.loglevel", cam_stitch, "3");
    gMiaLogLevel = atoi(cam_stitch);
}

CDKResult miaInit(void *params)
{
    MLOGD("E params %p", params);
    sp<MiPostProcServiceIntf> service = MiPostProcServiceIntf::getInstance();
    if (service.get() == NULL) {
        MLOGE("miaInit failed in geting service instance");
        return MIAS_NO_MEM;
    }

    miDynamicLogSwitch();
    return service->init();
}

CDKResult miaDeinit()
{
    MLOGD("E");
    MiPostProcServiceIntf::getInstance()->deinit();
    return MIAS_OK;
}

CDKResult miaSetMiviInfo(std::string info)
{
    MLOGD("E");
    MiPostProcServiceIntf::getInstance()->setMiviInfo(info);
    return MIAS_OK;
}

SessionHandle miaCreateSession(GraphDescriptor *gd, SessionOutput *output, MiaSessionCb *cb)
{
    if (gd == NULL || cb == NULL || !output) {
        MLOGE("miaCreateSession failed for invalid_argument %p output:%p", gd, output);
        return NULL;
    }
    MLOGI("E GraphDescriptor:%p output type:%u cb:%p", gd, output->type, cb);
    return MiPostProcServiceIntf::getInstance()->createSession(gd, output, cb);
}

CDKResult miaDestroySession(SessionHandle session)
{
    MLOGI("E session:%p", session);
    return MiPostProcServiceIntf::getInstance()->destroySession(session);
}

CDKResult miaProcessFrame(SessionHandle session, MiaFrame *frame)
{
    MLOGD("E session:%p frame: %p", session, frame);
    return MiPostProcServiceIntf::getInstance()->processFrame(session, frame);
}
CDKResult miaPreProcess(SessionHandle session, PreProcessInfo *params)
{
    MLOGD("E session:%p camera: %d, size: %dx%d, format: %d", session, params->cameraId,
          params->width, params->height, params->format);
    int preProcess;
    preProcess = gMiaPreProcess;
    if (preProcess) {
        return MiPostProcServiceIntf::getInstance()->preProcessFrame(session, params);
    } else {
        return MIAS_OK;
    }
}

CDKResult miaProcessFrameWithSync(SessionHandle session, ProcessRequestInfo *pProcessRequestInfo)
{
    MLOGD("E session:%p request: %p", session, pProcessRequestInfo);
    return MiPostProcServiceIntf::getInstance()->processFrameWithSync(session, pProcessRequestInfo);
}

CDKResult miaFlush(SessionHandle session)
{
    MLOGD("E session:%p", session);
    return MiPostProcServiceIntf::getInstance()->flush(session);
}

CDKResult miaQuickFinish(SessionHandle session, int64_t timeStamp, bool needImageResult)
{
    MLOGD("E miaQuickFinish session:%p", session);
    return MiPostProcServiceIntf::getInstance()->quickFinish(session, timeStamp, needImageResult);
}

// for dlsys
MiaInterface MIA_INTERFACES_SYS = {
    miaInit,           miaDeinit,       miaSetMiviInfo,          miaCreateSession,
    miaDestroySession, miaProcessFrame, miaProcessFrameWithSync, miaFlush,
    miaQuickFinish,
};

void *miOpenLib(void)
{
    return &MIA_INTERFACES_SYS;
}
// for dlsys
MiaExtInterface MIA_EXTINTERFACES_SYS = {
    miaPreProcess,
};

void *GetExtInterafce(void)
{
    return &MIA_EXTINTERFACES_SYS;
}

int GetExtVersion()
{
    return 2022042120; // mivi version
}

} // namespace mialgo
