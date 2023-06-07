/*===========================================================================
   Copyright (c) 2018,2020 by Tetras Technologies, Incorporated.
   All Rights Reserved.
============================================================================*/

/*===========================================================================

                           EDIT HISTORY FOR FILE

This section contains comments describing changes made to this file.
Notice that changes are listed in reverse chronological order.
============================================================================*/

#ifndef STM_COMMON_H_
#define STM_COMMON_H_ 1

#include "cv_errors.h"
#include "stm_utils.h"
///< @brief Common handle type
typedef void *stm_handle_t;

///< @brief refrence image feature
typedef void *stm_ref_feature_t;

///< @brief Error codes for API
typedef int stm_result_t;
enum {
    ///< interface could not detected face
    STM_E_NO_FACE = -11001,
    ///< feature need update
    STM_E_FEA_NEED_UPDATE = -11002,
    ///< input feature invalid
    STM_E_INVALID_INPUT_FEA = -11003,
    ///< reference pick fail
    STM_E_REF_PICK_FAIL = -11004,
    ///< full noise or extremely blur
    STM_E_INVALID_NOISE_OR_BLUR = -11005,

    ///< process be canceled
    STM_E_CANCELED = -12001
};

/// cv float type point definition
typedef struct _stm_pointf
{
    float x; ///< 点的水平方向坐标, 为浮点数
    float y; ///< 点的竖直方向坐标, 为浮点数
} stm_pointf_t;

/// @brief 矩形框
typedef struct _stm_rect
{
    int left;   ///< 矩形最左边的坐标
    int top;    ///< 矩形最上边的坐标
    int right;  ///< 矩形最右边的坐标
    int bottom; ///< 矩形最下边的坐标
} stm_rect_t;

///< @brief 性别枚举
typedef enum _stm_gender {
    FEMALE, ///< 女性
    MALE    ///< 男性
} stm_gender_t;

///< @brief 人脸属性
typedef struct _stm_attr
{
    int age;             ///< 年龄
    stm_gender_t gender; ///< 性别
} stm_attr_t;

/// @brief 模型类型
typedef enum {
    MODEL_TYPE_DETECT_HUNTER_GRAY = 0x01, ///< 检测模型
    MODEL_TYPE_ALIGN_COMMON = 0x11,       ///< 106点关键点模型
    MODEL_TYPE_ALIGN_LIPS = 0x12,         ///< 唇部关键点模型
    MODEL_TYPE_ALIGN_IRIS = 0x13,         ///< 虹膜关键点模型
    MODEL_TYPE_ALIGN_EYELID = 0x14,       ///< 眼睑关键点模型
    MODEL_TYPE_ALIGN_EYEBROW = 0x15,      ///< 眉毛关键点模型
    MODEL_TYPE_SR = 0x21,                 ///< 超分模型
    MODEL_TYPE_SEG = 0x31,                ///< 分割模型
    MODEL_TYPE_SKIN_SEG = 0x32,           ///< 皮肤分割模型
    MODEL_TYPE_REPAIR = 0x41,             ///< 修复模型
    MODEL_TYPE_FEATURE = 0x51,            ///< 人脸特征模型
    MODEL_TYPE_ATTR = 0x61,               ///< 属性模型
    MODEL_TYPE_HEADPOSE = 0x71,           ///< 脸部转角模型
    MODEL_TYPE_DENOISE = 0x81,            ///< 暗光去噪模型
    MODEL_TYPE_MULT = 0x1000              ///< 复合模型
} stm_model_type_t;

///< @brief 描述模型
typedef struct _stm_model
{
    const char *path;      ///< 模型路径
    stm_model_type_t type; ///< 模型类型
} stm_model_t;

///< @brief 图像格式
typedef enum {
    STM_IMAGE_FMT_GRAY8,  ///< 8位灰度,    单通道(<0~255>亮度)
    STM_IMAGE_FMT_BGR888, ///< 24位bgr,    3通道(<0~255>蓝|<0~255>绿|<0~255>红)
    STM_IMAGE_FMT_NV12, ///< yuv420格式, 1个连续通道存放亮度数据, UV分量表示颜色信息,
                        ///< 4个像素共用一对UV分量
    STM_IMAGE_FMT_NV21, ///< yuv420格式, 1个连续通道存放亮度数据, VU分量表示颜色信息,
                        ///< 4个像素共用一对VU分量
    STM_IMAGE_FMT_NV12_S, ///< 数据组织方式类似NV12, 但是Y通道地址和UV分量不连续(Y通道image.pY
                          ///< UV通道image.pUV)
    STM_IMAGE_FMT_NV21_S ///< 数据组织方式类似NV21, 但是Y通道地址和VU分量不连续(Y通道image.pY
                         ///< UV通道image.pUV)
} stm_image_format_t;

///< @brief 图像格式定义
typedef struct _stm_image
{
    union {
        unsigned char *data; ///< 图像数据指针
        ///< 存储地址不连续的YUV图
        struct
        {
            unsigned char *pY;
            unsigned char *pUV;
        };
    };
    int width;                 ///< 宽度(以像素为单位)
    int height;                ///< 高度(以像素为单位)
    stm_image_format_t format; ///< 图像格式
} stm_image_t;

///< @brief YUV图像格式
typedef enum {
    YUV_IMAGE_FMT_NV12, ///< yuv420格式,yyyyyyyyuvuv, 1个连续通道存放亮度数据, UV分量表示颜色信息,
                        ///< 4个像素共用一对UV分量
    YUV_IMAGE_FMT_NV21, ///< yuv420格式,yyyyyyyyvuvu, 1个连续通道存放亮度数据, VU分量表示颜色信息,
                        ///< 4个像素共用一对VU分量
} yuv_image_format_t;

///< @brief YUV图像格式定义
typedef struct _yuv_image
{
    int width;                 ///< 宽度(以像素为单位)
    int height;                ///< 高度(以像素为单位)
    yuv_image_format_t format; ///< YUV图像格式
    int strides[2];            ///< 通道数据实际占用宽度
    unsigned char *pY;
    unsigned char *pUV;
} yuv_image_t;

///< @brief 质量过滤力度
typedef enum {
    STM_QUALITY_LOW,    ///< 低过滤强度
    STM_QUALITY_MEDIUM, ///< 中过滤强度
    STM_QUALITY_HIGH    ///< 高过滤强度
} stm_quality_strength_t;

///< @brief config 参数key
enum {
    STM_CONFIG_ISO_THRESHOLD, ///< ISO 域值, 参数为浮点类型。默认阈值800,
                              ///< 高于阈值的图片将直接不做处理
    STM_CONFIG_SHARPEN_ALPHA, ///< 五官细节锐化强度, 参数为浮点类型，取值范围[0 - 1], 默认值1.0
    STM_CONFIG_SHARPEN_BETA, ///< 其他细节锐化强度, 参数为浮点类型，取值范围[0 - 1]，默认值0.4
    STM_CONFIG_SHARPEN_MOUTH, ///< 嘴巴锐化强度, 参数为浮点类型，取值范围[0 - 1]，默认值0.0
    STM_CONFIG_SHARPEN_EYE, ///< 眼睛细节锐化强度, 参数为浮点类型，取值范围[0 - 1]，默认值0.0
    STM_CONFIG_SHARPEN_EYEBROW, ///< 眉毛细节锐化强度, 参数为浮点类型，取值范围[0 - 1]，默认值0.0
    STM_CONFIG_SMOOTHNESS, ///< 修复人像与原人像融合比重, 参数为浮点类型, 取值范围[0 -
                           ///< 1]，默认值0.65, 取值越小修复后图所占比重越小, 0表示不处理
    STM_CONFIG_QUALITY_STRENGTH, ///< 人像过滤强度, 目的是过滤掉质量太差的人脸， 避免效果不协调，
                                 ///< 级别越高表示过滤越严格； 只对修复人脸个数有影响，
                                 ///< 对效果没有影响， 可选值参考{@see
                                 ///< stm_quality_strength_t}，默认值STM_QUALITY_LOW
    STM_CONFIG_MAX_FACES,     ///< 参与人像修复的最大人脸个数，可设置范围[1-10],
                              ///< 默认为10，超出范围将被截取
    STM_CONFIG_QUALITY_DEBUG, ///< 质量模块调试开关，默认0关闭，1
                              ///< 开启，开启后清晰度高也强制都进入修复>
    STM_CONFIG_BLEND_WEIGHT_MOUTH, ///< 嘴巴融合比重, 参数为浮点类型，取值范围[0 - 1]，默认值1.0
    STM_CONFIG_BLEND_WEIGHT_EYE, ///< 眼睛融合比重, 参数为浮点类型，取值范围[0 - 1]，默认值1.0
    STM_CONFIG_BLEND_WEIGHT_EYEBROW, ///< 眉毛融合比重, 参数为浮点类型，取值范围[0 - 1]，默认值1.0
    STM_CONFIG_BLEND_WEIGHT_FACE ///< 面部非五官区域融合比重, 参数为浮点类型，取值范围[0 -
                                 ///< 1]，默认值1.0
};

#define FACESR_MAX_FACE_NUM 10
#define FACESR_ROI_POS_NUM  4
#define FACESR_RESERVE_NUM  25

///< @brief 人脸检测扩展信息
struct FACESR_EXIF_INFO
{
    char version[32];
    int faces;                                          // face number
    int rects[FACESR_MAX_FACE_NUM][FACESR_ROI_POS_NUM]; // face rect (left, top, right, bottom)
    int blur[FACESR_MAX_FACE_NUM];     // 0：full_noise , 1:extremely_blur, 2: half_extremely_blur,
                                       // 3:blur, 4:sharp, 5:dark
    int isHDCrop[FACESR_MAX_FACE_NUM]; // HD process
    int reserveData[FACESR_RESERVE_NUM];
} __attribute__((packed));

#endif // STM_COMMON_H_
