/*
 * Copyright (c) 2019 AnC Technology Co., Ltd. All rights reserved.
 */
#ifndef WIDELENSUNDISTORTSERVER_INCLUDE_COMMON_WUNDISTORT_COMMON_H_
#define WIDELENSUNDISTORTSERVER_INCLUDE_COMMON_WUNDISTORT_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief define undistort mode information struct
 */
typedef enum {
    WUNDISTORT_MODE_PREVIEW = 0, ///< preview mode
    WUNDISTORT_MODE_CAPTURE      ///< capture mode
} WUndistortMode;

/**
 * @brief define static config data
 */
typedef struct
{
    WUndistortMode mode; ///< run mode
    WDataBlob mecpBlob;  ///< single camera module params
    WModelInfo model;    ///< model information
} WUndistortConfig;

/**
 * @brief Runtime parameters
 */
typedef struct
{
    WFaceInfo faces;  ///< Human face info
    WSize originSize; ///< Origin image size before crop or resize
    WRect cropRect;   ///< crop rect in the origin image
} WUndistortParam;

#ifdef __cplusplus
} // extern "C"
#endif

#endif // WIDELENSUNDISTORTSERVER_INCLUDE_COMMON_WUNDISTORT_COMMON_H_
