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
    {1.0f, 0},   {1.1f, 1},   {1.2f, 2},   {1.4f, 3},   {1.6f, 4},  {1.8f, 5},  {2.0f, 6},
    {2.2f, 7},   {2.5f, 8},   {2.8f, 9},   {3.2f, 10},  {3.5f, 11}, {4.0f, 12}, {4.5f, 13},
    {5.0f, 14},  {5.6f, 15},  {6.3f, 16},  {7.1f, 17},  {8.0f, 18}, {9.0f, 19}, {10.0f, 20},
    {11.0f, 21}, {13.0f, 22}, {14.0f, 23}, {16.0f, 24},
};

static MIDynamicBlurLevel MIDblCaptureBokeh[] = {
    {1.0f, 0},   {1.1f, 1},   {1.2f, 2},   {1.4f, 3},   {1.6f, 4},  {1.8f, 5},  {2.0f, 6},
    {2.2f, 7},   {2.5f, 8},   {2.8f, 9},   {3.2f, 10},  {3.5f, 11}, {4.0f, 12}, {4.5f, 13},
    {5.0f, 14},  {5.6f, 15},  {6.3f, 16},  {7.1f, 17},  {8.0f, 18}, {9.0f, 19}, {10.0f, 20},
    {11.0f, 21}, {13.0f, 22}, {14.0f, 23}, {16.0f, 24},
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
