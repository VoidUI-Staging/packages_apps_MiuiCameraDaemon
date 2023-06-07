/*
 *  @file       morpho_hdsr_base.h
 *  @brief      Morpho hdsr common struct definition
 *  @date       2021/06/16
 *  @version    v1.0.0
 *
 *  Copyright(c) 2021 Morpho China,Inc.
 */

#ifndef _MORPHO_HDSR_BASE_H
#define _MORPHO_HDSR_BASE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "morpho_image_data_ex.h"
#include "morpho_image_format.h"
#include "morpho_rect_int.h"

#define MORPHO_HDSR_MIN_NUM_IMAGES_EV 1
#define MORPHO_HDSR_MAX_NUM_IMAGES_EV 20
#define MORPHO_HDSR_MAX_NUM_FACES     5
#define MORPHO_HDSR_MAX_ZOOM_RAIO     30.0
#define MORPHO_HDSR_MIN_ZOOM_RAIO     1.0

/*
 * @struct    morpho_HDSR
 * @brief     morpho super lowlight engine struct
 * @params
 *            handle : super lowlight engine pointer
 */
typedef struct _morpho_HDSR
{
    void *handle;
} morpho_HDSR;

/*
 * @struct    morpho_HDSR_MetaData
 * @brief     morpho super lowlight meta data struct
 * @params
 *            iso           : iso value
 *            exposure_time : sensor shutter value, the time unit is us, 1ms = 1000000us
 *            is_base_image : set the image if is as the base image for merging.
 */
typedef struct _morpho_HDSR_MetaData
{
    int iso;
    int exposure_time;
    int is_base_image;
} morpho_HDSR_MetaData;

/*
 * @struct    morpho_HDSR_InitParams
 * @brief     morpho hdsr initialized parameters struct
 *
 * @params    camera_id              : camera id
 * @params    width                  : width of input image
 * @params    height                 : height of input image
 * @params    stride                 : stride of input image
 * @params    output_width           : width of output image
 * @params    output_height          : height of output image
 * @parmas    num_input              : number of input images
 * @params    zoom_ratio             : zoom ratio, will be available when input size is equal output
 * size
 * @params    zoom_rect              : rect info of zoom area, will be available when zoom_ratio is
 * 0
 * @params    format                 : image format of input frame
 * @params    params_cfg_file        : parameter configuration file path
 */
typedef struct _morpho_HDSR_InitParams
{
    int camera_id;
    int width;
    int height;
    int stride;
    int output_width;
    int output_height;
    int num_input;
    float zoom_ratio;
    morpho_RectInt zoom_rect;
    morpho_ImageFormat format;

    char *params_cfg_file;
} morpho_HDSR_InitParams;

#ifdef __cplusplus
} /* extern "C" { */
#endif

#endif /* _MORPHO_HDSR_BASE_H */
