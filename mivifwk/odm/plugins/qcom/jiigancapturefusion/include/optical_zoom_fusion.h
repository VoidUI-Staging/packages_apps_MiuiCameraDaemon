/*
 * Copyright (c) 2019 AnC Technology Co., Ltd. All rights reserved.
 * file: optical_zoom_fusion.h
 */
#ifndef OPTICALZOOMFUSION_INCLUDE_SILCLIENT_CC_V2_OPTICAL_ZOOM_FUSION_H_
#define OPTICALZOOMFUSION_INCLUDE_SILCLIENT_CC_V2_OPTICAL_ZOOM_FUSION_H_

#include "common/v1/wopz_fusion_common.h"
#include "common/wcommon.h"
#include "image/Image.h"
#include "utils/base_export.h"

namespace wa {

//! @addtogroup OpticalZoomFusion2
//! @{

/** @example example_fusion.cc
 *
 * An example using fusion SDK
 */

class BASE_EXPORT OpticalZoomFusion {
 public:
  OpticalZoomFusion();
  ~OpticalZoomFusion();

  OpticalZoomFusion(const OpticalZoomFusion& ozf);
  OpticalZoomFusion& operator=(const OpticalZoomFusion& ozf);

  /**
   * @brief Init algo with static config.
   *
   * @param config static config data include mecp bin, etc.
   *
   * @return true if success, false otherwise.
   */
  bool init(const WOpZFusionConfig& config);

  /**
   * @brief Set calibration data.
   *
   * @param calibInfo calibration info.
   *
   * @return true if success, false otherwise.
   */
  bool setCalibData(const WCalibInfo& calibInfo);

  /**
   * @brief Set model info, either path or data.
   *
   * @param modelInfo model info.
   *
   * @code
   *   WModelInfo modelInfo;
   *   // the model data file path if only one model, set full model file path
   *   // if multi models, just only set model file folder path.
   *   modelInfo.path = nullptr;
   *   // the model data.
   *   modelInfo.modelData[0].blob.data = malloc(10);
   *   // the model data length.
   *   modelInfo.modelData[0].blob.length = 10;
   *   // the model data file type.
   *   modelInfo.modelData[0].type = WMODEL_DATA_TYPE_MODEL;
   *   // the model file count, if one couple model include more than one file,
   *   // count > 1, the file struct should store more than one file info.
   *   modelInfo.count = 1;
   * @endcode
   *
   * @return true if success, false otherwise.
   */
  bool setModel(const WModelInfo& modelInfo);

  /**
   * @brief Set input main and aux images.
   *
   * @param mainImg the main origin image(the image with smaller FOV).
   * @param auxImg the aux origin image(the image with larger FOV).
   *
   * @return true if success, false otherwise.
   */
  bool setImages(const wa::Image& mainImg, const wa::Image& auxImg);

  /**
   * @brief Set config for run optical zoom fusion.
   *
   * @param params the config params.
   *               params.scale.zoomFactor zoom level value.
   *               params.scale.mainScale main camera scale value.
   *               params.scale.auxScale aux camera scale value.
   *               params.main the main object metadata.
   *                           the struct include luma info.
   *               params.aux the aux object metadata.
   *                          the struct include luma info.
   *
   * @return true if success, false otherwise.
   */
  bool setConfig(const WOpZFusionParams& params);

  /**
   * @brief Set focus point.
   *
   * @param focus the focus point.
   *              the focus point should be [x, y],
   *              x range[0, input image width]
   *              y range[0, input image height]
   *
   * @return true if success, false otherwise.
   */
  bool setFocus(const WPoint2i& focus);

  /**
   * @brief Set human face info.
   *
   * @param mainFaces: the detected faces roi on the main image.
   *                   (the image with smaller FOV is main image)
   *                   each face roi rect should be [left, top, right, bottom].
   * @param auxFaces: the detected faces roi on the aux image.
   *                  (the image with larger FOV is aux image)
   *                  each face roi rect should be [left, top, right, bottom].
   *
   * @return true if success, false otherwise.
   */
  bool setFaces(const WFaceInfo& mainFaces, const WFaceInfo& auxFaces);

  /**
   * @brief Run optical zoom fusion.
   *
   * @param outImg [out]output image.
   *
   * @return 0 if success, otherwise fail.
   */
  int run(wa::Image* outImg);

  /**
   * @brief Get version number.
   *
   * @return software version number.
   */
  const char* getVersion() const;

  /**
   * @brief Get internal api version.
   *
   * @return internal api version, ex: 2.0.0
   */
  const char* getApiVersion() const;

 private:
  class Head;
  Head* h;

  class Impl;
  Impl* p;
};  // class OpticalZoomFusion

//! @} OpticalZoomFusion2

/**
 * @brief Define Fusion API version.
 *
 * The format is major.minor.revision_date
 * - major: main version, fixed.
 * - minor: modifiable, for uncompatible api change. e.g rename api etc.
 * - revision: modifiable, for compatible api change. e.g comment udpate etc.
 * - date: modifiable, the date of add or remove api. accurate to hours.
 *
 * @note the value will be modified when the contents of api changed.
 *
 */
#define ANC_FUSION_API_VERSION "2.0.1_2021112413"
}  // namespace wa

#endif  // OPTICALZOOMFUSION_INCLUDE_SILCLIENT_CC_V2_OPTICAL_ZOOM_FUSION_H_
