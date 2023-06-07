/*
 * Copyright (c) 2019 AnC Technology Co., Ltd. All rights reserved.
 */
// NOLINT
#ifndef DEPTH_INCLUDE_SILCLIENT_DEPTH_H_
#define DEPTH_INCLUDE_SILCLIENT_DEPTH_H_

#include "image/Image.h"
#include "common/wdepth_common.h"
#include "utils/base_export.h"

namespace wa {

//! @addtogroup Depth2
//! @{

class BASE_EXPORT Depth {
 public:
  /** @brief Structure method.
   *
   * allocate buffer size with parameter.
   */
  Depth();
  ~Depth();
  Depth(const Depth& dp);
  Depth& operator=(const Depth& dp);

  /** @brief Set depth config data.
   *
   * @param[in] config Depth config data struct.
   *            if working on uncalibration mode,
   *            the calibration data should be set nullptr,
   *            and data length should be 0.
   *
   * @return 1 if success, otherwise failure.
   */
  int init(const WDepthConfig& config);

  /** @brief Set calibration data.
   *         if working on uncalibration mode, this API is no necessary.
   *
   * @param[in] data The calibration data.
   * @param[in] size The size of calibration data.
   *
   * @return 1 if success, otherwise falure.
   *
   * @DEPRECATED  and "init" is suggested.
   */
  int setCalibData(const char* data, const int size);

  /** @brief Set model data.
   *
   * @param[in] path Model file path.
   *                 if only one model, set full model file path.
   *                 if multi models, just only set model file folder path.
   * @param[in] model Model file data.
   *                  only available for one model condition.
   *                  if multi models, just set null.
   * @param[in] size Model file data size.
   *                 only available for one model condition.
   *                 if nulti models, just set 0.
   *
   * @return 1 if success, otherwise failure.
   *
   * @DEPRECATED  and "init" is suggested.
   */
  int setModel(const char* path, const void* model, int size);

  /** @brief Set human face info.
   *
   * @param[in] faces The faces roi rect.
   * @param[in] faceCounts The face numbers, match the face rect counts.
   * @param[in] phoneAngle The rotation angle for phone.
   *                       In vertical condition is 0,
   *                       then add by 90 degrees clockwise.
   *
   * @return 1 if success, otherwise failure.
   */
  int setFaces(const WFace* faces, int faceCounts, Image::Rotation phoneAngle);

  /** @brief Rechanges buffer size.
   *
   * @param[in] mainW Reset width of pre-allocate buffer.
   * @param[in] mainH Reset height of pre-allocate buffer.
   *
   * @return 1 if success, otherwise failure.
   */
  int resize(int mainW, int mainH);

  /** @brief Set main image and aux image.
   *
   * @param[in] mainImg Main image.
   * @param[in] auxImg  Aux image.
   */
  void setImages(const Image& mainImg, const Image& auxImg);

  /** @brief Run algorithm.
   *
   * @param[out] outImg Output Image.
   *
   * @return 1 if success, otherwise failure.
   */
  int run(Image& outImg);

  /** @brief Get mask image.
   *
   * @param[out] maskImg the mask image.
   */
  void getMask(Image& maskImg);

  /** @brief get pre width of depth.
   *
   * the function invoked after setImages().
   *
   * @return the width value of depth.
   */
  int getWidthOfDepth();

  /** @brief get pre height of depth.
   *
   * the function invoked after setImages().
   *
   * @return the height value of depth.
   */
  int getHeightOfDepth();

  /** @brief Get pre byte of per pixel.
   *         if google depth, pixel is 1 byte.
   *         if wa depth, pixel is 4 byte.
   *
   * - 8U:  1 byte
   * - 16U: 2 byte
   * - 32F: 4 byte
   *
   * @return the byte of per depth pixel.
   */
  unsigned int getElemSizeOfDepth();

  /** @brief Get depth format for google depth.
   *
   * @return the format of google depth.
   *     - 0: RangeInverse
   *     - 1: RangeLinear
   */
  int getFormatOfDepthWithGoogle();

  /** @brief Get max deep value(cm) of depth image.
   *         for google depth.
   *
   * @return the far value of depth.
   */
  float getFarOfDepth();

  /** @brief Get min deep value(cm) of depth image.
   *         for google depth.
   *
   * @return the near value of depth.
   */
  float getNearOfDepth();

  /** @brief Get algo version number.
   *
   * @return algo version number.
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
};  // class Depth

//! @} Depth2

/**
 * @brief Define Depth API version.
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
#define ANC_DEPTH_API_VERSION "2.0.1_2021111715"

}  // namespace wa

#endif  // DEPTH_INCLUDE_SILCLIENT_DEPTH_H_
