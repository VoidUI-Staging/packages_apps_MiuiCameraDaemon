/**
 * Bokeh Dynamic Blur Control
 **/

#ifndef DYNAMIC_BLUR_CONTROL_H
#define DYNAMIC_BLUR_CONTROL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utils/Log.h>

#include "arcsoft_debuglog.h"

#define DEFAULT_REALTIME_BLUR_LEVEL 85
#define DEFAULT_CAPTURE_BLUR_LEVEL  35

typedef struct
{
    float f_number;
    int32_t blur_level;
} DynamicBlurLevel;

static DynamicBlurLevel DblRealtimeBokeh[] = {
    {1.0f, 100}, {1.1f, 96},  {1.2f, 93}, {1.4f, 90}, {1.6f, 87}, {1.8f, 84}, {2.0f, 80},
    {2.2f, 77},  {2.5f, 74},  {2.8f, 70}, {3.2f, 65}, {3.5f, 60}, {4.0f, 55}, {4.5f, 40},
    {5.0f, 35},  {5.6f, 40},  {6.3f, 37}, {7.1f, 34}, {8.0f, 30}, {9.0f, 27}, {10.0f, 24},
    {11.0f, 20}, {13.0f, 10}, {14.0f, 5}, {16.0f, 0},
};

static DynamicBlurLevel DblCaptureBokeh[] = {
    {1.0f, 100}, {1.1f, 90}, {1.2f, 80}, {1.4f, 60}, {1.6f, 57}, {1.8f, 54}, {2.0f, 50},
    {2.2f, 47},  {2.5f, 44}, {2.8f, 40}, {3.2f, 35}, {3.5f, 30}, {4.0f, 25}, {4.5f, 20},
    {5.0f, 15},  {5.6f, 10}, {6.3f, 9},  {7.1f, 8},  {8.0f, 7},  {9.0f, 6},  {10.0f, 5},
    {11.0f, 4},  {13.0f, 3}, {14.0f, 2}, {16.0f, 0},
};

static int32_t findBlurLevelByFNumber(float fnum, bool isPreview)
{
    int32_t blur = 0;
    uint32_t size = 0;
    bool isFound = false;
    if (true == isPreview) {
        isFound = false;
        size = sizeof(DblRealtimeBokeh) / sizeof(DynamicBlurLevel);
        for (uint32_t i = 0; i < size; i++) {
            if (fnum == DblRealtimeBokeh[i].f_number) {
                blur = DblRealtimeBokeh[i].blur_level;
                isFound = true;
                break;
            }
        }
        if (0 == blur && false == isFound) {
            blur = DEFAULT_REALTIME_BLUR_LEVEL;
        }
    } else {
        isFound = false;
        size = sizeof(DblCaptureBokeh) / sizeof(DynamicBlurLevel);
        for (uint32_t i = 0; i < (sizeof(DblCaptureBokeh) / sizeof(DynamicBlurLevel)); i++) {
            if (fnum == DblCaptureBokeh[i].f_number) {
                blur = DblCaptureBokeh[i].blur_level;
                isFound = true;
                break;
            }
        }
        if (0 == blur && false == isFound) {
            blur = DEFAULT_CAPTURE_BLUR_LEVEL;
        }
    }

    ARC_LOGD("[ARC_RTB] blur_level = %d", blur);

    return blur;
}

#endif // DYNAMIC_BLUR_CONTROL_H