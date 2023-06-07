/*
 * Copyright (c) 2019 AnC Technology Co., Ltd. All rights reserved.
 */
// NOLINT
// clang-format off
#ifndef IMAGE_INCLUDE_IMAGE_CC_V2_IMAGE_H_
#define IMAGE_INCLUDE_IMAGE_CC_V2_IMAGE_H_

#include <stdio.h>
#include "utils/base_export.h"

namespace wa {

//! @addtogroup Image2
//! @{

/** @brief Construction image and transfer to algorithm
 *
 * @deprecated version 4 be used
 */
class BASE_EXPORT Image {
 public:
  /** @brief Define image type
   * @ingroup Image2
   */
  enum ImageType {
    IMAGETYPE_GRAY = 0,    ///<  Gray image, 8 bit
    IMAGETYPE_RGB,         ///<  RGB format, 3 channels, 24 bit
    IMAGETYPE_BGR,         ///<  BGR format, 3 channels, 24 bit
    IMAGETYPE_RGBA,        ///<  RGBA format, 4 channels, 32 bit
    IMAGETYPE_BGRA,        ///<  BGRA format, 4 channels, 32 bit
    IMAGETYPE_NV12,        ///<  Packed(Semi-planar) YUV420SP format, YUV:YYUVUV
    IMAGETYPE_NV21,        ///<  Packed(Semi-planar) YUV420SP format, YVU:YYVUVU
    IMAGETYPE_I420,        ///<  Planar YUV420P format, YUV:YY.UU.VV
    IMAGETYPE_YV12,        ///<  Planar YUV420P format, YVU:YY.VV.UU
    IMAGETYPE_YUYV,        ///<  Packed YUV422, alias YUY2
    IMAGETYPE_YVYU,        ///<  Packed YUV422
    IMAGETYPE_UYVY,        ///<  Packed YUV422
    IMAGETYPE_VYUY,        ///<  Packed YUV422
    IMAGETYPE_P010 = 300,  ///<  Planar YUV420 format, 16bit
    IMAGETYPE_UBWC_TP10 = 400,  ///< UBWC TP10 format
    IMAGETYPE_UBWC_NV12,        ///< UBWC NV12 format
    IMAGETYPE_MAX,              ///<  Max enum number
  };

  /** @brief Define data type
   * @ingroup Image2
   */
  enum DataType {
    DT_8U,   ///< unsigned char, 8 bit
    DT_32F,  ///< float, 32 bit
    DT_16U,  ///< short, 16 bit
  };

  /** @brief Define rotation angle
   * @ingroup Image2
   *
   * The rotation relative to dual camera module(total:four modules).<br>
   * eg:
   * @verbatim
   *  camera module       default direction of image
   *    right(main)
   * |`````````````|        |`````````````|
   * |   |`| |`|   |        |      |      |
   * |   |_| |`|   |        |    \ | /    |
   * |             |        |     \|/     |
   * |             |        |    /`_`\    |
   * |             |        |    \___/    |
   * |_____________|        |_____________|
   * @endverbatim
   *
   * if input image below to algorithm,
   * then the rotation value is:
   *  - left:  ROTATION_90, 90 degree anticlockwise.
   *  - right: ROTATION_180, 180 degree anticlockwise.
   *
   * @verbatim
   * |````````````````|     |`````````````|
   * |        \       |     |    /```\    |
   * |          \---\ |     |    \_-_/    |
   * | ----------|>  ||     |     /|\     |
   * |          /---/ |     |    / | \    |
   * |        /       |     |      |      |
   * |-----------------     |_____________|
   * @endverbatim
   *
   * DO NOT CHANGE THE VALUE
   */
  enum Rotation {
    ROTATION_0 = 0,      ///< 0 degree or 360 degree
    ROTATION_90 = 90,    ///< 90 degree
    ROTATION_180 = 180,  ///< 180 degree
    ROTATION_270 = 270   ///< 270 degree
  };

  /** @brief Define store mode
   * @ingroup Image2
   */
  enum StoreMode {
    STORE_ALL = 0,  ///< One buffer to store all data
    STORE_Y_UV,  ///< Only YUV, one buffer to store y data, one buffer to store
                 ///< uv data
    STORE_Y_U_V  ///< Only YUV, one buffer to store y data, one buffer to store
                 ///< u data, one buffer to store v data
  };

  /** @brief Define aligned mode
   * @ingroup Image2
   */
  enum AlignedMode {
    ALIGNED_NO,  ///< NO Aligned
    ALIGNED_W,   ///< Aligned with width
    ALIGNED_H,   ///< Aligned with height
    ALIGNED_W_H  ///< Aligned with width and height
  };

  typedef enum {
    ROLE_DEFAULT = 0,  ///< Camera Type Default
    ROLE_ULTRA,        ///< Camera type ultra
    ROLE_WIDE,         ///< Camera type wide
    ROLE_TELE,         ///< Camera type tele
    ROLE_PERISCOPE,    ///< Camera type periscope
    ROLE_INVALID,      ///< Invalid role to indicate error
  } RoleType;

  /** @brief Empty image object
   * @ingroup Image2
   */
  Image();

  /** @brief Allocate buffer with w(width) and h(height),
   * the buffer managed automatically
   * @ingroup Image2
   *
   * use example:
   * @code {.cpp}
   *   // Create Image object and the buffer of specify size be allocated
   *   const int imageWidth = 1920;
   *   const int imageHeight = 1080;
   *   wa::Image tImage(imageWidth, imageHeight, wa::Image::IMAGETYPE_NV21,
   *                    wa::Image::DT_8U);
   * @endcode
   *
   * @param w matrix width
   * @param h matrix height
   * @param imgType image type
   * @param dt data type(DT_8U(unsigned char)/DT_32F(float)/etc)
   * @param sm how match buffer mallocated related to image type
   * Default: 1. do not aligned
   *          2. direction(ROTATION_0)
   */
  Image(int w, int h, ImageType imgType, DataType dt, StoreMode sm = STORE_ALL);

  /** @brief The buffer managed by allocator
   * @ingroup Image2
   *
   * if the image buffer be allocated, then create image image object as below:
   * @code {.cpp}
   *   const int imageWidth = 1920;
   *   const int imageHeight = 1080;
   *   // yuv height = image height * 3 / 2
   *   const int yuvWidth = imageWidth;
   *   const int yuvHeight = imageHeight * 3 / 2;
   *   unsigned char* buffer = (unsigned char*)malloc(yuvWidth * yuvHeight);
   *   // Create Image object and default rotation is 0
   *   wa::Image tImage(imageWidth, imageHeight, buffer,
   *                    wa::Image::IMAGETYPE_NV21, wa::Image::DT_8U);
   *   // set rotation, eg: 90 angle
   *   tImage.setRotation(wa::Image::ROTATION_90);
   * @endcode
   *
   * @param w matrix width
   * @param h matrix height
   * @param data buffer referenced
   * @param imgType image type
   * @param dt data type(DT_8U(unsigned char)/DT_32F(float)/etc)
   * @param r image anti-clockwise direction related to camera module
   * Default: 1. direction(ROTATION_0)
   */
  Image(int w, int h, void* data, ImageType imgType, DataType dt,
        Rotation r = ROTATION_0);

  /** @brief The buffer managed by allocator
   * @ingroup Image2
   *
   * if the image buffer be allocated and that is split to y / uv buffer,
   * then create image image object as below:
   * @code {.cpp}
   *   const int imageWidth = 1920;
   *   const int imageHeight = 1080;
   *   // yuv height = image height * 3 / 2
   *   const int yWidth = imageWidth;
   *   const int yHeight = imageHeight;
   *   const int uvWidth = yWidth;
   *   const int uvHeight = yHeight / 2;
   *   unsigned char* yBuffer = (unsigned char*)malloc(yWidth * yHeight);
   *   unsigned char* uvBuffer = (unsigned char*)malloc(uvWidth * uvHeight);
   *   // Create Image object and default rotation is 0
   *   wa::Image tImage(imageWidth, imageHeight, yBuffer, uvBuffer, NULL,
   *                    wa::Image::IMAGETYPE_NV21, wa::Image::DT_8U);
   *   // set rotation, eg: 90 angle
   *   tImage.setRotation(wa::Image::ROTATION_90);
   * @endcode
   *
   * if the image buffer be allocated and that is split to y / u / v buffer,
   * then create image image object as below:
   * @code {.cpp}
   *   const int imageWidth = 1920;
   *   const int imageHeight = 1080;
   *   // yuv height = image height * 3 / 2
   *   const int yWidth = imageWidth;
   *   const int yHeight = imageHeight;
   *   const int uWidth = yWidth / 2;
   *   const int uHeight = yHeight / 2;
   *   unsigned char* yBuffer = (unsigned char*)malloc(yWidth * yHeight);
   *   unsigned char* uBuffer = (unsigned char*)malloc(uWidth * uHeight);
   *   unsigned char* vBuffer = (unsigned char*)malloc(uWidth * uHeight);
   *   // Create Image object and default rotation is 0
   *   wa::Image tImage(imageWidth, imageHeight, yBuffer, uBuffer, vBuffer
   *                    wa::Image::IMAGETYPE_NV21, wa::Image::DT_8U);
   *   // set rotation, eg: 90 angle
   *   tImage.setRotation(wa::Image::ROTATION_90);
   * @endcode
   *
   * @param w matrix width
   * @param h matrix height
   * @param data1 yuv or rgb data
   * @param data2 u or uv data
   * @param data3 v data
   * @param imgType image type
   * @param dt data type(DT_8U(unsigned char)/DT_32F(float)/etc)
   * @param r image anti-clockwise direction related to camera module
   */
  Image(int w, int h, void* data1, void* data2, void* data3, ImageType imgType,
        DataType dt, Rotation r = ROTATION_0);

  /**
   * @ingroup Image2
   */
  Image(const Image& c);

  /**
   * @ingroup Image2
   */
  virtual ~Image();

  /**
   * @ingroup Image2
   */
  Image& operator=(const Image& img);

  /** @brief Create new Image and allocate buffer with w(width) and h(height),
   *         the buffer managed by Image automatically
   * @ingroup Image2
   *
   * @param w matrix width
   * @param h matrix height
   * @param imgType image type
   * @param dt data type(DT_8U(unsigned char)/DT_32F(float)/etc)
   * @param sm how match buffer mallocated related to image type
   * @return NONE
   */
  void create(int w, int h, ImageType imgType, DataType dt,
              StoreMode sm = STORE_ALL);

  /** @brief Set real width and real height of image
   * @ingroup Image2
   *
   * @code
   *   // real height and width
   *   const int imageRealWidth = 1920;
   *   const int imageRealHeight = 1080;
   *   // stride width and height
   *   const int imageWidth = 1920;
   *   const int imageHeight = 1152; // stride with 128
   *   // yuv height = image height * 3 / 2
   *   const int yuvWidth = imageWidth;
   *   const int yuvHeight = imageHeight * 3 / 2;
   *   unsigned char* buffer = (unsigned char*)malloc(yuvWidth * yuvHeight);
   *   // Create Image object and default rotation is 0
   *   wa::Image tImage(imageWidth, imageHeight, buffer,
   *                    wa::Image::IMAGETYPE_NV21, wa::Image::DT_8U);
   *   // set rotation, eg: 90 angle
   *   tImage.setRotation(wa::Image::ROTATION_90);
   *   // if the image have stride, there need to set the real height and width
   *   of image tImage.setImageWH(imageRealWidth, imageRealHeight);
   * @endcode
   *
   * @return NONE
   */
  void setImageWH(int imgRealWidth, int imgRealHeight);

  /** @brief Set the ion fd for image buffer
   * @ingroup Image2
   *
   * @param ionFd int array for ion fd values
   * @param ionFdSize size of the int array
   *   if we have YUV buffer for image, we just have one ion fd;
   *   if we have Y/UV buffer for image, we should have two ion fd;
   *   if we have Y/U/V buffer for image, we should have three ion fd;
   *   we shoud set the ion fd for both the input and output images
   * @code
   *   // YUV buffer
   *   wa::Image inImgs(100, 50, inputYUVBuffer, wa::Image::IMAGETYPE_NV21,
   *                    wa::Image::DT_8U);
   *   int* inputIonFd = new int{5};
   *   inImgs.setIonFd(inputIonFd, 1);
   *
   *   wa::Image outImg(100, 50, outputYUVbuffer, wa::Image::IMAGETYPE_NV21,
   *                    wa::Image::DT_8U);
   *   int* outputIonFd = new int{10};
   *   outImg.setIonFd(outputIonFd, 1);
   *
   *   // Release on destruction or after the run
   *   delete inputIonFd;
   *   delete outputIonFd;
   *
   *
   *   // Y/UV buffer
   *   wa::Image inImgs(100, 50, inputYBuffer, inputUVBuffer,
   *                    wa::Image::IMAGETYPE_NV21, wa::Image::DT_8U);
   *   int* inputIonFd = new int[2]{5, 8};
   *   inImgs.setIonFd(inputIonFd, 2);
   *
   *   wa::Image outImg(100, 50, outputYbuffer, outputUVBuffer,
   *                    wa::Image::IMAGETYPE_NV21, wa::Image::DT_8U);
   *   int* outputIonFd = new int[2]{10, 15};
   *   outImg.setIonFd(outputIonFd, 2);
   *
   *   // Release on destruction or after the run
   *   delete[] inputIonFd;
   *   delete[] outputIonFd;
   * @endcode
   * @return NONE
   */
  void setIonFd(const int* ionFd, const int ionFdSize);

  /** @brief Different algorithm need different rotation
   * @ingroup Image2
   *
   * @param r [in] the value to @see Rotation enum
   * @return NONE
   */
  void setRotation(Rotation r);

  /** @brief Get image rotation, the value to @see Rotation enum
   * @ingroup Image2
   *
   * @return rotation type
   */
  Rotation rotation() const;

  /** @brief Get image type, the value to @see ImageType enum
   * @ingroup Image2
   *
   * @return image type
   */
  ImageType imageType() const;

  /** @brief Deep copy, need copied object similar with this
   * @ingroup Image2
   *
   * @param toImg copy to object
   * @return true if success, false otherwise
   */
  bool copyTo(Image& toImg);

  /** @brief Clone Image
   * @ingroup Image2
   *
   * @return Image
   */
  Image clone();

  /** @brief Get buffer pointer of yuv or rgb data
   * @ingroup Image2
   *
   * @return buffer pointer
   */
  unsigned char* data1();

  /** @brief Get buffer pointer of u or uv data
   * @ingroup Image2
   *
   * @return buffer pointer
   */
  unsigned char* data2();

  /** @brief Get buffer pointer of v data
   * @ingroup Image2
   *
   * @return buffer pointer
   */
  unsigned char* data3();

  /** @brief Get the ion fd
   * @ingroup Image2
   *
   * @return false if ion fd is empty
   */
  bool ionFd(int** ionFd, int* ionFdSize) const;

  /** @brief Get buffer size(bit)
   * @ingroup Image2
   *
   * size = w * h * channel nums * elem size(bit)
   * if YUV data split to independent Y buffer/U buffer/V buffer,
   * then U/V buffer size need to be computed by user.
   *
   * @return total buffer size(byte)
   */
  size_t total() const;

  /** @brief Get channel number of the image
   * @ingroup Image2
   *
   * eg: BGR image is 3 channels
   *
   * @return channel number
   */
  size_t channels() const;

  /** @brief Get size(byte) of all channels
   * @ingroup Image2
   *
   * eg: BGR image is 3 channels, and 8 bit(1 byte) for per channel,
   * then the size is 3
   *
   * @return channel number
   */
  size_t elemSize() const;

  /** @brief Store mode
   * @ingroup Image2
   *
   * @return store mode to @see StoreMode enum
   */
  StoreMode storeMode() const;

  /** @brief Matrix width
   * @ingroup Image2
   *
   * @return width
   */
  int width() const;

  /** @brief Matrix height
   * @ingroup Image2
   *
   * @return height
   */
  int height() const;

  /** @brief Image real width
   * @ingroup Image2
   *
   * @return real width
   */
  int realWidth() const;

  /** @brief Image real height
   * @ingroup Image2
   *
   * @return real height
   */
  int realHeight() const;

  /** @brief Get aligned mode
   * @ingroup Image2
   *
   * @return aligned mode to @see AlignedMode enum
   */
  AlignedMode alignedMode() const;

  /** @brief Whether the image buffer is empty
   * @ingroup Image2
   *
   * @return true if image buffer is empty, false otherwise
   */
  bool empty() const;

  /** @brief Whether the image is valid
   * @ingroup Image2
   *
   * @deprecated
   */
  int isValid();

  /** @brief Set image role (camera type)
   * @ingroup Image2
   *
   * @param role [in] camera type
   */
  void setExifRoleType(RoleType role);

  /** @brief Get image role
   * @ingroup Image2
   *
   * @return camera type
   */
  RoleType exifRoleType() const;

  /** @brief Set ISO of the image
   * @ingroup Image2
   *
   * @param iso [in] image sensitivity
   */
  void setExifIso(int iso);

  /** @brief ISO of the image
   * @ingroup Image2
   *
   * @return sensitivity
   */
  int exifIso();

  /** @brief Set to show the watermark or not
   * @ingroup Image2
   *
   * @param isOn [in] true to show watermark, false not to show
   */
  void setExifWatermark(bool isOn);

  /** @brief Status of showing watermark
   * @ingroup Image2
   *
   * @return show status
   */
  bool exifIsWatermarkOn();

  /** @brief Set exif timestamp information
   * @ingroup Image2
   *
   * @param timestamp the timestamp information
   */
  void setExifTimestamp(unsigned long timestamp);

  /** @brief Get exif timestamp information
   * @ingroup Image2
   *
   * @return the timestamp information value
   */
  unsigned long exifTimestamp();

  /** @brief Set exif camera fov
   * @ingroup Image2
   *
   * @param camera fov
   */
  void setExifFov(const float fov);

  /** @brief Get exif camera fov
   * @ingroup Image2
   *
   * @return image fov
   */
  float getExifFov() const;

  /** @brief Set exif camera module information
   * @ingroup Image2
   *
   * @param moduleInfo camera module information
   */
  void setExifModuleInfo(char* moduleInfo);

  /** @brief Get exif camera module information
   * @ingroup Image2
   *
   * @return camera module information
   */
  char* exifModuleInfo() const;

  /** @brief Set exif camera AE gain information
   * @ingroup Image2
   *
   * @param aeGain camera AE gain value
   * @return true if success, false otherwise
   */
  bool setExifAeGain(float aeGain);

  /** @brief Get exif camera AE gain information
   * @ingroup Image2
   *
   * @return camera AE gain value
   */
  float exifAeGain() const;

  /** @brief Set exif camera exposure time information
   * @ingroup Image2
   *
   * @param expTime camera exposure time value
   * @return true if success, false otherwise
   */
  bool setExifExpTime(float expTime);

  /** @brief Get exif camera exposure time information
   * @ingroup Image2
   *
   * @return camera exposure time
   */
  float exifExpTime() const;

  /** @brief Set exif camera exposure value information
   *  @ingroup Image2
   *
   * @param evValue camera exposure value
   * @return true if success, false otherwise
   */
  bool setExifEvValue(float evValue);

  /** @brief Get exif camera exposure value information
   *  @ingroup Image2
   *
   * @return camera exposure value
   */
  float exifEvValue() const;

 private:
  class Impl;
  Impl* p;
};  // class Image

//! @} Image2

}  // namespace wa

#endif  // IMAGE_INCLUDE_IMAGE_CC_V2_IMAGE_H_
// clang-format on
