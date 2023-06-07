/**
 * Bokeh Dynamic Blur Control
 **/

#ifndef DYNAMIC_BLUR_CONTROL_H
#define DYNAMIC_BLUR_CONTROL_H

#include <stdio.h>
#include <stdlib.h>
#include <utils/Log.h>
#include <unistd.h>
#include <string.h>

#include "arcsoft_debuglog.h"

#define ROLE_REARBOKEH2X 61
#define ROLE_REARBOKEH1X 63
#define ARC_DEFAULT_BLUR_LEVEL 35
#define MI_DEFAULT_BLUR_LEVEL 11

typedef struct
{
    float f_number;
    int32_t blur_level;
} DynamicBlurLevel;

enum mappingVersion
{
    ArcSoftBokeh1x = 1,
    ArcSoftBokeh2x
};

static DynamicBlurLevel DblRealtimeBokeh[] = {
    {1.0f, 100}, {1.1f, 96}, {1.2f, 93}, {1.4f, 90}, {1.6f, 87}, {1.8f, 84},
    {2.0f, 80}, {2.2f, 77}, {2.5f, 74}, {2.8f, 70}, {3.2f, 65}, {3.5f, 60}, 
    {4.0f, 55}, {4.5f, 40}, {5.0f, 35}, {5.6f, 40}, {6.3f, 37}, {7.1f, 34}, {8.0f, 30}, 
    {9.0f, 27}, {10.0f, 24}, {11.0f, 20}, {13.0f, 10}, {14.0f, 5}, {16.0f, 0},
};

static DynamicBlurLevel DblCaptureBokeh1x[] = {
    {1.0f, 50}, {1.1f, 46}, {1.2f, 42}, {1.4f, 38}, {1.6f, 34}, {1.8f, 30},
    {2.0f, 26}, {2.2f, 23}, {2.5f, 20}, {2.8f, 18}, {3.2f, 16}, {3.5f, 15},
    {4.0f, 13}, {4.5f, 11}, {5.0f, 10}, {5.6f, 9}, {6.3f, 8}, {7.1f, 7}, {8.0f, 6},
    {9.0f, 5}, {10.0f, 4}, {11.0f, 3}, {13.0f, 2}, {14.0f, 1}, {16.0f, 0},
};

static DynamicBlurLevel DblCaptureBokeh2x[] = {
    {1.0f, 100}, {1.1f, 90}, {1.2f, 80}, {1.4f, 70}, {1.6f, 63}, {1.8f, 56},
    {2.0f, 50}, {2.2f, 46}, {2.5f, 43}, {2.8f, 40}, {3.2f, 36}, {3.5f, 33},
    {4.0f, 30}, {4.5f, 26}, {5.0f, 23}, {5.6f, 20}, {6.3f, 16}, {7.1f, 13}, {8.0f, 10},
    {9.0f, 8}, {10.0f, 6}, {11.0f, 5}, {13.0f, 3}, {14.0f, 1}, {16.0f, 0},
};

static DynamicBlurLevel MIDblRealtimeBokeh[] = {
    {1.0f, 0},   {1.1f, 1},   {1.2f, 2},   {1.4f, 3},   {1.6f, 4},  {1.8f, 5},  {2.0f, 6},
    {2.2f, 7},   {2.5f, 8},   {2.8f, 9},   {3.2f, 10},  {3.5f, 11}, {4.0f, 12}, {4.5f, 13},
    {5.0f, 14},  {5.6f, 15},  {6.3f, 16},  {7.1f, 17},  {8.0f, 18}, {9.0f, 19}, {10.0f, 20},
    {11.0f, 21}, {13.0f, 22}, {14.0f, 23}, {16.0f, 24},
};

static DynamicBlurLevel MIDblCaptureBokeh[] = {
    {1.0f, 0},   {1.1f, 1},   {1.2f, 2},   {1.4f, 3},   {1.6f, 4},  {1.8f, 5},  {2.0f, 6},
    {2.2f, 7},   {2.5f, 8},   {2.8f, 9},   {3.2f, 10},  {3.5f, 11}, {4.0f, 12}, {4.5f, 13},
    {5.0f, 14},  {5.6f, 15},  {6.3f, 16},  {7.1f, 17},  {8.0f, 18}, {9.0f, 19}, {10.0f, 20},
    {11.0f, 21}, {13.0f, 22}, {14.0f, 23}, {16.0f, 24},
};

static int32_t findBlurLevelByFNumber(DynamicBlurLevel *DblBokeh ,float fnum)
{
    int32_t blur = 0;
    uint32_t size = sizeof(DblCaptureBokeh2x) / sizeof(DynamicBlurLevel);
    for(uint32_t i = 0; i < size; i++)
    {
        if (fnum == DblBokeh[i].f_number)
        {
                blur = DblBokeh[i].blur_level;       
                break;
        }
    }
    ARC_LOGD("[ARC_RTB] blur_level = %d", blur);
    return blur;
};

static int32_t findBlurLevelByCaptureType(float fnum, bool isPreview, int BokehRole)
{
     int32_t bluelevel = ARC_DEFAULT_BLUR_LEVEL;
    if (true == isPreview)
    {
         bluelevel = findBlurLevelByFNumber(DblRealtimeBokeh, fnum);
    }
    else if(BokehRole == ROLE_REARBOKEH2X)
    {
        bluelevel = findBlurLevelByFNumber(DblCaptureBokeh2x, fnum);
    }
    else
    {
        bluelevel = findBlurLevelByFNumber(DblCaptureBokeh1x, fnum);
    }
    return bluelevel;
};

static int32_t findBlurLevelForLite(float fnum, bool isPreview)
{
     int32_t bluelevel = MI_DEFAULT_BLUR_LEVEL;
    if (true == isPreview)
    {
         bluelevel = findBlurLevelByFNumber(MIDblRealtimeBokeh, fnum);
    }
    else
    {
        bluelevel = findBlurLevelByFNumber(MIDblCaptureBokeh, fnum);
    }
    return bluelevel;
};

#endif //DYNAMIC_BLUR_CONTROL_H