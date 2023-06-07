// Copyright 2017 XiaoMi. All Rights Reserved.
// Author: pengyongyi@xiaomi.com (Yongyi PENG)

#ifndef BOKEH_BOKEH_H_
#define BOKEH_BOKEH_H_

#include "bokeh_conf.h"

#define MIBOKEH_SO_VERSION "2021_1217_1930"

namespace bokeh {

class MiBokeh;

// 用于创建 MiBokeh 实例
class MiBokehFactory
{
public:
    // 创建新的 MiBokeh 实例并返回。返回的实例需要调用者释放，建议使用 std::unique_prt.
    static MiBokeh *NewBokeh();
    // 创建 BokehConfLoader 实例，加载 filename 指定的配置文件并返回。返回的 BokehConfLoader
    // 实例不需要释放。
    static const BokehConfLoader *LoadConf(const char *filename);

    static const BokehConfLoader *LoadConf();

private:
    MiBokehFactory(); // not implemented.
};

// 背景虚化的主要 API。非线程安全。
class MiBokeh
{
public:
    virtual ~MiBokeh() {}
    // 初始化 MiBokeh，可以多次调用，相同的 new_conf 重复调用的开销较小。model_path 目前没有使用。
    // new_conf 请参考 bokeh_conf.h 中的注释
    virtual int Init(const BokehConf &new_conf, const char *model_path) = 0;
    // 在输入图像的分辨率在 init 时没有传入的情况下，在调用 Bokeh
    // 函数前调用，更新输入分辨率的图片朝向 输入格式为 NV21，要求 y plane 和 uv plane 连续且 stride
    // 相等，uv_offset 为两者起始地址相差的字节数
    virtual int UpdateSizeIn(int width, int height, int rotate_angle, int uv_offset, int stride,
                             bool flip) = 0;

    // 对 img 进行虚化，结果保存在 to_img。Bokeh 函数完成了虚化所有的工作。
    // 如果需要分阶段执行，与其它算法（如美颜）并行，请分别调用 Prepare, Segment, BlurAndBlend
    virtual void Bokeh(uint8_t *img, uint8_t *to_img) = 0;
    // 输入图像
    virtual void Prepare(uint8_t *img) = 0;
    // 对图像分割，生成分割通道
    virtual void Segment() = 0;
    // 使用 Prepare 输入的分割结果，来对 img 虚化背景并融合前景，输出 bokeh 的最终结果。
    // 要求 img 的分辨率与 Prepare 输入的图像相同。 在同一张输入图像上使用不同虚化参数使用这个函数。
    virtual void BlurAndBlend(uint8_t *img, uint8_t *to_img) = 0;
    // 功能与 BlurAndBlend 相同，只能用于 NV21 格式，输入分辨率可以与 Prepare 的输入不同。
    // to_img 是用于虚化的输入，也保存虚化后的结果。
    virtual void BlurAndBlendNv21(int width, int height, int uv_offset, int stride,
                                  uint8_t *to_img) = 0;

    // 功能与 BlurAndBlendNv21 类似，但使用 Prepare 的图像虚化作为背景，to_img 用于提取前景。
    // 只能用于 NV21 格式，输入分辨率可以与 Prepare 的输入不同。to_img
    // 是用于前景的输入，也保存虚化后的结果。
    virtual void BlendNv21(int width, int height, int uv_offset, int stride, uint8_t *to_img) = 0;

    // 功能相当于 Prepare, Segment, Blur
    virtual void SegAndBlur(uint8_t *img) = 0;
    // 功能相当于 Segment, Blur
    virtual void SegAndBlur() = 0;
    // 与 BlendNv21 类似，但要求分辨率与 Prepare 输入图像相同
    virtual void Blend(uint8_t *img, uint8_t *to_img) = 0;
    // 与 Blend 类似，要求分辨率与 Prepare 输入图像相同，当 Prepare 输入为 NV21，img / to_img 格式为
    // rgb 时使用。
    virtual void BlendRgb(uint8_t *img, uint8_t *to_img) = 0;

    virtual void GetSceneCategory(int *sceneCategory, int *sceneSequence) = 0;
    virtual void SetSceneCategory(int sceneCategory, int sceneSequence) = 0;

    // 切换模型
    virtual void ChangeModel(BokehModelShape modelShape, BokehConf &new_conf) = 0;
    virtual void DisableBlur() = 0;
    virtual void EnableBlur() = 0;
    // [[deprecated]]
    virtual void UpdateScene(int64_t timestamp = 0) = 0;
    virtual void GetDepthImage(void *depthImage, bool flip) = 0;
    // [[deprecated]]
    virtual void AiSceneToSceneCategory(int sceneflag, int confident) = 0;
    virtual void StartFunction() = 0;
    virtual void EndFunction() = 0;
    virtual void UpdateFlip(int flip) = 0;
    virtual void GetOriginDepthImage(void *depthImage, bool flip) = 0;
    virtual void GetDepthImage(void *depthImage, uint8_t *src, int width, int height,
                               int rotate_angle, int uv_offset, int stride, bool flip) = 0;
    virtual void UpdateAperature(float aperature) = 0;
    virtual void DisableClassify() = 0;
    virtual void EnableClassify() = 0;
    virtual void GetMiDepthImage(void *depthImage, bool flip) = 0;
    virtual void GetRawDepthImage(void *depthImage, bool flip) = 0;
    virtual void Bokeh(void *pDisparityData, MiBufferPtr pMainImg, float strength, int category,
                       MiBufferPtr pDstImg) = 0;
    virtual int SetAiScenePreview(int category, int64_t timestamp = 0) = 0;
    virtual int SyncAiSceneCapture(int64_t timestamp = 0) = 0;
    virtual void SetExposureTime(float exposure_time = 0.0001) = 0;

    /**
     * 对 img 进行人像分割后做效果增强。 比如人像留色功能。检测到人像才进行效果增强，否则返回原图
     * @param img 输入nv21格式的原图像
     * @param to_img 输出nv21格式增强后的图像
     * @return 执行状态： 0,正常; 1,图片中无人像; 2,滤镜分类设置不对; 3,分割失败
     */
    virtual int SegAndEnhance(uint8_t *img, uint8_t *to_img) = 0;

    /**
     * 动态更新滤镜的强度。 必须在SegAndEnhance之后调用
     * @param filter_strength 滤镜增强的强度。 范围0~1
     * @param img 输入的nv21格式原图.
     * @param to_img 返回的nv21格式 新效果图.
     */
    virtual void UpdateEnhance(float filter_strength, uint8_t *img, uint8_t *to_img) = 0;
    virtual void SetMode(VideoMode mode) = 0;
    virtual void UpdateDecoAperature(float aperature) = 0;
    virtual void GetResizeDepthImage(void *depthImage, bool flip) = 0;
    virtual void SegmentRbg(int cols, int rows, uint8_t *src_img, uint8_t *to_img) = 0;
    virtual void SegAndBlend(int width, int height, uint8_t *img, uint8_t *to_img) = 0;
    virtual void setStylizeParam(StylizeProcParam &param) = 0;
};

void dumpYuvBuffer(const char *suffix, uint8_t *img, int width, int height, int uv_offset,
                   int stride, bool flip, int orientation, bool switchRB);
void dumpYuvBuffer(const char *prefix, const char *suffix, uint8_t *img, int width, int height,
                   int uv_offset, int stride, bool flip, int orientation, bool switchRB);

// id photo
void pushData(int width, int height, int rotate_angle, int uv_offset, int stride, bool flip,
              uint8_t *img);
void getData(int width, int height, int uv_offset, int stride, uint8_t *src_img, uint8_t *to_img);

void setMode(VideoMode mode);
int initVideoMibokeh(void **p);
int destroyVideoMibokeh(void **p);

// ai filter
void Filter(MIBOKEH_FILTER_DEPTHINFO *pDisparityData, MibokehFilterPara *filterPara);

} // namespace bokeh

#endif // BOKEH_BOKEH_H_
