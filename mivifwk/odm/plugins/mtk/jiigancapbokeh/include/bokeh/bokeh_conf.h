// Copyright 2017 XiaoMi. All Rights Reserved.
// Author: pengyongyi@xiaomi.com (Yongyi PENG)

#ifndef BOKEH_BOKEH_CONF_H_
#define BOKEH_BOKEH_CONF_H_

#include <iomanip>

#include "bokeh_base.h"

namespace bokeh {

// 用于指定对背景进行虚化的算法
enum class BlurAlgorithm {
    FAST,     // 最快，无高级效果，用于预览
    AVG,      // 有高级效果，用于后处理
    AVG_MRGB, // 效果最好，性能较弱，用于高性能平台的后处理
    AVG_GPU,
    AVG_REAR,
    NONE = 200,
};

enum class ClassifyCategory {
    UNKNOWN,
    AUTUMN,
    CLOUD,
    FLOWER,
    NIGHTSCENE,
    PLANT,
    REDLEAF,
    BLUESKY,
    SNOW,
    SUNSET,
    WATERSIDE,
    BEACH,
    DIVING,
    POTRAIT,
    POTRAIT_KEEP,           //人脸留色滤镜
    MAX,                    // please keep it last
    INVALID_BY_MANUAL = 99, // manual set invalid
    INVALID_IN_V1 = 100,    // only mark for version 1.0.0
};

struct SceneConf
{
    SceneConf()
        : scene_category_(ClassifyCategory::INVALID_BY_MANUAL),
          scene_sequence_(0),
          filter_strength_(0.0f)
    {
    }

    SceneConf(ClassifyCategory scene_category, int scene_sequence, float filter_strength)
        : scene_category_(scene_category),
          scene_sequence_(scene_sequence),
          filter_strength_(filter_strength)
    {
    }

    ClassifyCategory scene_category_;
    int scene_sequence_;
    float filter_strength_; // 滤镜强度

    std::string ToString() const;
};

struct BlurConf
{
    BlurConf()
        : algorithm_(BlurAlgorithm::FAST),
          spot_shape_(BlurSpotShape::CIRCLE),
          max_radius_(0),
          radius_(0),
          gamma_(0.0f),
          gamma_thresh_(0.0f),
          cut_(-1),
          extension1_(-1.0f),
          extension2_(-1.0f),
          extension3_(-1.0f)
    {
    }

    BlurConf(BlurAlgorithm algorithm, BlurSpotShape spot_shape, int max_radius, int radius,
             float gamma, float gamma_thresh, int cut, float extension1, float extension2,
             float extension3)
        : algorithm_(algorithm),
          spot_shape_(spot_shape),
          max_radius_(max_radius),
          radius_(radius),
          gamma_(gamma),
          gamma_thresh_(gamma_thresh),
          cut_(cut),
          extension1_(extension1),
          extension2_(extension2),
          extension3_(extension3)
    {
    }

    BlurAlgorithm algorithm_;  // 背景虚化算法
    BlurSpotShape spot_shape_; // 光斑类型，目前没有实现
    int max_radius_; // 最大虚化半径，虚化半径只能在这个范围内变动，主要影响初始化
    int radius_;         // 虚化半径，代表了虚化的强度
    float gamma_;        // 光斑强度
    float gamma_thresh_; // 光斑密度
    int cut_;            // mask cut for blend
    float extension1_;
    float extension2_;
    float extension3_;

    std::string ToString() const;
};

enum class SegmentRuntime {
    CPU = 0,
    GPU = 1,
    DSP = 2,
    APU = 3,
};

enum class BokehModelShape {
    MODEL_64_64 = 0, // One Block
    MODEL_256_512 = 1,
    MODEL_512_512 = 2, // Three Block
    MODEL_384_768 = 3,
    MODEL_432_432 = 4,
    MODEL_512_512_4_BLOCK = 5, // D5X exclusive
    MODEL_64_64_3_BLOCK = 6,   // DSP, Three Block
    MODEL_128_512 = 7,
    MODEL_352_352 = 8, // E7S
    MODEL_128_128 = 9,
    MODEL_64_64_1_BLOCK_VERSION_3 = 10,
    MODEL_192_192 = 11,
    MODEL_256_512_V1 = 12,
    MODEL_352_352_V2 = 13, // E7S
    MODEL_128_128_4_VIDEO = 14,
    MODEL_368_368 = 15,
    MODEL_512_512_MN_V2 = 16, // mobile net
    MODEL_512_512_V4 = 17,
    MODEL_368_368_MN_V3 = 18,
    MODEL_768_768_MN_V1 = 19,
    MODEL_184_184_MN_V1 = 20,
    MODEL_512_512_V3 = 21,
    MODEL_368_368_MN_V4 = 22,
    MODEL_128_128_4_VIDEO_V1 = 23,
    MODEL_128_128_4_VIDEO_V2 = 24,
    MODEL_128_128_4_VIDEO_V3 = 25,
    MODEL_128_128_4_VIDEO_V4 = 26,
    MODEL_96_96 = 27,
    MODEL_184_184 = 28,
    MODEL_310_310 = 29,
    MODEL_280_280 = 30,
    MODEL_184_256_V1 = 31,
    MODEL_184_256_V2 = 32,
    MODEL_184_256_MN_V3 = 33,
    MODEL_396_512_MN_V2 = 34,
    MODEL_EXBISENET_512_V1 = 35,
    MODEL_MN_128_192_V1 = 36,
    MODEL_184_256_MN_V4_NO_SOFTMAX = 37,
    MODEL_MN_160_224_V1 = 38,

    MODEL_512_512_MN_V5_NO_SOFTMAX = 39,
    MODEL_184_256_MN_V4_NO_SOFTMAX_FLOAT = 40,
    MODEL_160_160_NO_SOFTMAX = 41,
    MODEL_184_256_V5 = 42,
    MODEL_384_384_V1 = 43,

    MODEL_384_768_BACK = 50,
    MODEL_512_512_BACK = 51,
    MODEL_352_352_BACK = 52,
    MODEL_512_512_BACK_MN_V3 = 53,
    MODEL_512_512_BACK_MN_V4 = 54,
    MODEL_384_384_BACK_GPU_V1 = 55,

    MODEL_512_512_FRAGMENT = 80,
    MODEL_256_256_FRAGMENT = 81,
    MODEL_256_256_FRAGMENT_NO_SOFTMAX = 82,
    MODEL_EMPTY = 100,
};

enum class BokehBlurVersion {
    BLUR_VERSION_EMPTY = 0,
    BLUR_VERSION_1 = 1,
    BLUR_VERSION_2 = 2,
    BLUR_VERSION_3 = 3,
};

struct SegmentConf
{
    SegmentConf()
        : photo_orient_(Orientation::NORMAL),
          photo_flip_(0),
          async_seg_peroid_ms_(0),
          runtime_(SegmentRuntime::GPU),
          size_seg_({0, 0}),
          mean_r_(0.0f),
          mean_g_(0.0f),
          mean_b_(0.0f),
          divider_(0.0f),
          gf_radius_(0),
          gf_eps_(0.0f),
          model_(BokehModelShape::MODEL_EMPTY),
          with_det_(true),
          with_refine_(true),
          has_portrait_th_(0.5f),
          has_portrait_area_(32 * 32)
    {
    }

    SegmentConf(Orientation photo_orient, bool photo_flip, int async_seg_peroid_ms,
                SegmentRuntime runtime, Rect size_seg, float mean_r, float mean_g, float mean_b,
                float divider, int gf_radius, float gf_eps, BokehModelShape model, bool with_det,
                bool with_refine, float portrait_th, int portrait_area)
        : photo_orient_(photo_orient),
          photo_flip_(photo_flip),
          async_seg_peroid_ms_(async_seg_peroid_ms),
          runtime_(runtime),
          size_seg_(size_seg),
          mean_r_(mean_r),
          mean_g_(mean_g),
          mean_b_(mean_b),
          divider_(divider),
          gf_radius_(gf_radius),
          gf_eps_(gf_eps),
          model_(model),
          with_det_(with_det),
          with_refine_(with_refine),
          has_portrait_th_(portrait_th),
          has_portrait_area_(portrait_area)
    {
    }

    Orientation photo_orient_; // 输入图像的朝向
    bool photo_flip_;          // 输出时是否需要将分割结果镜像翻转
    int async_seg_peroid_ms_;  // 预览异步分割的时候，分割一次的周期
    SegmentRuntime runtime_;   // 指定分割算法的计算设备
    Rect size_seg_;            // 分割模型的分辨率
    float mean_r_;             // mean of r channel
    float mean_g_;
    float mean_b_;
    float divider_; // divider of segmentation input

    int gf_radius_;         // 对分割结果进行 gf 的半径
    float gf_eps_;          // 对分割结果进行 gf 的 eps
    BokehModelShape model_; // 模型

    // SmartSeg
    bool with_det_;
    bool with_refine_;
    float has_portrait_th_;
    int has_portrait_area_;

    std::string ToString() const;
};

struct BokehConf
{
    BokehConf()
        : pixel_format_(PixelFormat::NV21),
          uv_offset_(0),
          stride_(0),
          size_max_portrait_blur_(Rect()),
          size_in_(Rect()),
          blur_conf_(BlurConf()),
          night_blur_conf_(BlurConf()),
          blur_version(BokehBlurVersion::BLUR_VERSION_EMPTY),
          scene_conf_(SceneConf()),
          seg_conf_(SegmentConf())
    {
    }

    BokehConf(PixelFormat pixel_format, int uv_offset, int stride, Rect size_max_portrait_blur,
              Rect size_in, BlurConf blur_conf, BlurConf night_blur_conf,
              BokehBlurVersion blur_version, SceneConf scene_conf, SegmentConf seg_conf,
              float reserve1, float reserve2, float reserve3, float reserve4, float reserve5,
              const char conf_name[100])
        : pixel_format_(pixel_format),
          uv_offset_(uv_offset),
          stride_(stride),
          size_max_portrait_blur_(size_max_portrait_blur),
          size_in_(size_in),
          blur_conf_(blur_conf),
          night_blur_conf_(night_blur_conf),
          blur_version(blur_version),
          scene_conf_(scene_conf),
          seg_conf_(seg_conf),
          reserve1_(reserve1),
          reserve2_(reserve2),
          reserve3_(reserve3),
          reserve4_(reserve4),
          reserve5_(reserve5)
    {
        strcpy(conf_name_, conf_name);
    }

    PixelFormat pixel_format_;    // 输入图像的格式
    int uv_offset_;               // only used for nv21 format.
    int stride_;                  // only used for nv21 format, of y and uv plane both.
    Rect size_max_portrait_blur_; // 虚化的分辨率
    Rect size_in_;                // 输入的分辨率

    BlurConf blur_conf_;
    BlurConf night_blur_conf_;
    BokehBlurVersion blur_version;
    SceneConf scene_conf_; // 滤镜类型
    SegmentConf seg_conf_;
    float reserve1_; // 保留字段1
    float reserve2_; // 保留字段2
    float reserve3_; // 保留字段3
    float reserve4_; // 保留字段4
    float reserve5_; // 保留字段5
    char conf_name_[100];

    std::string ToString() const;
};

enum class BokehConfName {
    BOKEH_PREVIEW,
    BOKEH_MOVIE,
    BOKEH_REAR,
    BOKEH_REAR_PREVIEW,
    MAX, // please keep it last
};

enum MaceInitStatus {
    MACE_SUCCESS = 0,
    MACE_INVALID_ARGS = 1, //正常拍照
    MACE_OUT_OF_RESOURCES = 2,
    MACE_INVALID_ARGS_FOR_NIGHT = 4,     //夜景模式拍照
    MACE_OUT_OF_RESOURCES_FOR_NIGHT = 8, //
    MACE_INVALID_ARGS_FOR_SCENE = 16,    //场景识别
    MACE_OUT_OF_RESOURCES_FOR_SCENE = 32,
    MACE_INVALID_ARGS_FOR_PREVIEW = 64, //预览模式
    MACE_OUT_OF_RESOURCES_FOR_PREVIEW = 128,

};

enum MaceInitStatusDelta {
    MACE_DELTA_NIGHT = 2,
    MACE_DELTA_SCENE = 4,
    MACE_DELTA_PREVIEW = 6,
};

enum VideoMode {
    MODE_DISABLE_ALL = 0,    //虚化和留色都禁用
    MODE_BOKEH_ONLY = 1,     //只开启虚化
    MODE_KEEP_ONLY = 2,      //只开启留色
    MODE_BOKEH_AND_KEEP = 3, //虚化和留色都开启
};

typedef struct _MIBOKEH_DEPTHINFO
{
    signed int lDepthDataSize;
    unsigned char *pDepthData;
} MIBOKEH_DEPTHINFO;

typedef struct _MIBOKEH_ORIGIN_DEPTHINFO
{
    signed int width;
    signed int height;
    unsigned char *pDepthData;
} MIBOKEH_ORIGIN_DEPTHINFO;

#pragma pack(push)
#pragma pack(1)
struct DispMap
{
    unsigned int from;
    signed int width;
    signed int height;
    signed int pitch;
    signed int reserve;
};
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
typedef struct _MIBOKEH_FILTER_DEPTHINFO
{
    signed int lDepthDataSize;
    DispMap dispMap;
    unsigned char *pDepthData;
} MIBOKEH_FILTER_DEPTHINFO;
#pragma pack(pop)

typedef struct _MIBOKEH_FILTER_PARA
{
    int width;
    int height;
    int uv_offset;
    int stride;
    int category;        //场景
    int filter_strength; //滤镜强度
    uint8_t *src_Img;
    uint8_t *dst_img;
    bool is_preview;
    PixelFormat format;
} MibokehFilterPara;

// 虚化配置加载器
class BokehConfLoader
{
public:
    virtual ~BokehConfLoader() {}
    // 返回 BokehConfName 对应的配置
    virtual const BokehConf &ConfByName(BokehConfName name) const = 0;
};
} // namespace bokeh
#endif // BOKEH_BOKEH_CONF_H_
