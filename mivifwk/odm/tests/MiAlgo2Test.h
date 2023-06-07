#ifndef __MIALGO2TEST_H__
#define __MIALGO2TEST_H__
#include <MiaPluginUtils.h>
#include <MiaPostProcIntf.h>
#include <dlfcn.h>

#include "MiAlgo2Callback.h"
#include "MiBuf.h"

using namespace mialgo2;

static void *gHandle;
const char fileNameIn[128] = {"/data/misc/media/input_image0_6016x4512.yuv"};
const char fileNameOut[128] = {"/data/misc/media/output_image_mialt_6016x4512.yuv"};

typedef enum {
    SnapshotType = 0,
    PreviewType = 1,
    VideType = 2,
    MaxType = 3,
} MiFrameType;

typedef void *(*funcCameraPostProcCreate)(PostProcCreateParams *params);

typedef CDKResult (*funcCameraPostProcProcess)(void *postProcInstance,
                                               PostProcSessionParams *sessionParams);

typedef void (*funcCameraPostProcDestroy)(void *postProcInstance);

funcCameraPostProcCreate pCameraPostProcCreate = NULL;
funcCameraPostProcProcess pCameraPostProcProcess = NULL;
funcCameraPostProcDestroy pCameraPostProcDestroy = NULL;

void funcInit()
{
    const char *libname = "libmialgoengine.so";
    gHandle = dlopen(libname, RTLD_NOW | RTLD_LOCAL);
    if (gHandle == NULL) {
        char const *err_str = dlerror();
        MLOGE(Mia2LogGroupCore, "load: module=%s%s", libname, err_str ? err_str : "unknown");
    } else {
        pCameraPostProcCreate = (funcCameraPostProcCreate)dlsym(gHandle, "CameraPostProc_Create");
        pCameraPostProcProcess =
            (funcCameraPostProcProcess)dlsym(gHandle, "CameraPostProc_Process");
        pCameraPostProcDestroy =
            (funcCameraPostProcDestroy)dlsym(gHandle, "CameraPostProc_Destroy");
    }
}

#endif /*__MIALGO2TEST_H__*/
