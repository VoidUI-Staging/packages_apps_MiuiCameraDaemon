/**
 * XReality为效果实时处理功能
 * XRealityPreview provides effects processing functions for real-time preview.
 */
#pragma once

#include <TrueSight/TrueSightDefine.hpp>
#include <TrueSight/Common/NonCopyable.h>
#include <TrueSight/Common/FrameInfo.h>

#include <TrueSight/face/FaceFeatures.h>
#include "XRealityParams.h"

#if defined(ANDROID) || defined(__ANDROID__)
#include <android/hardware_buffer.h>
#endif

namespace TrueSight {

enum class TextureType {
    kTextureType_NORMAL_RGBA,
    kTextureType_OES_NV21,
    kTextureType_OES_NV12,
};

class XRealityPreviewImpl;

class TrueSight_PUBLIC XRealityPreview : NonCopyable {
public:
    XRealityPreview();

    ~XRealityPreview();

    /**
     * step1.
     * 设置资源路径
     * To use the XRealityImage function,
     * you need to set the resource package that TrueSight SDK depends on to XRealityImage,
     * and pResPath points to the root directory of the resource package.
     */
    XRealityPreview &SetResourcesPath(const char *pResPath);

    /**
     * step1.
     * 设置是否使用内部的EGL环境, 默认不使用内部环境
     * Set whether to use the internal EGL environment, the default is not to use the internal environment.
     * @param enable : default false
     * @return
     */
    XRealityPreview &SetUseInnerEGLEnv(bool enable);

    /** step1.
     * 内部是否使用独立的线程来运算，默认开启，目前只针对AHardwareBuffer接口
     * Set whether to enable a separate thread internally for operations. For the AHardwareBuffer interface, it is enabled by default.
     * @param flag true: draw时将使用另外线程池的线程来执行, false: draw时直接在所在的调用线程执行
     * @param flag True means DrawTexture will use another thread pool thread to execute; False means DrawTexture is directly executed on the calling thread where it is located.
     * @return
     */
    XRealityPreview &SetEnableStandaloneThread(bool flag = true);

    /**
     * step1.
     * 是否开启ai效果初始化, 请在Initialize前使用。
     * Whether to enable AI Effect must be set before Initialize.
     * @param enable : default false
     * @return
     */
    XRealityPreview &SetEnableAIEffect(AIEffectType type, bool enable);

    /**
     * step2.
     * 需要在GL环境下使用, 若设置了SetUseInnerEGLEnv将在Initialize中创建内部的GL环境
     * Initialize the face detection instance. Returns 0 on success; -1 on failure.
     */
    int Initialize();

    /*
     * 可选, 与Initialize配套使用
     * 需要在GL环境下使用
     * Used in conjunction with initialize, optional call.
     */
    void Release();

    /**
     * step3.
     * 需要gl环境，和SetEffectParamsValue搭配使用，效果参数配置策略
     *
     * @param type 效果参数配置策略类型 0=经典,
     *                              1=原生,
     *                              2=前置其他模式,
     *                              3=后置其他模式
     */
    [[deprecated("Use LoadEffectConfig() instead.")]]
    int LoadRetouchConfig(int retouch_type);

    /**
     * step3.1
     * 加载特效资源
     * 需要gl环境，和SetEffectParamsValue搭配使用，效果参数配置策略
     * Requires the gl environment, used in conjunction with SetEffectParamsValue to set the effect parameter configuration strategy
     *
     * @param nIndex {@link EffectIndex}
     * @param strEffectPath 效果json，为null表示删除
     *
     * eg:
     * 1. 配置 all套装1 效果，应用json内包含所有效果(10:ColorTone 20:FaceRetouch 30:BasicRetouch 40:FaceStereo 50:SkinTone 60:Makeup 70:FacialRefine)：
     * 1. Configure all set 1 effects, all effects are included in the application json(10:ColorTone 20:FaceRetouch 30:BasicRetouch 40:FaceStereo 50:SkinTone 60:Makeup 70:FacialRefine)：
     *    LoadEffectConfig(0, "xxx/resources/render/Effect/effect_mode_clear.json");
     * 2. 替换 60：妆容 效果，仅替换all内的妆容：
     * 2. Replace 60: Makeup effect, only replace the makeup in all:
     *    LoadEffectConfig(60, "xxx/resources/render/Effect/60_Makeup/mode_3D.json");
     * 3. 替换 all套装2 效果，所有效果替换成套装2：
     * 3. Replace all set 2 effects and replace all effects with set 2:
     *    LoadEffectConfig(0, "xxx/resources/render/Effect/effect_mode_3D.json");
     * 4. 删除 all 效果，删除当前所有效果：
     * 4. Delete all effects, delete all current effects:
     *    LoadEffectConfig(0, null);
     *
     * @return
     */
    int LoadEffectConfig(EffectIndex nIndex, const char *strEffectPath);

    /**
     * step3.1
     * 卸载特效资源
     * Uninstall special effects assets
     * @param nIndex {@link EffectIndex}
     * @return
     */
    int UnloadEffectConfig(EffectIndex nIndex);

    /**
     * step3.2
     * 刷新效果
     * refresh effect
     */
    void RefreshEffectConfig();

    /**
     * step3.
     * 每一帧设置帧信息
     * Set frame information for each frame
     * @param info : iso、BV、Lux Index、ExpIndex、FlashTag、Exposure、Adrc、ColorTemperature等
     * @return
     */
    int SetFrameInfo(const FrameInfo &info);

    /**
     * step3.3
     * 设置效果参数
     * Set effect parameters
     * @param fValue         : 0.0-1.0
     * @param flag           : 参数枚举
     * @return
     */
    int SetEffectParamsValue(ParameterFlag flag, float fValue);

    /**
     * step3.3
     * 关闭当前所有效果
     * Turn off all current effects
     * @return
     */
    int ResetEffectParams();

    /**
     * step3.3
     * 获取效果参数
     * Get effect parameters
     * @param flag            : 参数枚举
     * @return 0.0-1.0
     */
    float GetEffectParamsValue(ParameterFlag flag);

    /**
     * step4-preview : 此方式会增加输入输出的拷贝操作
     * step4-preview: This method will increase the copy operation of input and output
     * @param pSrcImage       : 输入CPU数据, 支持NV21/NV12/RGBA
     * @param pSrcImage       : Input CPU data, support NV21/NV12/RGBA
     * @param pDstImage       : 输出CPU数据, 支持NV21/NV12/RGBA
     * @param pDstImage       : Output CPU data, support NV21/NV12/RGBA
     * @param faceFeatures    : 输入图的人脸信息
     * @param faceFeatures    : Enter the face information of the image
     * @return
     */
    int DrawTexture(Image *pSrcImage, Image *pDstImage, FaceFeatures *faceFeatures);

    /**
     * step4-preview : 此方式会增加输入的拷贝操作(输出是纹理不可使用内部EGL环境!)
     * step4-preview : This method will increase the copy operation of the input (the output is texture and cannot use the internal EGL environment!)
     * @param pSrcImage       : 输入CPU数据, 支持NV21/NV12/RGBA
     * @param pSrcImage       : input CPU data, support NV21/NV12/RGBA
     * @param nDstTextureID   : 输出GPU纹理数据
     * @param nDstTextureID   : output GPU texture data
     * @param dstTextureType  : 输出纹理类型 -- OES纹理(NV21/NV12), NORMAL纹理(RGBA)
     * @param dstTextureType  : output texture type -- OES texture (NV21/NV12), NORMAL texture (RGBA)
     * @param faceFeatures    : 输入图的人脸信息
     * @param faceFeatures    : the face information of the input image
     * @return
     */
    int DrawTexture(Image *pSrcImage, int nDstTextureID, TextureType dstTextureType, FaceFeatures *faceFeatures);

    /**
     * step4-preview : 建议使用此方式, 由外部创建HardwareBuffer来减少拷贝(输入输出目标是纹理不可使用内部EGL环境!)
     * step4-preview : It is recommended to use this method, create HardwareBuffer externally to reduce copying (input and output targets are textures that cannot use the internal EGL environment!)
     * @param nSrcTextureID   : 输入GPU纹理数据
     * @param nSrcTextureID   : input GPU texture data
     * @param srcTextureType  : 输入纹理类型 -- OES纹理(NV21/NV12), NORMAL纹理(RGBA)
     * @param srcTextureType  : Input texture type -- OES texture (NV21/NV12), NORMAL texture (RGBA)
     * @param nDstTextureID   : 输出GPU纹理数据
     * @param nDstTextureID   : output GPU texture data
     * @param dstTextureType  : 输出纹理类型 -- OES纹理(NV21/NV12), NORMAL纹理(RGBA)
     * @param dstTextureType  : output texture type -- OES texture (NV21/NV12), NORMAL texture (RGBA)
     * @param nWidth          : 输入图宽
     * @param nWidth          : input image width
     * @param nHeight         : 输入图高
     * @param nHeight         : input image height
     * @param nOrientation    : 输入图旋转方向
     * @param nOrientation    : the rotation direction of the input image
     * @param faceFeatures    : 输入图的人脸信息
     * @param faceFeatures    : the face information of the input image
     * @param pSrcImage       : 输入图对应的CPU数据(可以为缩放后的图), 有输入性能会更高
     * @param pSrcImage       : The CPU data corresponding to the input image (it can be a zoomed image), the performance will be higher with input
     * @return
     */
    int DrawTexture(int nSrcTextureID, TextureType srcTextureType,
                    int nDstTextureID, TextureType dstTextureType,
                    int nWidth, int nHeight, int nOrientation, FaceFeatures *faceFeatures, Image *pSrcImage = nullptr);

    int DrawTexture(int nSrcTextureID, TextureType srcTextureType, int srcFbo,
                    int nDstTextureID, TextureType dstTextureType, int dstFbo,
                    int nWidth, int nHeight, int nOrientation, FaceFeatures *faceFeatures, Image *pSrcImage = nullptr);


#if defined(__ANDROID__) || defined(ANDROID)

    int DrawTexture(AHardwareBuffer *srcAHardwareBuffer, AHardwareBuffer_Desc &srcAHardwareBuffer_Desc,
                    TextureType srcTextureType,
                    AHardwareBuffer *dstAHardwareBuffer, AHardwareBuffer_Desc &dstAHardwareBuffer_Desc,
                    TextureType dstTextureType,
                    int orientation, FaceFeatures *faceFeatures, Image *pSrcImage);

#endif

    /**
     * 可选, 保存算法所需的二进制文件, 必须放在Initialize之后
     * Optional, save the binary file required by the algorithm, must be placed after Initialize
     */
    void SaveBinary(const std::string &strBasePath);

private:
    XRealityPreviewImpl *m_Handle;
};

} //  namespace TrueSight
