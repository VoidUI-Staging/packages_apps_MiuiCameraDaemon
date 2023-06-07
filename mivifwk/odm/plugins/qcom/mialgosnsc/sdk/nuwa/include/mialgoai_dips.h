//==============================================================================
//
//  Copyright (c) 2018-2021 Xiaomi Camera AI Group.
//  All Rights Reserved.
//  Confidential and Proprietary - Xiaomi Camera AI Group.
//
//==============================================================================

/*
 * mialgoai_DIPS_.h
 *
 *  Created on: Dec 14, 2020
 *  Updated on: Jan 19, 2021
 *      Author: jishilong@xiaomi.com
 *     Version: 1.4.3
 */

#ifndef MIALGOAI_DIPS_H_
#define MIALGOAI_DIPS_H_

#define MIALGOAI_DIPS_API __attribute__((visibility("default")))

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Data Types
//=============================================================================

typedef enum {
    OK = 0,
    BAD_ARG = 1,
    NULL_PTR = 2,
    BAD_BUFFER = 3,
    BAD_OP = 4,
    BAD_PIPELINE = 5,
    INTERRUPT = 6,
    TIMEOUT = 7,
    UNSPECIFIED = 8,
    NULL_HANDLE = 9,
    REINIT_MODEL = 10
} MialgoAi_DIPS_Status;

typedef void *MialgoAi_DIPS_Handle;

typedef struct
{
    int major;
    int minor;
    int patch;
    int revision;
} MialgoAi_DIPS_Verison;

typedef struct
{
    float ratio;
    int output_type;
    int frame_abandon;
} MialgoAi_DIPS_ExecLog;

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
} MialgoAi_DIPS_GyroElement;

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
} MialgoAi_DIPS_FrameTime;

/**
 * @brief Data structure of gyroscope informations associating to a frame.
 * @member count        the number of gyroscope elements in the frame.
 * @member gyro_eles    all gyroscope elements data.
 * @member frame_time   frame time information.
 */
typedef struct
{
    int count;
    MialgoAi_DIPS_GyroElement *gyro_eles;
    MialgoAi_DIPS_FrameTime frame_time;
} MialgoAi_DIPS_GyroData;

/**
 * @brief Type of raw image.
 * MIPI_RAW_10BIT     10bit mipi raw image.
 * IDEAL_RAW_14BIT    14bit ideal raw image.
 */
typedef enum { MIPI_RAW_10BIT, IDEAL_RAW_14BIT } MialgoAi_DIPS_RawType;

/**
 * @brief Bayer pattern of raw image.
 */
typedef enum { BAYER_RGGB, BAYER_GRBG, BAYER_BGGR, BAYER_GBRG } MialgoAi_DIPS_BayerPattern;

/**
 * @brief Modes provided by ell capture algorithm.
 * TRIPOD     Pipeline deal with sufficiently stable cases.
 * GENERAL    Pipeline deal with other cases.
 * [todo]     other modes are on developing...
 */
typedef enum { TRIPOD, GENERAL } MialgoAi_DIPS_Mode;

typedef enum { MIALGO_ELLC, MIALGO_SNSC } MialgoAi_DIPS_Type;

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
    MialgoAi_DIPS_RawType raw_type;
    int num_frames;
    int height;
    int width;
    int stride;
    int black_level[4];
    int white_level;
    MialgoAi_DIPS_BayerPattern bayer_pattern;
} MialgoAi_DIPS_RawBurstInfo;

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
    float adrc_gain;
    float blue_gain;
    float red_gain;
    float isp_gain;
    float lux_index;
} MialgoAi_DIPS_GainInfo;

/**
 * @brief Data structure to encapsulate parameters when calling <em>MialgoAi_DIPS_Init</em>.
 * @member raw_burst_info   informations of raw burst.
 * @member mode             mode.
 * @member path_assets      path to assets file.
 * @member path_backend     path to backend dynamic library.
 */
typedef struct
{
    MialgoAi_DIPS_RawBurstInfo raw_burst_info;
    MialgoAi_DIPS_Mode mode;
    MialgoAi_DIPS_Type dips_type;
    const char *path_assets;
    const char *path_backend;
    float reserved[4];
} MialgoAi_DIPS_InitParams;

/**
 * @brief Data structure to encapsulate parameters when calling <em>MialgoAi_DIPS_Init_Model</em>.
 * @member num_frames   number of frames in a burst..
 * @member mode             mode.
 * @member path_assets      path to assets file.
 * @member path_backend     path to backend dynamic library.
 */
typedef struct
{
    MialgoAi_DIPS_Type dips_type;
    int num_frames;
    const char *path_assets;
    const char *path_backend;
    float reserved[4];
} MialgoAi_DIPS_InitModelParams;

/**
 * @brief Data structure to encapsulate parameters when calling <em>MialgoAi_DIPS_Preprocess</em>.
 * @member raw_data       raw image data.
 * @member fd
 * @member gyro_data      gyroscope data.
 * @member gain_info        informations of gain.
 */
typedef struct
{
    void *raw_data;
    int fd;
    MialgoAi_DIPS_GyroData gyro_data;
    MialgoAi_DIPS_GainInfo gain_info;
    float motion_max;
    int *base_index = nullptr;
    int banding;
    int focus_status;
    float reserved[4];
} MialgoAi_DIPS_PreprocessParams;

/**
 * @brief Data structure to encapsulate parameters when calling <em>MialgoAi_DIPS_Run</em>.
 * @member raw_data       raw image data.
 */
typedef struct
{
    void *raw_data;
    int fd;
    int *base_index = nullptr;
    float reserved[4];
} MialgoAi_DIPS_RunParams;

typedef int (*FN_MialgoAi_DIPS_android_log_print)(int prio, const char *tag, const char *fmt, ...);

//=============================================================================
// Public Functions
//=============================================================================

/**
 * @brief Get version of ell capture algorithm.
 * @return
 *        <em>MialgoAi_DIPS_Verison</em>
 */
MIALGOAI_DIPS_API
MialgoAi_DIPS_Verison MialgoAi_DIPS_GetVersion(void);

MIALGOAI_DIPS_API
MialgoAi_DIPS_Status MialgoAi_DIPS_SetLogLevel(
    int logLevel, // 0 is disable log and 1 is enable all log
    FN_MialgoAi_DIPS_android_log_print fn_log_print);

/**
 * @brief Initialize ell capture algorithm.
 * @param[out] DIPS__handle      handle.
 * @param[in] DIPS__init         parameters to initialize DIPS_.
 * @return
 *        <em>MialgoAi_DIPS_Error</em>
 */
MIALGOAI_DIPS_API
MialgoAi_DIPS_Status MialgoAi_DIPS_Init_Model(
    MialgoAi_DIPS_Handle &dips_handle, const MialgoAi_DIPS_InitModelParams &dips_init_params);

/**
 * @brief Initialize ell capture algorithm.
 * @param[out] DIPS__handle      handle.
 * @param[in] DIPS__init         parameters to initialize DIPS_.
 * @return
 *        <em>MialgoAi_DIPS_Error</em>
 */
MIALGOAI_DIPS_API
MialgoAi_DIPS_Status MialgoAi_DIPS_Init(MialgoAi_DIPS_Handle &dips_handle,
                                        const MialgoAi_DIPS_InitParams &dips_init_params);

/**
 * @brief Multiple preprocess ell capture algorithm. This function must be called as many times as
 * the value of DIPS__init_params.raw_burst_info.num_frames passed at initializing step.
 * @param[in] DIPS__handle         handle.
 * @param[in] DIPS__init_params    parameters to initialize DIPS_.
 * @return
 *        <em>MialgoAi_DIPS_Error</em>
 */
MIALGOAI_DIPS_API
MialgoAi_DIPS_Status MialgoAi_DIPS_Preprocess(
    const MialgoAi_DIPS_Handle &dips_handle,
    const MialgoAi_DIPS_PreprocessParams &dips_preprocess_params);

/**
 * @brief Run ell capture algorithm.
 * @param[in] DIPS__handle           handle.
 * @param[out] DIPS__run_params
 * @return
 *        <em>MialgoAi_DIPS_Error</em>
 */
MIALGOAI_DIPS_API
MialgoAi_DIPS_Status MialgoAi_DIPS_Run(const MialgoAi_DIPS_Handle &dips_handle,
                                       MialgoAi_DIPS_RunParams &dips_run_params);

/**
 * @brief Uninitialize ell capture algorithm. <em>DIPS__handle</em> is assigned to
 * null after calling this function. This function must be called at the end of
 * processing thread.
 * @param[in,out] DIPS__handle         handle.
 * @return
 *        <em>MialgoAi_DIPS_Error</em>
 */
MIALGOAI_DIPS_API
MialgoAi_DIPS_Status MialgoAi_DIPS_Unit(MialgoAi_DIPS_Handle &dips_handle);

/**
 * @brief Uninitialize ell capture algorithm. <em>DIPS__handle</em> is assigned to
 * null after calling this function. This function must be called at the end of
 * processing thread.
 * @param[in,out] DIPS__handle         handle.
 * @return
 *        <em>MialgoAi_DIPS_Error</em>
 */
MIALGOAI_DIPS_API
MialgoAi_DIPS_Status MialgoAi_DIPS_Unit_Model(MialgoAi_DIPS_Handle &dips_handle);

/**
 * @brief Terminate ell capture algorithm. Under normal use cases, user calls this function
 * in different thread and it will block the thread not more than <em>wait_time</em>.
 * When user calls <em>MialgoAi_DIPS_Terminate</em> before <em>MialgoAi_DIPS_Unit</em>
 * returns, it returns either OK or TIMEOUT. When the returned status is
 * TIMEOUT, user shall not use the processing thread occupied until <em>DIPS__handle</em>
 * is null.
 * @param[in,out] DIPS__handle         handle.
 * @param[in] wait_time               representing the maximum time to spend waiting(ms).
 * @return
 *        <em>MialgoAi_DIPS_Error</em>
 */
MIALGOAI_DIPS_API
MialgoAi_DIPS_Status MialgoAi_DIPS_Terminate(MialgoAi_DIPS_Handle &dips_handle, int wait_time);

/**
 * @brief Get execution log of ell capture algorithm.
 * @param[in,out] DIPS__handle         handle.
 * @param[in,out] DIPS__execlog        DIPS_ exectution log.
 * @return
 *        <em>MialgoAi_DIPS_Error</em>
 */
MIALGOAI_DIPS_API
MialgoAi_DIPS_Status MialgoAi_DIPS_GetExecLog(MialgoAi_DIPS_Handle &dips_handle,
                                              MialgoAi_DIPS_ExecLog &dips_execlog);

#ifdef __cplusplus
}
#endif

#endif /* MIALGOAI_DIPS__H_ */
