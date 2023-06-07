/*
 * Copyright (c) 2019 AnC Technology Co., Ltd. All rights reserved.
 */
#ifndef IMAGE_INCLUDE_COMMON_WCOMMON_H_
#define IMAGE_INCLUDE_COMMON_WCOMMON_H_

#include "utils/base_defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief define point in coordinate system using int 32bit
 */
typedef struct {
  WInt32 x;  ///< integer 32bit, x coordinate
  WInt32 y;  ///< integer 32bit, y coordinate
} WPoint2i;

/**
 * @brief define point in coordinate system using float 32bit
 */
typedef struct {
  WFloat32 x;  ///< float 32bit, x coordinate
  WFloat32 y;  ///< float 32bit, y coordinate
} WPoint2f;

/**
 * @brief define 3d coordinate using float 32bit
 */
typedef struct {
  WFloat32 x;
  WFloat32 y;
  WFloat32 z;
} WPoint3f;

/**
 * @brief define point in coordinate system using float 64bit
 */
typedef struct {
  WFloat64 x;  ///< float 64bit, x coordinate
  WFloat64 y;  ///< float 64bit, y coordinate
} WPoint2d;

/**
 * @brief define width and height
 */
typedef struct {
  WInt32 width;
  WInt32 height;
} WSize;

/** @brief define rectangle range
 * <br>NOTES: right/bottom from original x/y coordinate
 */
typedef struct {
  WInt32 left;    ///< left value of rectangle
  WInt32 top;     ///< top value of rectangle
  WInt32 right;   ///< right value of rectangle
  WInt32 bottom;  ///< bottom value of rectangle
} WRect;

/** @brief define rectangle range
 * <br>NOTES: width/height from original x/y coordinate
 */
typedef struct {
  WInt32 left;    ///< left value of rectangle
  WInt32 top;     ///< top value of rectangle
  WInt32 width;   ///< width value of rectangle
  WInt32 height;  ///< height value of rectangle
} WRectangle;

/**
 * @brief define human face
 */
typedef struct {
  WRect rect;  ///< face rect
} WFace;

/**
 * @brief define orientation
 */
typedef enum {
  WORIENTATION_0 = 0,      ///< 0 degree or 360 degree
  WORIENTATION_90 = 90,    ///< 90 degree
  WORIENTATION_180 = 180,  ///< 180 degree
  WORIENTATION_270 = 270,  ///< 270 degree
} WOrientation;

/**
 * @brief define face info struct
 *
 * @code {c}
 *   WInt32 counts = 2;
 *   WRect tFaceRoi[counts];
 *         tFaceRoi[0].left = 0;
 *         tFaceRoi[0].top = 0;
 *         tFaceRoi[0].right = 10;
 *         tFaceRoi[0].bottom = 10;
 *         tFaceRoi[1].left = 20;
 *         tFaceRoi[1].top = 20;
 *         tFaceRoi[1].right = 30;
 *         tFaceRoi[1].bottom = 30;
 *   WOrientation tfaceOrient[counts];
 *         faceOrient[0] = WORIENTATION_0;
 *         faceOrient[1] = WORIENTATION_0;
 *
 *   WFaceInfo faceInfo;
 *         faceInfo.counts = counts;
 *         faceInfo.faceRoi = tFaceRoi;
 *         faceInfo.faceOrient = tfaceOrient;
 * @endcode {c}
 */
typedef struct {
  WInt32 counts;             ///< face counts
  WRect* faceRoi;            ///< face roi, one roi per face
  WOrientation* faceOrient;  ///< face orientation, one face roi one face
                             ///< orientation, clockwise.
} WFaceInfo;

/**
 * @brief define model data type
 */
typedef enum {
  WMODEL_DATA_TYPE_CACHE_CL = 0,                      ///< opencl cache data
  WMODEL_DATA_TYPE_CACHE_MDL,                         ///< mdl cache data
  WMODEL_DATA_TYPE_MODEL,                             ///< model data
  WMODEL_DATA_TYPE_MAX = WMODEL_DATA_TYPE_MODEL + 1,  ///< extend type
} WModelDataType;

/**
 * @brief define data blob struct
 */
typedef struct {
  void* data;      ///< data content
  WUInt32 length;  ///< data length
} WDataBlob;

/**
 * @brief define model data blob struct
 */
typedef struct {
  WDataBlob blob;       ///< data blob
  WModelDataType type;  ///< data type
} WModelDataBlob;

/**
 * @brief define model data info struct
 *
 * @code {c}
 *   WModeInfo tModeInfo;
 *   tModeInfo.path = nullptr;
 *   tModeInfo.counts = 2;
 *   tModeInfo.modelData[0].blob.data = malloc(100);
 *   tModeInfo.modelData[1].blob.data = malloc(100);
 * @endcode {c}
 */
typedef struct {
  WInt8* path;                ///< model file path
  WModelDataBlob* modelData;  ///< model file data
  WInt32 counts;              ///< model file data counts
} WModelInfo;

/**
 * @brief define depth information struct
 */
typedef struct {
  WInt32 width;    ///< depth image width
  WInt32 height;   ///< depth image height
  WInt8 elemSize;  ///< depth image pixel size,
                   ///< 8U: 1byte, 16U: 2byte, 32F: 4byte
  WInt8 channel;   ///< depth image channel, usually 1 channel.
} WDepthInfo;

/**
 * @brief define depth information struct
 */
typedef enum {
  WGDEPTH_FMT_INVERSE = 0,                   ///< inverse depth
  WGDEPTH_FMT_LINEAR,                        ///< linear depth
  WGDEPTH_FMT_MAX = WGDEPTH_FMT_LINEAR + 1,  ///< extend
} WGDepthFormat;

/**
 * @brief define google depth information struct
 */
typedef struct {
  WGDepthFormat format;  ///< google depth format
  WFloat32 farVal;       ///< max deep value(cm) of depth
  WFloat32 nearVal;      ///< min deep value(cm) of depth
} WGDepthInfo;

/**
 * @brief define blur ability information struct
 */
typedef struct {
  WFloat32 maxVal;   ///< max value of blur level. usually 1.0f.
  WFloat32 minVal;   ///< min value of blur level. usually 16.0f.
  WFloat32 stepVal;  ///< blur step value. usually 0.1f per step.
} WBlurAbility;

typedef enum {
  WCALIB_TYPE_W_U,                            ///< wide + ultra
  WCALIB_TYPE_W_T,                            ///< wide + tele
  WCALIB_TYPE_T_P,                            ///< tele + periscope
  WCALIB_TYPE_W_P,                            ///< wide + periscope
  WCALIB_TYPE_MAX = WCALIB_TYPE_W_P + 1,      ///< extend
} WCalibDataType;

/**
 * @brief define calib data blob struct
 */
typedef struct {
  WCalibDataType type;          ///<  calib data type
  WDataBlob blob;               ///< calib data blob
} WCalibDataBlob;

/**
 * @brief define calib info struct
 */
typedef struct {
  WInt32 counts;                        ///< calib data counts
  WCalibDataBlob* calibData;            ///<  calib data
} WCalibInfo;

typedef enum {
  WCAM_ROLE_DEFAULT,                ///< Camera Type Default
  WCAM_ROLE_ULTRA,                  ///< Camera type ultra
  WCAM_ROLE_WIDE,                   ///< Camera type wide
  WCAM_ROLE_TELE,                   ///< Camera type tele
  WCAM_ROLE_PERISCOPE,              ///< Camera type periscope
} WCameraRole;

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // IMAGE_INCLUDE_COMMON_WCOMMON_H_
