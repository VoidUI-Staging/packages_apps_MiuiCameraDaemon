#ifndef _H_VISIDONCLEARSHOT_S_H_
#define _H_VISIDONCLEARSHOT_S_H_

#include "vdclearshotapi.h"

#define VD_PARAM_COUNT 11
#define CS_MAX_INPUT_NUM 12
#define MAX_CAMERA_ID 4

namespace mialgo {

struct VDClearShotRuntimeParamsXml
{
    float luminanceNoiseReduction;
    float chromaNoiseReduction;
    float sharpen;
    int   brightening;
    int   colorBoost;
    int   movingObjectNoiseRemoval;
    int   inputSharpnessThreshold;
    int   allocate;
    int   noiseReductionAdjustment[6];
    float inputBrightnessThreshold;
    int   baseFrame;
    int   effectiveFrames;
    int   useTripodMode;
    float movingPixelsRejectionThreshold;
    int   deghostingStrength;
    int   isoIndex;
    int   num;
};

class QVisidonClearshot_S {
public:
    QVisidonClearshot_S();
    virtual ~ QVisidonClearshot_S();
    void init();
    void deinit(void);
    void process(struct MiImageBuffer *input, struct MiImageBuffer *output, int input_num, int iso, int crop_boundaries, int *output_baseframe);

private:
    int ParseAlogLib(void);
    void get_int_array(char *string, int *array);
    void parser_parameter(const char *file_name, struct VDClearShotRuntimeParamsXml *params, const char *device);
    void set_parameter(VDClearShotRuntimeParams *param, struct VDClearShotRuntimeParamsXml *xml, int iso);

    void *m_VDlib_handle;
    struct VDClearShotRuntimeParamsXml mClearShotRuntimeParams[MAX_CAMERA_ID][VD_PARAM_COUNT];
};

}
#endif
