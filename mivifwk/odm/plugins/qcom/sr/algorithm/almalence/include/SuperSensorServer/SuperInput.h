#ifndef __SUPERINPUT_H__
#define __SUPERINPUT_H__

#include <SuperSensorServer/SuperParcel.h>
#include <SuperSensorServer/Yuv888.h>

namespace almalence {

/**
 * Struct holding SuperSensor pipeline input parameters.
 * Received by the first processing stage.
 */
struct SuperInput : public SuperParcel
{
    int sensitivity;
    bool scaled;
    bool processingEnabled;
    float droGamma;
    bool digitalIS;
    float zoom;

    float gammaCurveOffset;
    float gammaCurveStrength;
    float blackLevelAttenuation;

    Size size;
    Yuv888 image;

    alma_config_t config[MAX_CONFIG_SIZE];
};

} // namespace almalence

#endif // __SUPERINPUT_H__
