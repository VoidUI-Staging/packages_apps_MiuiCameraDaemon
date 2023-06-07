#include <stdio.h>
#include <utils/Log.h>
#include <cutils/properties.h>
#include <dlfcn.h>
#include "miautil.h"
#include "visidonclearshot_s.h"
#include "vdclearshotapi.h"
#include "xmlutil.h"
#include <vndksupport/linker.h>

#define ALIGN_TO_SIZE(size, align) ((size + align - 1) & ~(align - 1))
#define MIN(a, b) (((a)<(b))?(a):(b))

#undef LOG_TAG
#define LOG_TAG "QVisidonClearshot_S"

#define CLEAR_SHOT_PARAMS_FILEPATH "/vendor/etc/camera/visidon_clearshot_params_algoup.xml"

namespace mialgo {

static VDClearshotOps_t m_VCS_ops;

QVisidonClearshot_S::QVisidonClearshot_S(): m_VDlib_handle(NULL) {
    ALOGD("QVisidonClearshot_S::construct %s : %d", __func__, __LINE__);
}

QVisidonClearshot_S::~QVisidonClearshot_S() {
    //NOOP
}

void QVisidonClearshot_S::get_int_array(char *string, int *array)
{
    sscanf(string, "%d,%d,%d,%d,%d,%d", &array[0], &array[1], &array[2],
        &array[3], &array[4], &array[5]);
}

int QVisidonClearshot_S::ParseAlogLib(void)
{
    #if defined(_LP64)
    static const char * libName = "/vendor/lib64/libVDClearShot.so";
    #else
    static const char * libName = "/vendor/lib/libVDClearShot.so";
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
    *(void **)&GetVersion = dlsym(handle, "VDClearShot_GetVersion");
    XM_CHECK_NULL_POINTER_AND_RELEASE_RESOURCE(GetVersion, handle, -1);

    VDErrorType (*Initialize)(VDClearShotInitParameters, int, int, void **);
    *(void **)&Initialize = dlsym(handle, "VDClearShot_Initialize");
    XM_CHECK_NULL_POINTER_AND_RELEASE_RESOURCE(Initialize, handle, -1);

    int (*SetNRForBrightness)(int, float, void *);
    *(void **)&SetNRForBrightness = dlsym(handle, "VDClearShot_SetNRForBrightness");
    XM_CHECK_NULL_POINTER_AND_RELEASE_RESOURCE(SetNRForBrightness, handle, -1);

    int (*Process)(VDYUVMultiPlaneImage *, VDYUVMultiPlaneImage *, VDClearShotRuntimeParams, int *, void *);
    *(void **)&Process = dlsym(handle, "VDClearShot_Process");
    XM_CHECK_NULL_POINTER_AND_RELEASE_RESOURCE(Process, handle, -1);

    VDErrorType (*Release)(void **);
    *(void **)&Release = dlsym(handle, "VDClearShot_Release");
    XM_CHECK_NULL_POINTER_AND_RELEASE_RESOURCE(Release, handle, -1);

    m_VCS_ops.GetVersion = GetVersion;
    m_VCS_ops.Initialize = Initialize;
    m_VCS_ops.SetNRForBrightness = SetNRForBrightness;
    m_VCS_ops.Process = Process;
    m_VCS_ops.Release = Release;

    VDClearShot_APIOps_Init(&m_VCS_ops);

    m_VDlib_handle = handle;

    return 0;
}


void QVisidonClearshot_S::parser_parameter(const char *file_name, struct VDClearShotRuntimeParamsXml *params, const char *device)
{
    xmlNodePtr root = NULL;
    xmlNodePtr cur_a = NULL;
    xmlDocPtr doc = xml_parser(file_name);
    while(xml_get_node(doc, &root, &cur_a, 1))
    {
        xmlNodePtr cur_b = NULL;
        while(xml_get_node(doc, &cur_a, &cur_b, 0))
        {
            char *prop;
            xmlNodePtr cur_c = NULL;
            xml_get_prop(cur_b, "name", &prop);
            if (strcmp(device, prop))
                continue;

            while(xml_get_node(doc, &cur_b, &cur_c, 0))
            {
                int num = 0;
                xmlNodePtr cur_d = NULL;
                xml_get_prop(cur_c, "id", &prop);
                struct VDClearShotRuntimeParamsXml *param = params + atoi(prop) * VD_PARAM_COUNT;
                while(xml_get_node(doc, &cur_c, &cur_d, 0))
                {
                    xmlNodePtr cur_e = NULL;
                    char *key, *value;
                    xml_get_prop(cur_d, "value", &prop);
                    param->isoIndex = atoi(prop);
                    while(xml_get_node(doc, &cur_d, &cur_e, 0))
                    {
                        xml_get_item(doc, cur_e, &key, &value);
                        if (!strcmp(key, "luminance_noise_reduction"))
                        {
                            param->luminanceNoiseReduction = atof(value);
                        }
                        else if (!strcmp(key, "chroma_noise_reduction"))
                        {
                            param->chromaNoiseReduction = atof(value);
                        }
                        else if (!strcmp(key, "sharpen"))
                        {
                            param->sharpen = atof(value);
                        }
                        else if (!strcmp(key, "brightening"))
                        {
                            param->brightening = atoi(value);
                        }
                        else if (!strcmp(key, "color_boost"))
                        {
                            param->colorBoost = atoi(value);
                        }
                        else if (!strcmp(key, "moving_object_noise_removal"))
                        {
                            param->movingObjectNoiseRemoval = atoi(value);
                        }
                        else if (!strcmp(key, "input_sharpness_threshold"))
                        {
                            param->inputSharpnessThreshold = atoi(value);
                        }
                        else if (!strcmp(key, "allocate"))
                        {
                            param->allocate = atoi(value);
                        }
                        else if (!strcmp(key, "noise_reduction_adjustment"))
                        {
                            get_int_array(value, param->noiseReductionAdjustment);
                        }
                        else if (!strcmp(key, "input_brightness_threshold"))
                        {
                            param->inputBrightnessThreshold = atof(value);
                        }
                        else if (!strcmp(key, "baseFrame"))
                        {
                            param->baseFrame = atoi(value);
                        }
                        else if (!strcmp(key, "effectiveFrames"))
                        {
                            param->effectiveFrames = atoi(value);
                        }
                        else if (!strcmp(key, "useTripodMode"))
                        {
                            param->useTripodMode = atoi(value);
                        }
                        else if (!strcmp(key, "moving_pixels_rejection_threshold"))
                        {
                            param->movingPixelsRejectionThreshold = atof(value);
                        }
                        else if (!strcmp(key, "deghosting_strength"))
                        {
                            param->deghostingStrength = atoi(value);
                        }
                    }
                    num++;
                    param++;
                }
                for (int i = num; i > 0; i--)
                {
                    param--;
                    param->num = num;
                }
            }
        }
    }
    xml_free(doc);
}

void QVisidonClearshot_S::set_parameter(VDClearShotRuntimeParams *param, struct VDClearShotRuntimeParamsXml *xml, int iso)
{
    int i, num;
    for (i = 0, num = xml->num; i < num; i++)
    {
        if (iso <= xml->isoIndex)
            break;
        xml++;
    }
    if (i >= num)
        xml--;

    param->luminanceNoiseReduction        = xml->luminanceNoiseReduction;
    param->chromaNoiseReduction           = xml->chromaNoiseReduction;
    param->sharpen                        = xml->sharpen;
    param->brightening                    = xml->brightening;
    param->colorBoost                     = xml->colorBoost;
    param->movingObjectNoiseRemoval       = xml->movingObjectNoiseRemoval;
    param->inputSharpnessThreshold        = xml->inputSharpnessThreshold;
    param->allocate                       = xml->allocate;
    param->inputBrightnessThreshold       = xml->inputBrightnessThreshold;
    param->baseFrame                      = xml->baseFrame;
    param->effectiveFrames                = xml->effectiveFrames;
    param->useTripodMode                  = xml->useTripodMode;
    param->movingPixelsRejectionThreshold = xml->movingPixelsRejectionThreshold;
    param->deghostingStrength             = xml->deghostingStrength;
    memcpy(param->noiseReductionAdjustment, xml->noiseReductionAdjustment, sizeof(param->noiseReductionAdjustment));
}

void QVisidonClearshot_S::init() {
    ALOGD("huangxin QVisidonClearshot_S::init start %s : %d", __func__, __LINE__);
    ParseAlogLib();
    char device[PROPERTY_VALUE_MAX];
    memset(device, 0, sizeof(device));
    property_get("ro.product.device", device, "0");
    memset(mClearShotRuntimeParams, 0, sizeof(mClearShotRuntimeParams));

    parser_parameter(CLEAR_SHOT_PARAMS_FILEPATH, (struct VDClearShotRuntimeParamsXml *)mClearShotRuntimeParams, device);
    ALOGD("QVisidonClearshot_S::init end %s : %d device %s", __func__, __LINE__, device);
}

void QVisidonClearshot_S::deinit(void) {
    /* do dlclose() */
    if (m_VDlib_handle) {
        dlclose(m_VDlib_handle);
        m_VDlib_handle = NULL;
    }
    VDClearShot_APIOps_DeInit();
}

void QVisidonClearshot_S::process(struct MiImageBuffer *input, struct MiImageBuffer *output, int input_num, int iso,
        int crop_boundaries, int *output_baseframe)
{
    unsigned char *input_y[CS_MAX_INPUT_NUM], *input_uv[CS_MAX_INPUT_NUM];
    int32_t error = 0;
    void * engine = NULL;
    VDClearShotInitParameters params;
    VDClearShotRuntimeParams runtimeParameters;
    VDYUVMultiPlaneImage *inputs = NULL;
    VDYUVMultiPlaneImage outputImage;
    int cpuMask = 0;
    int targetParamIndex = 0;
    int effetiveFrames = 0;
    char name[PROPERTY_VALUE_MAX];
    int32_t dstBufferSize = 0;
    static int32_t tagIdx = 0;
    unsigned char *psrc = NULL, *pdst = NULL;
    int inputStride = 0, inputScanline = 0;
    int outputStride = 0, outputScanline = 0;

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
    const float brightnessWeights[256] = {1.000000f,1.000000f,1.000000f,1.000000f,1.000000f,1.000000f,1.000000f,1.000000f,1.000000f,1.000000f,1.000000f,
	1.000000f,1.000000f,1.000000f,1.000000f,1.000000f,1.000000f,1.000000f,1.000000f,1.000000f,0.999931f,0.999727f,0.999388f,0.998917f,0.998316f,0.997586f,
	0.996730f,0.995749f,0.994645f,0.993421f,0.992077f,0.990616f,0.989040f,0.987351f,0.985550f,0.983640f,0.981622f,0.979498f,0.977270f,0.974940f,0.972510f,
	0.969981f,0.967356f,0.964637f,0.961825f,0.958922f,0.955931f,0.952852f,0.949688f,0.946442f,0.943113f,0.939706f,0.936221f,0.932660f,0.929025f,0.925319f,
	0.921542f,0.917698f,0.913787f,0.909812f,0.905774f,0.901676f,0.897519f,0.893305f,0.889036f,0.884714f,0.880341f,0.875919f,0.871450f,0.866934f,0.862376f,
	0.857775f,0.853135f,0.848457f,0.843742f,0.838994f,0.834213f,0.829401f,0.824561f,0.819695f,0.814804f,0.809889f,0.804954f,0.800000f,0.794836f,0.789280f,
	0.783342f,0.777036f,0.770374f,0.763367f,0.756028f,0.748369f,0.740401f,0.732138f,0.723591f,0.714772f,0.705694f,0.696368f,0.686806f,0.677022f,0.667027f,
	0.656832f,0.646451f,0.635894f,0.625176f,0.614306f,0.603299f,0.592165f,0.580916f,0.569566f,0.558126f,0.546608f,0.535024f,0.523387f,0.511708f,0.500000f,
	0.487568f,0.473785f,0.458767f,0.442627f,0.425480f,0.407440f,0.388622f,0.369141f,0.349110f,0.328644f,0.307858f,0.286865f,0.265781f,0.244720f,0.223797f,
	0.203125f,0.182819f,0.162994f,0.143764f,0.125244f,0.107548f,0.090790f,0.075085f,0.060547f,0.047291f,0.035431f,0.025082f,0.016357f,0.009373f,0.004242f,
	0.001080f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,
	0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,
	0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,
	0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,
	0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,
	0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,
	0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,0.000000f,
	0.000000f,0.000000f,0.000000f,0.000000f,0.000000f};

    ALOGD("MIALGO_clearshot:%s++,input_num=%d", __func__, input_num);
    memset(&params, 0, sizeof(VDClearShotInitParameters));
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
    params.type = VD_YUV_NV12;//VD_YUV_NV21;

    runtimeParameters.baseFrame = -1;
    runtimeParameters.useTripodMode = 0;
    runtimeParameters.movingPixelsRejectionThreshold = 0.15f;
    runtimeParameters.deghostingStrength = 60; // default deghost strength

    //Clearshot only for Front Camera, so it is mClearShotRuntimeParams[1]
    //Should use param sensonIndex to distinguish mClearShotRuntimeParams[sensonIndex]
    set_parameter(&runtimeParameters, mClearShotRuntimeParams[1], iso);

    ALOGD("MIALGO_clearshot :iso:%d"
        ", luminanceNoiseReduction:%f"
        ", inputBrightnessThreshold:%f"
        ", sharpen:%f"
        ", chromaNoiseReduction:%f"
        ", noiseReductionAdjustment[0]:%d"
        ", effectiveFrames:%d"
        , iso
        , runtimeParameters.luminanceNoiseReduction
        , runtimeParameters.inputBrightnessThreshold
        , runtimeParameters.sharpen
        , runtimeParameters.chromaNoiseReduction
        , runtimeParameters.noiseReductionAdjustment[0]
        , runtimeParameters.effectiveFrames);
    ALOGE("MIALGO_clearshot:Version %s", VDClearShot_GetVersion());

    // Use CPU cores 4,5,6, and 7
    cpuMask = 128 + 64 + 32 + 16; // for example with MSM8976+ it is faster to use cores 4-7
    cpuMask = 255; // this uses cores 0-8
    // Initialize engine
    error = VDClearShot_Initialize(params, 8, cpuMask, &engine);
    if (error || !engine) {
        ALOGE("MIALGO_clearshot:%s: VDClearShot_Initialize failed[%d]", __FUNCTION__, error);
        goto VD_EXIT;
    }
    inputs = (VDYUVMultiPlaneImage *)malloc(params.numberOfFrames * sizeof(VDYUVMultiPlaneImage));
    if (!inputs) {
        ALOGE("MIALGO_clearshot:%s: Memory allocation failed.", __FUNCTION__);
        error = -1;
        goto VD_EXIT;
    }
	
    for (int32_t k = 0; k < params.numberOfFrames; k++) {
        inputs[k].width = params.width;
        inputs[k].height= params.height;
        inputs[k].yRowStride = params.yRowStride;
        inputs[k].yColumnStride = params.yColumnStride;
        inputs[k].type = VD_YUV_NV12;//VD_YUV_NV21;
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
    outputImage.type = VD_YUV_NV12;//VD_YUV_NV21;
    outputImage.yPixelStride = 1;
    outputImage.cbPixelStride = 2;
    outputImage.crPixelStride = 1;
    outputImage.cbRowStride = params.yRowStride;
    outputImage.crRowStride = params.yRowStride;
    outputImage.yPtr = (unsigned char*)output->Plane[0]; // Pointer to Y channel
    outputImage.cbPtr = (unsigned char*)output->Plane[1]; // pointer to cb channel
    outputImage.crPtr = outputImage.cbPtr + 1; // pointer to cr channel

    for (int i = 0; i < 64; i++) {
        originalNoiseReductionStrength = runtimeParameters.luminanceNoiseReduction;
        coefficient = MIN(runtimeParameters.luminanceNoiseReduction, 2.0f);
        strengthAddition = coefficient * brightnessWeights[i * 4];
        newNoiseReductionStrength = originalNoiseReductionStrength + strengthAddition;
        if(newNoiseReductionStrength > 10.0f)
            newNoiseReductionStrength = 10.0f;
        VDClearShot_SetNRForBrightness(i, newNoiseReductionStrength, engine);
    }

    // Apply processing
    effetiveFrames = VDClearShot_Process(inputs, &outputImage, runtimeParameters, &selectedBaseFrame, engine);
    ALOGD("MIALGO_clearshot:Selected base frame was %d", selectedBaseFrame);
    *output_baseframe = selectedBaseFrame;

    if (effetiveFrames <= 0) {
        ALOGE("MIALGO_clearshot:%s: VDClearShot_Process failed", __FUNCTION__);
        goto VD_EXIT;
    }

    VD_EXIT:
    if (inputs) {
        free(inputs);
        inputs = NULL;
    }

    if (engine) {
        VDClearShot_Release(&engine);
        engine = NULL;
    }
    ALOGD("MIALGO_clearshot:%s--", __func__);
}
}
