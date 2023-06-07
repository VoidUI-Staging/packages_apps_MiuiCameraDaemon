#include <stdio.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <dlfcn.h>
#include "miautil.h"
#include "visidonclearshot.h"
#include "vddualcameraclearshotapi.h"
#include <vndksupport/linker.h>

#define ALIGN_TO_SIZE(size, align) ((size + align - 1) & ~(align - 1))
#define MIN(a, b) (((a)<(b))?(a):(b))
#define VD_PARAM_COUNT (10)
#define CS_MAX_INPUT_NUM (10)

#undef LOG_TAG
#define LOG_TAG "QVisidonClearshot"

namespace mialgo {
static VDDClearshotOps_t m_VCS_ops;

QVisidonClearshot::QVisidonClearshot(): m_VDlib_handle(NULL) {
    ALOGD("QVisidonClearshot::construct %s : %d", __func__, __LINE__);
}

QVisidonClearshot::~QVisidonClearshot() {
    // NOOP
}

int QVisidonClearshot::ParseAlogLib(void)
{
    #if defined(_LP64)
    static const char * libName = "/vendor/lib64/libVDDualCameraClearShot.so";
    #else
    static const char * libName = "/vendor/lib/libVDDualCameraClearShot.so";
    #endif
    m_VDlib_handle = NULL;

    memset(&m_VCS_ops, 0, sizeof(m_VCS_ops));

    void *handle = android_load_sphal_library(libName, RTLD_NOW);
    if (!handle) {
        ALOGE("Open %s Error :%s", libName, dlerror());
        return -1;
    }
    ALOGI("###Open %s with handle:%p", libName, handle);

    const char * (*GetVersion)(void) = NULL;
    *(void **)&GetVersion = dlsym(handle, "VDDualCameraClearShotVersion");
    XM_CHECK_NULL_POINTER_AND_RELEASE_RESOURCE(GetVersion, handle, -1);

    VDErrorCode (*Initialize)(VDDualCameraClearShotInitParameters_t, void **);
    *(void **)&Initialize = dlsym(handle, "VDDualCameraClearShotInitialize");
    XM_CHECK_NULL_POINTER_AND_RELEASE_RESOURCE(Initialize, handle, -1);

    VDErrorCode (*SetNRForBrightness)(int, float, void *);
    *(void **)&SetNRForBrightness = dlsym(handle, "VDDualCameraClearShotSetNRForBrightness");
    XM_CHECK_NULL_POINTER_AND_RELEASE_RESOURCE(SetNRForBrightness, handle, -1);

    int (*Process)(VDYUVMultiPlaneImage *, VDYUVMultiPlaneImage *, VDDualCameraClearShotParameters_t, int *, void *);
    *(void **)&Process = dlsym(handle, "VDDualCameraClearShotProcess");
    XM_CHECK_NULL_POINTER_AND_RELEASE_RESOURCE(Process, handle, -1);

    VDErrorCode (*Release)(void **);
    *(void **)&Release = dlsym(handle, "VDDualCameraClearShotRelease");
    XM_CHECK_NULL_POINTER_AND_RELEASE_RESOURCE(Release, handle, -1);

    m_VCS_ops.GetVersion = GetVersion;
    m_VCS_ops.Initialize = Initialize;
    m_VCS_ops.SetNRForBrightness = SetNRForBrightness;
    m_VCS_ops.Process = Process;
    m_VCS_ops.Release = Release;

    VDDClearShot_APIOps_Init(&m_VCS_ops);

    m_VDlib_handle = handle;

    return 0;
}

void QVisidonClearshot::init() {
    ALOGD("QVisidonClearshot::init start %s : %d", __func__, __LINE__);
    ParseAlogLib();
    ALOGD("QVisidonClearshot::init end %s : %d", __func__, __LINE__);
}

void QVisidonClearshot::deinit(void) {
    /* do dlclose() */
    if (m_VDlib_handle) {
        dlclose(m_VDlib_handle);
        m_VDlib_handle = NULL;
    }
    VDDClearShot_APIOps_DeInit();
}

void QVisidonClearshot::process(struct MiImageBuffer *input, struct MiImageBuffer *output,
                                int input_num, int iso, int *output_baseframe) {
    unsigned char *input_y[CS_MAX_INPUT_NUM], *input_uv[CS_MAX_INPUT_NUM];
    int32_t error = 0;
    void *engine = NULL;
    VDDualCameraClearShotInitParameters_t params;
    VDDualCameraClearShotParameters_t runtimeParameters[VD_PARAM_COUNT] = {{0}};;
    VDYUVMultiPlaneImage *inputs = NULL;
    VDYUVMultiPlaneImage outputImage;

    int targetParamIndex = 0;
    int effetiveFrames = 0;
    char name[PROPERTY_VALUE_MAX];
    int32_t dstBufferSize = 0;
    static int32_t tagIdx = 0;
    int h = 0;
    unsigned char *psrc, *pdst;

    int selectedBaseFrame = 0;

    float maxSharpnessBrightness = 0.0f;
    float brightness = 0.0f;
    float faceWeight = 1.0f;
    float sharpness = 0.0f;

    float originalNoiseReductionStrength = 0.0f;
    float newNoiseReductionStrength = 0.0f;
    float coefficient = 0.0f;
    float strengthAddition = 0.0f;

    /* Set brightness based luminance reduction table, */
    const float brightnessWeights[256] = {
            1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f,
            1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f,
            1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f,
            1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f,
            1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f,
            1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f,
            1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f,
            1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f, 1.000000f,
            1.000000f, 0.999931f, 0.999727f, 0.999388f, 0.998917f, 0.998316f, 0.997586f, 0.996730f,
            0.995749f, 0.994645f, 0.993421f, 0.992077f, 0.990616f, 0.989040f, 0.987351f, 0.985550f,
            0.983640f, 0.981622f, 0.979498f, 0.977270f, 0.974940f, 0.972510f, 0.969981f, 0.967356f,
            0.964637f, 0.961825f, 0.958922f, 0.955931f, 0.952852f, 0.949688f, 0.946442f, 0.943113f,
            0.939706f, 0.936221f, 0.932660f, 0.929025f, 0.925319f, 0.921542f, 0.917698f, 0.913787f,
            0.909812f, 0.905774f, 0.901676f, 0.897519f, 0.893305f, 0.889036f, 0.884714f, 0.880341f,
            0.875919f, 0.871450f, 0.866934f, 0.862376f, 0.857775f, 0.853135f, 0.848457f, 0.843742f,
            0.838994f, 0.834213f, 0.829401f, 0.824561f, 0.819695f, 0.814804f, 0.809889f, 0.804954f,
            0.800000f, 0.794836f, 0.789280f, 0.783342f, 0.777036f, 0.770374f, 0.763367f, 0.756028f,
            0.748369f, 0.740401f, 0.732138f, 0.723591f, 0.714772f, 0.705694f, 0.696368f, 0.686806f,
            0.677022f, 0.667027f, 0.656832f, 0.646451f, 0.635894f, 0.625176f, 0.614306f, 0.603299f,
            0.592165f, 0.580916f, 0.569566f, 0.558126f, 0.546608f, 0.535024f, 0.523387f, 0.511708f,
            0.500000f, 0.487568f, 0.473785f, 0.458767f, 0.442627f, 0.425480f, 0.407440f, 0.388622f,
            0.369141f, 0.349110f, 0.328644f, 0.307858f, 0.286865f, 0.265781f, 0.244720f, 0.223797f,
            0.203125f, 0.182819f, 0.162994f, 0.143764f, 0.125244f, 0.107548f, 0.090790f, 0.075085f,
            0.060547f, 0.047291f, 0.035431f, 0.025082f, 0.016357f, 0.009373f, 0.004242f, 0.001080f,
            0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f,
            0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f,
            0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f,
            0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f,
            0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f,
            0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f,
            0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f,
            0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f, 0.000000f
    };

    ALOGD("MIALGO_dualcameraclearshot:%s++,input_num=%d", __func__, input_num);
    memset(&params, 0, sizeof(VDDualCameraClearShotInitParameters_t));
    memset(&outputImage, 0, sizeof(VDYUVMultiPlaneImage));
    if (input_num > CS_MAX_INPUT_NUM)
        input_num = CS_MAX_INPUT_NUM;
    for (int i = 0; i < input_num; i++) {
        input_y[i] = input[i].Plane[0];
        input_uv[i] = input[i].Plane[1];
    }

    // Initialize clear shot engine
    params.width = input[0].Width;
    params.height = input[0].Height;
    params.yRowStride = ALIGN_TO_SIZE(params.width, 64);
    params.yColumnStride = ALIGN_TO_SIZE(params.height, 64);
    params.numberOfFrames = input_num;
    params.type = VDImageFormatYUV_NV12;
    params.numberOfCores = 8;

    for (int32_t m = 0; m < input_num; m++) {
        runtimeParameters[m].noiseReductionAdjustment[0] = 1;
        runtimeParameters[m].noiseReductionAdjustment[1] = 1;
        runtimeParameters[m].noiseReductionAdjustment[2] = 1;
        runtimeParameters[m].noiseReductionAdjustment[3] = 1;
        runtimeParameters[m].noiseReductionAdjustment[4] = 2;
        runtimeParameters[m].noiseReductionAdjustment[5] = 2;
    }
//0-198
    runtimeParameters[0].luminanceNoiseReduction = 0.01;
    runtimeParameters[0].chromaNoiseReduction = 0.8f;
    runtimeParameters[0].sharpen = 0.0f;
    runtimeParameters[0].brightening = 0;
    runtimeParameters[0].colorBoost = 0;
    runtimeParameters[0].movingObjectNoiseRemoval = 1;
    runtimeParameters[0].allocate = 0;
    runtimeParameters[0].inputSharpnessThreshold = 10;
    runtimeParameters[0].inputBrightnessThreshold = 0.02;
    runtimeParameters[0].effectiveFrames = 5;
//198-400
    runtimeParameters[1].luminanceNoiseReduction = 0.01;
    runtimeParameters[1].chromaNoiseReduction = 0.8f;
    runtimeParameters[1].sharpen = 0.0f;
    runtimeParameters[1].brightening = 0;
    runtimeParameters[1].colorBoost = 0;
    runtimeParameters[1].movingObjectNoiseRemoval = 2;
    runtimeParameters[1].allocate = 0;
    runtimeParameters[1].inputSharpnessThreshold = 15;
    runtimeParameters[1].inputBrightnessThreshold = 0.02;
    runtimeParameters[1].effectiveFrames = 5;
//400-800
    runtimeParameters[2].luminanceNoiseReduction = 0.01;
    runtimeParameters[2].chromaNoiseReduction = 1.2f;
    runtimeParameters[2].sharpen = 0.0f;
    runtimeParameters[2].brightening = 0;
    runtimeParameters[2].colorBoost = 0;
    runtimeParameters[2].movingObjectNoiseRemoval = 2;
    runtimeParameters[2].allocate = 0;
    runtimeParameters[2].inputSharpnessThreshold = 15;
    runtimeParameters[2].inputBrightnessThreshold = 0.02;
    runtimeParameters[2].effectiveFrames = 5;
//800-1200
    runtimeParameters[3].luminanceNoiseReduction = 0.4;
    runtimeParameters[3].chromaNoiseReduction = 1.5f;
    runtimeParameters[3].sharpen = 0.0f;
    runtimeParameters[3].brightening = 0;
    runtimeParameters[3].colorBoost = 0;
    runtimeParameters[3].movingObjectNoiseRemoval = 2;
    runtimeParameters[3].allocate = 0;
    runtimeParameters[3].inputSharpnessThreshold = 20;
    runtimeParameters[3].inputBrightnessThreshold = 0.02;
    runtimeParameters[3].effectiveFrames = 5;
//1200-1800
    runtimeParameters[4].luminanceNoiseReduction = 0.9;
    runtimeParameters[4].chromaNoiseReduction = 1.5f;
    runtimeParameters[4].sharpen = 0.0f;
    runtimeParameters[4].brightening = 0;
    runtimeParameters[4].colorBoost = 0;
    runtimeParameters[4].movingObjectNoiseRemoval = 2;
    runtimeParameters[4].allocate = 0;
    runtimeParameters[4].inputSharpnessThreshold = 20;
    runtimeParameters[4].inputBrightnessThreshold = 0.02;
    runtimeParameters[4].effectiveFrames = 5;
//1800-2400
    runtimeParameters[5].luminanceNoiseReduction = 1.2;
    runtimeParameters[5].chromaNoiseReduction = 1.8;
    runtimeParameters[5].sharpen = 0.0f;
    runtimeParameters[5].brightening = 0;
    runtimeParameters[5].colorBoost = 0;
    runtimeParameters[5].movingObjectNoiseRemoval = 2;
    runtimeParameters[5].allocate = 0;
    runtimeParameters[5].inputSharpnessThreshold = 20;
    runtimeParameters[5].inputBrightnessThreshold = 0.02;
    runtimeParameters[5].effectiveFrames = 5;
//2400-3200
    runtimeParameters[6].luminanceNoiseReduction = 1.6;
    runtimeParameters[6].chromaNoiseReduction = 2.0f;
    runtimeParameters[6].sharpen = 0.0f;
    runtimeParameters[6].brightening = 0;
    runtimeParameters[6].colorBoost = 0;
    runtimeParameters[6].movingObjectNoiseRemoval = 2;
    runtimeParameters[6].allocate = 0;
    runtimeParameters[6].inputSharpnessThreshold = 25;
    runtimeParameters[6].inputBrightnessThreshold = 0.02;
    runtimeParameters[6].effectiveFrames = 5;
//3200-4200
    runtimeParameters[7].luminanceNoiseReduction = 2.0;
    runtimeParameters[7].chromaNoiseReduction = 2.5f;
    runtimeParameters[7].sharpen = 0.0f;
    runtimeParameters[7].brightening = 0;
    runtimeParameters[7].colorBoost = 0;
    runtimeParameters[7].movingObjectNoiseRemoval = 2;
    runtimeParameters[7].allocate = 0;
    runtimeParameters[7].inputSharpnessThreshold = 25;
    runtimeParameters[7].inputBrightnessThreshold = 0.02;
    runtimeParameters[7].effectiveFrames = 5;
//4200-5500
    runtimeParameters[8].luminanceNoiseReduction = 2.5;
    runtimeParameters[8].chromaNoiseReduction = 2.5f;
    runtimeParameters[8].sharpen = 0.0f;
    runtimeParameters[8].brightening = 0;
    runtimeParameters[8].colorBoost = 0;
    runtimeParameters[8].movingObjectNoiseRemoval = 2;
    runtimeParameters[8].allocate = 0;
    runtimeParameters[8].inputSharpnessThreshold = 25;
    runtimeParameters[8].inputBrightnessThreshold = 0.02;
    runtimeParameters[8].effectiveFrames = 5;
//5500-8000
    runtimeParameters[9].luminanceNoiseReduction = 2.5;
    runtimeParameters[9].chromaNoiseReduction = 3.0f;
    runtimeParameters[9].sharpen = 0.0f;
    runtimeParameters[9].brightening = 0;
    runtimeParameters[9].colorBoost = 0;
    runtimeParameters[9].movingObjectNoiseRemoval = 2;
    runtimeParameters[9].allocate = 0;
    runtimeParameters[9].inputSharpnessThreshold = 28;
    runtimeParameters[9].inputBrightnessThreshold = 0.02;
    runtimeParameters[9].effectiveFrames = 5;

    runtimeParameters[0].baseFrame = -1;
    runtimeParameters[1].baseFrame = -1;
    runtimeParameters[2].baseFrame = -1;
    runtimeParameters[3].baseFrame = -1;
    runtimeParameters[4].baseFrame = -1;
    runtimeParameters[5].baseFrame = -1;
    runtimeParameters[6].baseFrame = -1;
    runtimeParameters[7].baseFrame = -1;
    runtimeParameters[8].baseFrame = -1;
    runtimeParameters[9].baseFrame = -1;


    if (0 <= iso && iso <= 198) {
        targetParamIndex = 0;
    } else if (198 < iso && iso <= 400) {
        targetParamIndex = 1;
    } else if (400 < iso && iso <= 800) {
        targetParamIndex = 2;
    } else if (800 < iso && iso <= 1200) {
        targetParamIndex = 3;
    } else if (1200 < iso && iso <= 1800) {
        targetParamIndex = 4;
    } else if (1800 < iso && iso <= 2400) {
        targetParamIndex = 5;
    } else if (2400 < iso && iso <= 3200) {
        targetParamIndex = 6;
    } else if (3200 < iso && iso <= 4200) {
        targetParamIndex = 7;
    } else if (4200 < iso && iso <= 5500) {
        targetParamIndex = 8;
    } else if (5500 < iso) {
        targetParamIndex = 9;
    }

    ALOGE("iso:%d,targetParamIndex: %d, luminanceNoiseReduction:%f,"
            "inputBrightnessThreshold:%f,"
            "sharpen:%f, effectiveFrames: %d"
            "chromaNoiseReduction:%f\n",
        iso, targetParamIndex, runtimeParameters[targetParamIndex].luminanceNoiseReduction,
        runtimeParameters[targetParamIndex].inputBrightnessThreshold,
        runtimeParameters[targetParamIndex].sharpen,
        runtimeParameters[targetParamIndex].effectiveFrames,
        runtimeParameters[targetParamIndex].chromaNoiseReduction);

    ALOGE("MIALGO_dualcameraclearshot:Version %s", VDDualCameraClearShotVersion());

    // Initialize engine
    error = VDDualCameraClearShotInitialize(params, &engine);
    if (error || !engine) {
        ALOGE("MIALGO_dualcameraclearshot:%s: VDClearShot_Initialize failed[%d]", __FUNCTION__, error);
        goto VD_EXIT;
    }
    inputs = (VDYUVMultiPlaneImage *) malloc(params.numberOfFrames * sizeof(VDYUVMultiPlaneImage));
    if (!inputs) {
        ALOGE("MIALGO_dualcameraclearshot:%s: Memory allocation failed.", __FUNCTION__);
        error = -1;
        goto VD_EXIT;
    }

    memset(inputs, 0x0, params.numberOfFrames * sizeof(VDYUVMultiPlaneImage));
    for (int32_t k = 0; k < params.numberOfFrames; k++) {
        inputs[k].width = params.width;
        inputs[k].height = params.height;
        inputs[k].yRowStride = params.yRowStride;
        inputs[k].yColumnStride = params.yColumnStride;
        inputs[k].type = VDImageFormatYUV_NV12;//VD_YUV_NV21;
        inputs[k].yPixelStride = 1; // y pixel stide (means how many bytes between two adjacent pixels).
        inputs[k].cbPixelStride = 2; // cb pixel stide (means how many bytes between two adjacent pixels)
        inputs[k].crPixelStride = 2; // cb pixel stide (means how many bytes between two adjacent pixels)
        inputs[k].cbRowStride = params.yRowStride; // length of cb row (may be longer than width) (typically same as with yRowStride)
        inputs[k].crRowStride = params.yRowStride; // length of cr row (may be longer than width) (typically same as with yRowStride)
        inputs[k].yPtr = input_y[k]; // Pointer to y channel
        inputs[k].cbPtr = input_uv[k]; // pointer to CB channel
        inputs[k].crPtr = inputs[k].cbPtr + 1; // pointer to CR channel
    }
    // Define output image
    outputImage.width = params.width;
    outputImage.height = params.height;
    outputImage.yRowStride = params.yRowStride;
    outputImage.yColumnStride = params.yColumnStride;
    outputImage.type = VDImageFormatYUV_NV12;//VD_YUV_NV21;
    outputImage.yPixelStride = 1;
    outputImage.cbPixelStride = 2;
    outputImage.crPixelStride = 1;
    outputImage.cbRowStride = params.yRowStride;
    outputImage.crRowStride = params.yRowStride;
    outputImage.yPtr = (unsigned char*)output->Plane[0]; // Pointer to Y channel
    outputImage.cbPtr = (unsigned char*)output->Plane[1]; // pointer to cb channel
    outputImage.crPtr = outputImage.cbPtr + 1; // pointer to cr channel

    for (int i = 0; i < 64; i++) {
        originalNoiseReductionStrength = runtimeParameters[targetParamIndex].luminanceNoiseReduction;
        coefficient = MIN(runtimeParameters[targetParamIndex].luminanceNoiseReduction, 2.0f);
        strengthAddition = coefficient * brightnessWeights[i * 4];
        newNoiseReductionStrength = originalNoiseReductionStrength + strengthAddition;
        if (newNoiseReductionStrength > 10.0f)
            newNoiseReductionStrength = 10.0f;
        VDDualCameraClearShotSetNRForBrightness(i, newNoiseReductionStrength, engine);
    }

    if (runtimeParameters[targetParamIndex].effectiveFrames > input_num) {
        ALOGW("WARNING! %s, clip effectiveFrames from %d to maximum %d",
            __func__, runtimeParameters[targetParamIndex].effectiveFrames, input_num);
            runtimeParameters[targetParamIndex].effectiveFrames = input_num;
    }

    // Apply processing
    effetiveFrames = VDDualCameraClearShotProcess(inputs, &outputImage, runtimeParameters[targetParamIndex],
                                         &selectedBaseFrame, engine);

    ALOGD("MIALGO_dualcameraclearshot:Selected base frame was %d", selectedBaseFrame);
    *output_baseframe = selectedBaseFrame;

    if (effetiveFrames <= 0) {
        ALOGE("MIALGO_dualcameraclearshot:%s: VDDualCameraClearShotProcess failed", __FUNCTION__);
        goto VD_EXIT;
    }

    VD_EXIT:
    if (inputs) {
        free(inputs);
        inputs = NULL;
    }

    if (engine) {
        VDDualCameraClearShotRelease(&engine);
        engine = NULL;
    }
    ALOGD("MIALGO_dualcameraclearshot:%s--", __func__);
}
}
