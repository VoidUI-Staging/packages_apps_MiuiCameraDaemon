/*
 *  @file       morpho_hdsr.h
 *  @brief      Morpho hdsr API definition
 *  @date       2021/06/16
 *  @version    v1.0.0
 *
 *  Copyright(c) 2021 Morpho China,Inc.
 */

#ifndef _MORPHO_HDSR_H
#define _MORPHO_HDSR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "morpho_api.h"
#include "morpho_hdsr_base.h"
#include "morpho_image_data_ex.h"
#include "morpho_rect_int.h"

/*
 * @func    MorphoHDSR_getVersion
 * @brief    Get hdsr engine version
 * @params
 * @return    Version string
 */
MORPHO_API(const char *const)
MorphoHDSR_getVersion();

/*
 * @func    MorphoHDSR_init
 * @brief    Initialize hdsr engine
 * @params
 *            engine[in]: morpho_HDSR pointer
 *            init_params[in]: morpho_HDSRInitParams pointer
 * @return    Error code, reference to @morpho_error.h
 */
MORPHO_API(int)
MorphoHDSR_init(morpho_HDSR *const engine, const morpho_HDSR_InitParams *const init_params);

/*
 * @func     MorphoHDSR_setParamsByCfgFile
 * @brief    Set morpho super lowlight parameters by config file and specified condition
 * @params
 *           engine[in]: morpho_HDSR pointer
 *           meta_data[in]: set paramsters to engine according to meta data.
 * @return   Error code, refer to @morpho_error.h
 */
MORPHO_API(int)
MorphoHDSR_setParamsByCfgFile(const morpho_HDSR *const engine,
                              const morpho_HDSR_MetaData *const meta_data);

/*
 * @func    MorphoHDSR_addFaceRect
 * @brief    Add face data to hdsr engine to protect face area
 * @params
 *            engine[in]: morpho_HDSR pointer
 *            params[in]: morpho_RectInt pointer
 * @return    Error code, reference to @morpho_error.h
 */
MORPHO_API(int)
MorphoHDSR_addFaceRect(const morpho_HDSR *const engine, const morpho_RectInt *const face_rect);

/*
 * @func    MorphoHDSR_addImage
 * @brief    Add image data to hdsr engine
 * @params
 *            engine[in]: morpho_HDSR pointer
 *            params[in]: morpho_HDSRInputImgData pointer
 *          meta_data[in]: meda data of input frame
 * @return    Error code, reference to @morpho_error.h
 */
MORPHO_API(int)
MorphoHDSR_addImage(const morpho_HDSR *const engine, const morpho_ImageDataEx *const input_image,
                    const morpho_HDSR_MetaData *const meta_data);

/*
 * @func    MorphoHDSR_process
 * @brief    Start processing image data when call this API
 * @params
 *            engine[in]: morpho_HDSR pointer
 *            params[in/out]: morpho_HDSROutputImgData pointer
 *            context[in/out]: the context of super lowlight processing, it can be set NULL
 * @return    Error code, reference to @morpho_error.h
 */
MORPHO_API(int)
MorphoHDSR_process(const morpho_HDSR *const engine, morpho_ImageDataEx *const output_image,
                   void *context);

/*
 * @func    MorphoHDSR_uninit
 * @brief    Uninitialize hdsr engine and release memory
 * @params
 *            engine[in]: morpho_HDSR pointer
 * @return    Error code, reference to @morpho_error.h
 */
MORPHO_API(int)
MorphoHDSR_uninit(morpho_HDSR *const engine);

#ifdef __cplusplus
} /* extern "C" { */
#endif
#endif //_MORPHO_HDSR_H
