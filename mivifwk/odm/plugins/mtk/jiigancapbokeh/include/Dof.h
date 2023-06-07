/*
 * Copyright (c) 2019 AnC Technology Co., Ltd. All rights reserved.
 */
// NOLINT

// clang-format off
#ifndef DOF_INCLUDE_SILCLIENT_CC_V2_DOF_H_
#define DOF_INCLUDE_SILCLIENT_CC_V2_DOF_H_

#include "image/Image.h"
#include "common/wdof_common.h"

namespace wa {

//! @addtogroup Dof2
//! @{

/** @example example_dof_v2.cpp
 *
 * An example using Dof algorithm
 */

/** @brief Provide refocus effert for dual camera
 *
 * the class using @ref Image2
 *
 * @deprecated version 4 be used
 */
class BASE_EXPORT Dof {
 public:
  Dof();
  ~Dof();
  Dof(const Dof& df);
  Dof& operator=(const Dof& df);

  /** @brief Set dof config data.
   *
   * @param[in] config Dof config data struct.
   *
   * @return 1 if success, otherwise failure.
   */
  int init(const WDofConfig& config);

  /** @brief Set calibration data.
   *
   * @param[in] data The calibration data.
   * @param[in] size The size of calibration data.
   *
   * @return 1 if success, otherwise falure
   *
   * @DEPRECATED  and "init" is suggested.
   */
  int setCalibData(const char* data, int size);

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
   *
   * @DEPRECATED
   */
  int resize(int mainW, int mainH);

  /** @brief Set main image and aux image.
   *
   * @param[in] mainImg Main image.
   * @param[in] auxImg Aux image.
   */
  void setImages(Image& mainImg, Image& auxImg);

  /** @brief Set bokeh spot type.
   *
   * @param[in] bt Bokeh spot type.
   */
  void setBokehSpotType(WBokehSpotType bt);

  /** @brief Run algorithm.
   *
   * @param[in] focusX The focus point x axis value.
   *                   valid range[0, mainImg.realWidth()].
   * @param[in] focusY The focus point y axis value.
   *                   valid range[0, mainImg.realHeight()].
   * @param[in] fNum The fnumber value.
   *                 valid range[1.0f, 16.0f]
   * @param[out] outRefocused Output Image.
   *
   * @return 1 if success, otherwise failure.
   */
  int run(int focusX, int focusY, float fNum, Image& outRefocused);

  /** @brief Get width of depth.
   *
   * @return width of depth.
   */
  int getWidthOfDepth();

  /** @brief Get height of depth.
   *
   * @return height of depth.
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

  /** @brief Get depth image.
   *
   * @param[out] depthImg the depth image.
   */
  void getDepth(Image& depthImg);

  /** @brief Get mask image.
   *
   * @param[out] maskImg the mask image.
   */
  void getMask(Image& maskImg);

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
};  // class Dof

//! @} Dof2

}  // namespace wa

#endif  // DOF_INCLUDE_SILCLIENT_CC_V2_DOF_H_
// clang-format on
