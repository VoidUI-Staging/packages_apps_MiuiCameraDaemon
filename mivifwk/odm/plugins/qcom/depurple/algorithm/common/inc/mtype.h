/****************************************************************************
* File: mtype.h                                                             *
* Description: Global type definition                                       *
*   Please don't add new definition arbitrarily.                            *
*   Only system-member is allowed to modify this file.                      *
****************************************************************************/

#ifndef _MTYPE_H_
#define _MTYPE_H_

#include <stdint.h>

// Global constant
#ifndef TRUE
#define TRUE                    (1)
#endif

#ifndef FALSE
#define FALSE                   (0)
#endif

#ifndef NULL
#define NULL                    ((void *)0)
#endif

// Global data type
typedef float                   FLOAT32;
typedef double                  FLOAT64;

typedef uint8_t                 BYTE;
//typedef int8_t                  CHAR;

typedef int8_t                  INT8;
typedef uint8_t                 UINT8;
typedef int16_t                 INT16;
typedef uint16_t                UINT16;
typedef int32_t                 INT32;
typedef uint32_t                UINT32;
typedef int64_t                 INT64;
typedef uint64_t                UINT64;

typedef INT32 BOOL;

typedef struct cfrModeInfo{
    uint32_t sensorId;
    uint32_t vendorId;
    bool isHDMode;
    bool isSNMode;
    bool isInSensorZoom;
    bool isHDRMode;
    uint32_t zoomRatio;
    bool operator == (const cfrModeInfo &obj)  const{
        return (sensorId == obj.sensorId) && (vendorId == obj.vendorId) && (isHDMode == obj.isHDMode) &&
            (isSNMode == obj.isSNMode) && (isInSensorZoom == obj.isInSensorZoom) && (zoomRatio == obj.zoomRatio) && (isHDRMode == obj.isHDRMode);
    }
}CFR_MODEINFO_T;

enum DepurpleInitState {
    CFR_UNINIT,
    CFR_INIT_ENTER,
    CFR_INIT_FAIL,
    CFR_INT_SUCCESS,
};
#endif
