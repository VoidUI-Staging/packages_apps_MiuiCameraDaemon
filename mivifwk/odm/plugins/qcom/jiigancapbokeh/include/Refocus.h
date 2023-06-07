/*
 * Copyright (c) 2019 AnC Technology Co., Ltd. All rights reserved.
 */
// NOLINT
#ifndef REFOCUS_INCLUDE_SILCLIENT_REFOCUS_H_
#define REFOCUS_INCLUDE_SILCLIENT_REFOCUS_H_

#include "common/wrefocus_common.h"
#include "image/Image.h"
#include "utils/base_export.h"

namespace wa {

/** @example example_refocus.cc
 *
 * An example using refocus algorithm
 */

//! @addtogroup Refocus2
//! @{

class BASE_EXPORT Refocus {
 public:
  Refocus();
  ~Refocus();

  Refocus(const Refocus& rf);
  Refocus& operator=(const Refocus& rf);

  /** @brief Set refocus config data.
   *
   * @param[in] config Refocus config data struct.
   *
   * @return 1 if success, otherwise failure.
   */
  int init(const WRefocusConfig& config);

  /** @brief Set model data.
   * @deprecated
   * @param[in] path Model file path.
   *                 if only one model, set full model file path.
   *                 if multi models, just only set model file folder path.
   * @param[in] model Model file data. only available for one model condition.
   *                  if multi models, just set null.
   * @param[in] size Model file data size.
   *                 only available for one model condition.
   *                 if nulti models, just set 0.
   *
   * @return 1 if success, otherwise failure.
   */
  int setModel(const char* path, const void* model, int size);

  /** @brief Set human face info.
   * @deprecated
   * @param[in] faces The faces roi rect.
   * @param[in] faceCounts The face numbers, match the face rect counts.
   * @param[in] phoneAngle The rotation angle for phone while taking photo.
   *                       In vertical condition is 0, then add by 90 degrees
   *                       clockwise.
   *
   * @return 1 if success, otherwise failure.
   */
  int setFaces(const WFace* faces, int faceCounts, Image::Rotation phoneAngle);

  /** @brief Set main image and depth image.
   *
   * @param[in] mainImg Main image
   * @param[in] depthImg Depth image
   */
  void setImages(const Image& mainImg, const Image& depthImg);

  /**
   * @brief This api is used to set the main image,
   *        google depth image and mask image.
   *
   * @param[in] mainImg Main image
   * @param[in] depthImg Depth image
   * @param[in] maskImg Portrait mask image
   */
  void setImages(const Image& mainImg, const Image& depthImg,
                 const Image& maskImg);

  /** @brief Set bokeh spot type.
   *
   * @param[in] bt bokeh spot type.
   */
  void setBokehSpotType(WBokehSpotType bt);

  /** @brief Set bokeh style.
   *
   * @param[in] bt bokeh style.
   *
   * @return 1 if success, otherwise failure.
   */
  int setBokehStyle(WBokehStyle style);

  /** @brief Run algorithm.
   *
   * @param[in] focusX The focus point x axis value.
   *                   valid range[0, mainImg.realWidth()].
   * @param[in] focusY The focus point y axis value.
   *                   valid range[0, mainImg.realHeight()].
   * @param[in] fNum The fnumber value.
   *                 valid range[1.0f, 16.0f]
   * @param[out] outRefocusedImg Output Image.
   *
   * @return 1 if success, otherwise failure.
   */
  int run(int focusX, int focusY, float fNum, Image& outRefocusedImg);

  /** @brief Get algo version number.
   *
   * @return algo version number
   */
  const char* getVersion() const;

  /** @brief Get algo full version number.
   *
   * @deprecated
   *
   * @return algo full version number.
   */
  const char* getFullVersion(int flags) const;

 private:
  class Impl;
  Impl* p;
};  // class Refocus

//! @} Refocus2

/**
 * @brief Define Refocus API version.
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
#define ANC_REFOCUS_API_VERSION "2.0.1_2021112311"



}  // namespace wa

#endif  // REFOCUS_INCLUDE_SILCLIENT_REFOCUS_H_
