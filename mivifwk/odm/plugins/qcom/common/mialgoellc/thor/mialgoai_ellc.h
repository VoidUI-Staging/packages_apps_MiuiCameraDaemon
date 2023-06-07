//==============================================================================
//
//  Copyright (c) 2018-2021 Xiaomi Camera AI Group.
//  All Rights Reserved.
//  Confidential and Proprietary - Xiaomi Camera AI Group.
//
//==============================================================================

/*
 * mialgoai_ellc.h
 *
 *  Created on: Dec 14, 2020
 *  Updated on: Jan 19, 2021
 *      Author: jishilong@xiaomi.com
 *     Version: 1.4.3
 */

#ifndef MIALGOAI_ELLC_H_
#define MIALGOAI_ELLC_H_

#define MIALGOAI_ELLC_API __attribute__((visibility ("default")))

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Data Types
//=============================================================================

typedef enum
{
    OK = 0,
    BAD_ARG = 1,
    NULL_PTR = 2,
    BAD_BUFFER = 3,
    BAD_OP = 4,
    BAD_PIPELINE = 5,
    INTERRUPT = 6,
    TIMEOUT = 7,
    UNSPECIFIED = 8,
    NULL_HANDLE = 9
} MialgoAi_EllcStatus;

typedef void *MialgoAi_EllcHandle;

typedef struct
{
    int major;
    int minor;
    int patch;
    int revision;
} MialgoAi_EllcVerison;

typedef struct
{
    float ratio;
    int output_type;
    int frame_abandon;
} MialgoAi_EllcExecLog;

/**
 * @brief Data structure of gyroscope. It represents data sampled from gyroscope sensor at
 *    certain time point.
 * @member shift_x      shifting in direction X.
 * @member shift_y      shifting in direction Y.
 * @member shift_z      shifting in direction Z.
 * @member timestamp    time point when the data sampled.
 */
typedef struct
{
    float shift_x;
    float shift_y;
    float shift_z;
    unsigned long int timestamp;
} MialgoAi_EllcGyroElement;

/**
 * @brief Data structure of frame time information.
 * @member exp_beg      time point exposure begins.
 * @member exp_end      time point exposure ends.
 * @member exp_dur      duration of exposure.
 */
typedef struct
{
    unsigned long int exp_beg;
    unsigned long int exp_end;
    unsigned long int exp_dur;
} MialgoAi_EllcFrameTime;

/**
 * @brief Data structure of gyroscope informations associating to a frame.
 * @member count        the number of gyroscope elements in the frame.
 * @member gyro_eles    all gyroscope elements data.
 * @member frame_time   frame time information.
 */
typedef struct
{
    int count;
    MialgoAi_EllcGyroElement *gyro_eles;
    MialgoAi_EllcFrameTime frame_time;
} MialgoAi_EllcGyroData;

/**
 * @brief Type of raw image.
 * MIPI_RAW_10BIT     10bit mipi raw image.
 * IDEAL_RAW_14BIT    14bit ideal raw image.
 */
typedef enum
{
    MIPI_RAW_10BIT,
    IDEAL_RAW_14BIT
} MialgoAi_EllcRawType;

/**
 * @brief Bayer pattern of raw image.
 */
typedef enum
{
    BAYER_RGGB,
    BAYER_GRBG,
    BAYER_BGGR,
    BAYER_GBRG
} MialgoAi_EllcBayerPattern;

/**
 * @brief Modes provided by ell capture algorithm.
 * TRIPOD     Pipeline deal with sufficiently stable cases.
 * GENERAL    Pipeline deal with other cases.
 * [todo]     other modes are on developing...
 */
typedef enum
{
    TRIPOD,
    GENERAL
} MialgoAi_EllcMode;

/**
 * @brief Data structure to encapsulate all information raw burst needed.
 * @member raw_type         type of raw images.
 * @member num_frames       number of frames in a burst.
 * @member height           height of raw images.
 * @member width            width of raw images.
 * @member stride           stride of raw images in bytes.
 * @member black_level      black level across all channels.
 * @member white_level      white level of raw images.
 * @member bayer_pattern    bayer pattern of raw images.
 */
typedef struct
{
    MialgoAi_EllcRawType raw_type;
    int num_frames;
    int height;
    int width;
    int stride;
    int black_level[4];
    int white_level;
    MialgoAi_EllcBayerPattern bayer_pattern;
} MialgoAi_EllcRawBurstInfo;

/**
 * @brief Data structure to encapsulate gain informations.
 * @member cur_luma         current luma.
 * @member tar_luma         target luma.
 * @member shutter_prev     shutter speed of preview mode(ms).
 * @member shutter_capt     shutter speed of capture mode(ms).
 * @member isp_gain         isp gain of preview mode.
 * @member lux_index
 */
typedef struct
{
    float cur_luma;
    float tar_luma;
    float shutter_prev;
    float shutter_capt;
    float isp_gain_prev;
    float isp_gain_capt;
    float lux_index;
    float total_gain_capt;
} MialgoAi_EllcGainInfo;

/**
 * @brief Data structure to encapsulate parameters when calling <em>MialgoAi_ellcInit</em>.
 * @member raw_burst_info   informations of raw burst.
 * @member mode             mode.
 * @member path_assets      path to assets file.
 * @member path_backend     path to backend dynamic library.
 */
typedef struct
{
    MialgoAi_EllcRawBurstInfo raw_burst_info;
    MialgoAi_EllcMode mode;
    const char *path_assets;
    const char *path_backend;
} MialgoAi_EllcInitParams;

/**
 * @brief Data structure to encapsulate parameters when calling <em>MialgoAi_ellcPreprocess</em>.
 * @member raw_data       raw image data.
 * @member fd
 * @member gyro_data      gyroscope data.
 * @member gain_info        informations of gain.
 */
typedef struct
{
    void *raw_data;
    int fd;
    MialgoAi_EllcGyroData gyro_data;
    MialgoAi_EllcGainInfo gain_info;
} MialgoAi_EllcPreprocessParams;

/**
 * @brief Data structure to encapsulate parameters when calling <em>MialgoAi_ellcRun</em>.
 * @member raw_data       raw image data.
 */
typedef struct
{
    void *raw_data;
    int fd;
} MialgoAi_EllcRunParams;

typedef int (*FN_MialgoAi_ellc_android_log_print)(int prio, const char *tag, const char *fmt, ...);

//=============================================================================
// Public Functions
//=============================================================================

/**
 * @brief Get version of ell capture algorithm.
 * @return
 *        <em>MialgoAi_EllcVerison</em>
 */
MIALGOAI_ELLC_API
MialgoAi_EllcVerison MialgoAi_ellcGetVersion(void);

MIALGOAI_ELLC_API
MialgoAi_EllcStatus MialgoAi_ellc_SetLogLevel(int logLevel, //0 is disable log and 1 is enable all log
                                              FN_MialgoAi_ellc_android_log_print fn_log_print);

/**
 * @brief Initialize ell capture algorithm.
 * @param[out] ellc_handle      handle.
 * @param[in] ellc_init         parameters to initialize ellc.
 * @return
 *        <em>MialgoAi_EllcError</em>
 */
MIALGOAI_ELLC_API
MialgoAi_EllcStatus MialgoAi_ellcInit(MialgoAi_EllcHandle &ellc_handle,
                                      const MialgoAi_EllcInitParams &ellc_init_params);

/**
 * @brief Multiple preprocess ell capture algorithm. This function must be called as many times as
 * the value of ellc_init_params.raw_burst_info.num_frames passed at initializing step.
 * @param[in] ellc_handle         handle.
 * @param[in] ellc_init_params    parameters to initialize ellc.
 * @return
 *        <em>MialgoAi_EllcError</em>
 */
MIALGOAI_ELLC_API
MialgoAi_EllcStatus MialgoAi_ellcPreprocess(const MialgoAi_EllcHandle &ellc_handle,
                                            const MialgoAi_EllcPreprocessParams &ellc_preprocess_params);

/**
 * @brief Run ell capture algorithm.
 * @param[in] ellc_handle           handle.
 * @param[out] ellc_run_params
 * @return
 *        <em>MialgoAi_EllcError</em>
 */
MIALGOAI_ELLC_API
MialgoAi_EllcStatus MialgoAi_ellcRun(const MialgoAi_EllcHandle &ellc_handle,
                                     MialgoAi_EllcRunParams &ellc_run_params);

/**
 * @brief Uninitialize ell capture algorithm. <em>ellc_handle</em> is assigned to
 * null after calling this function. This function must be called at the end of
 * processing thread.
 * @param[in,out] ellc_handle         handle.
 * @return
 *        <em>MialgoAi_EllcError</em>
 */
MIALGOAI_ELLC_API
MialgoAi_EllcStatus MialgoAi_ellcUnit(MialgoAi_EllcHandle &ellc_handle);

/**
 * @brief Terminate ell capture algorithm. Under normal use cases, user calls this function
 * in different thread and it will block the thread not more than <em>wait_time</em>.
 * When user calls <em>MialgoAi_ellcTerminate</em> before <em>MialgoAi_ellcUnit</em>
 * returns, it returns either OK or TIMEOUT. When the returned status is
 * TIMEOUT, user shall not use the processing thread occupied until <em>ellc_handle</em>
 * is null.
 * @param[in,out] ellc_handle         handle.
 * @param[in] wait_time               representing the maximum time to spend waiting(ms).
 * @return
 *        <em>MialgoAi_EllcError</em>
 */
MIALGOAI_ELLC_API
MialgoAi_EllcStatus MialgoAi_ellcTerminate(MialgoAi_EllcHandle &ellc_handle,
                                           int wait_time);

/**
 * @brief Get execution log of ell capture algorithm.
 * @param[in,out] ellc_handle         handle.
 * @param[in,out] ellc_execlog        ellc exectution log.
 * @return
 *        <em>MialgoAi_EllcError</em>
 */
MIALGOAI_ELLC_API
MialgoAi_EllcStatus MialgoAi_ellcGetExecLog(MialgoAi_EllcHandle &ellc_handle, MialgoAi_EllcExecLog &ellc_execlog);

#ifdef __cplusplus
}
#endif

#endif /* MIALGOAI_ELLC_H_ */
