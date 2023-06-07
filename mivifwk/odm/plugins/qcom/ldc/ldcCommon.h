#ifndef __LDC_COMMON_H__
#define __LDC_COMMON_H__
// clang-format off
#include "chi.h"
#include "device.h"
#include <string>

#define FACE_INFO_NUM_MAX   (10)
const std::string g_paraDataPath = "/odm/etc/camera/";
const std::string g_miCalibName0x01 = "MILDC_CALIB_0x01.bin"; // In most case only need 0x01
const std::string g_miCalibName0x02 = "MILDC_CALIB_0x02.bin";
const std::string g_miPreParasName = "MILDC_PREVIEW_PARAMS.json";
const std::string g_miPreParasNameOfPro = "MILDC_PREVIEW_PARAMS_PRO.json";
const std::string g_miSnapParasName = "MILDC_CAPTURE_PARAMS.json";
const std::string g_miSnapParasNameOfPro = "MILDC_CAPTURE_PARAMS_PRO.json";

typedef enum {
    LDC_PROCESS_MODE_BYPASS   = 0,
    LDC_PROCESS_MODE_PREVIEW  = 1,
    LDC_PROCESS_MODE_CAPTURE  = 2,
} LDC_PROCESS_MODE;

typedef enum {
    LDC_FACE_MASK_NEEDLESS     = 0,
    LDC_FACE_MASK_AILAB        = 1,
    LDC_FACE_MASK_MIALGOSEG    = 2,
    LDC_FACE_MASK_MIALGODETSEG = 3,
} LDC_FACE_MASK;

typedef enum {
    LDC_CAPTURE_VENDOR_ALTEK     = 0,
    LDC_CAPTURE_VENDOR_MIPHONE   = 1,
    LDC_CAPTURE_VENDOR_JIIGAN    = 2,
} LDC_CAPTURE_VENDOR;

typedef enum {
    LDC_PREVIEW_VENDOR_ALTEK     = 0,
    LDC_PREVIEW_VENDOR_MIPHONE   = 1,
    LDC_PREVIEW_VENDOR_JIIGAN    = 2,
} LDC_PREVIEW_VENDOR;

typedef enum {
    LDC_LIBSTATUS_INVALID     = 0,
    LDC_LIBSTATUS_VALID       = 1,
    LDC_LIBSTATUS_ERRORSTATE  = 2,
} LDC_LIBSTATUS;

typedef enum {
    LDC_LEVEL_DISABLED = (0) << (0),
    LDC_LEVEL_ENABLE   = (1) << (0),
} LDC_LEVEL;

// "persist.vendor.camera.LDC.debug"
typedef enum {
    LDC_DEBUG_INFO                  = (1) << (0),
    LDC_DEBUG_DUMP_IMAGE_PREVIEW    = (1) << (1),
    LDC_DEBUG_DUMP_IMAGE_CAPTURE    = (1) << (2),
    LDC_DEBUG_DUMP_TABLE            = (1) << (3),
    LDC_DEBUG_DIS_UPDATE_FACE       = (1) << (4),
    LDC_DEBUG_BYPASS_PREVIEW        = (1) << (6),
    LDC_DEBUG_BYPASS_CAPTURE        = (1) << (7),
    LDC_DEBUG_TEST_CAPTURE_ALTEK_AI = (1) << (8),
    LDC_DEBUG_PREVIEW_DRAWFACE      = (1) << (9),
    LDC_DEBUG_OUTPUT_DRAW_FACE_BOX  = (1) << (12),
    LDC_DEBUG_INPUT_DRAW_FACE_BOX   = (1) << (13),
} LDC_DEBUG;

typedef enum {
    SUCCESS     =  0,
    NEED_BYPASS = -1,
    MATCH_ERROR = -2,
} LdcErrorCode;

typedef struct ldcRect
{
    UINT32 left;   ///< x coordinate for top-left point
    UINT32 top;    ///< y coordinate for top-left point
    UINT32 width;  ///< width
    UINT32 height; ///< height
} LDCRECT;

typedef struct ldcFaceInfo {
    UINT32 num;
    LDCRECT face[FACE_INFO_NUM_MAX];
    UINT32 orient[FACE_INFO_NUM_MAX];
    UINT32 score[FACE_INFO_NUM_MAX];
} LDCFACEINFO;

struct LdcProcessInputInfo
{
    UINT64           frameNum;
    DEV_IMAGE_BUF*   inputBuf;
    DEV_IMAGE_BUF*   outputBuf;
    BOOL             isBurstShot;// only capture
    BOOL             isSATMode;
    BOOL             hasPhyMeta;
    BOOL             needUpdateFace;
    UINT32           vendorId;
    FLOAT            zoomRatio;
    LDCRECT          sensorSize;
    LDCRECT          cropRegion;
    LDCRECT          zoomWOI;
    LDCFACEINFO      faceInfo;
};

struct LdcProcessOutputInfo
{
    BOOL             hasUpdateFace;
    LDCFACEINFO      faceInfo;
};

inline void ldcRectToChi (CHIRECT* chiRec, const LDCRECT* ldcRec)
{
    chiRec->width = ldcRec->width;
    chiRec->height = ldcRec->height;
    chiRec->top = ldcRec->top;
    chiRec->left = ldcRec->left;
}

// clang-format on
#endif // __LDC_COMMON_H__