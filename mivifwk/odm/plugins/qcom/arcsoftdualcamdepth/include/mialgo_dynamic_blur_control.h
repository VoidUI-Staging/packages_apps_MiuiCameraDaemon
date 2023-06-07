/**
 * Bokeh Dynamic Blur Control
 **/

#ifndef MIALGO_DYNAMIC_BLUR_CONTROL_H
#define MIALGO_DYNAMIC_BLUR_CONTROL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utils/Log.h>

#define DEFAULT_REALTIME_BLUR_LEVEL 11
#define DEFAULT_CAPTURE_BLUR_LEVEL  11

typedef struct
{
    float f_number;
    int32_t blur_level;
} MIDynamicBlurLevel;

static MIDynamicBlurLevel MIDblRealtimeBokeh[] = {
    {1.0f, 80},   {1.1f, 77},   {1.2f, 75},   {1.4f, 72},   {1.6f, 70},  {1.8f, 68},  {2.0f, 65},
    {2.2f, 63},   {2.5f, 60},   {2.8f, 55},   {3.2f, 50},  {3.5f, 45}, {4.0f, 40}, {4.5f, 37},
    {5.0f, 35},  {5.6f, 33},  {6.3f, 30},  {7.1f, 27},  {8.0f, 25}, {9.0f, 23}, {10.0f, 20},
    {11.0f, 15}, {13.0f, 10}, {14.0f, 5}, {16.0f, 0},
};

static MIDynamicBlurLevel MIDblCaptureBokeh[] = {
    {1.0f, 60},   {1.1f, 55},   {1.2f, 53},   {1.4f, 50},   {1.6f, 48},  {1.8f, 46},  {2.0f, 44},
    {2.2f, 42},   {2.5f, 39},   {2.8f, 35},   {3.2f, 30},  {3.5f, 25}, {4.0f, 20}, {4.5f, 18},
    {5.0f, 15},  {5.6f, 10},  {6.3f, 9},  {7.1f, 8},  {8.0f, 7}, {9.0f, 6}, {10.0f, 5},
    {11.0f, 4}, {13.0f, 3}, {14.0f, 2}, {16.0f, 0},
};

inline int32_t findBlurLevelByFNumber(float fnum, bool isPreview)
{
    int32_t blur = 0;
    int32_t size = 0;
    int32_t isFound = false;
    if (true == isPreview) {
        isFound = false;
        size = sizeof(MIDblRealtimeBokeh) / sizeof(MIDynamicBlurLevel);
        for (int32_t i = 0; i < size; i++) {
            if (fnum == MIDblRealtimeBokeh[i].f_number) {
                blur = MIDblRealtimeBokeh[i].blur_level;
                isFound = true;
                break;
            }
        }
        if (0 == blur && false == isFound) {
            blur = DEFAULT_REALTIME_BLUR_LEVEL;
        }
    } else {
        isFound = false;
        size = sizeof(MIDblCaptureBokeh) / sizeof(MIDynamicBlurLevel);
        for (int32_t i = 0; i < (sizeof(MIDblCaptureBokeh) / sizeof(MIDynamicBlurLevel)); i++) {
            if (fnum == MIDblCaptureBokeh[i].f_number) {
                blur = MIDblCaptureBokeh[i].blur_level;
                isFound = true;
                break;
            }
        }
        if (0 == blur && false == isFound) {
            blur = DEFAULT_CAPTURE_BLUR_LEVEL;
        }
    }

    return blur;
}

#endif // MIALGO_DYNAMIC_BLUR_CONTROL_H
