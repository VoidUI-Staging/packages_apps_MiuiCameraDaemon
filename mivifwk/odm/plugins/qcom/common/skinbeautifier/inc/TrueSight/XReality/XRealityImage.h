/**
 * XRealityImage为图片效果处理功能
 * XRealityImage provides image effect processing functions.
 */
#pragma once

#include <TrueSight/TrueSightDefine.hpp>
#include <TrueSight/Common/NonCopyable.h>
#include <TrueSight/Common/FrameInfo.h>

#include <TrueSight/face/FaceFeatures.h>
#include "XRealityParams.h"

namespace TrueSight {

class XRealityImageImpl;

/**
 * #enum 硬件加速方案：
 */
enum class  HardwareAccelerationType{
    kHardwareAccelerationType_GPU,
    kHardwareAccelerationType_NPU,
};

class TrueSight_PUBLIC XRealityImage : NonCopyable {
public:
    XRealityImage();

    ~XRealityImage();

    /**
     * step1.
     * 设置资源路径
     * To use the XRealityImage function,
     * you need to set the resource package that TrueSight SDK depends on to XRealityImage,
     * and pResPath points to the root directory of the resource package.
     */
    XRealityImage &SetResourcesPath(const char *pResPath);

    /**
     * step1.
     * 是否使用内部皮肤检测, 请在Initialize前使用, 默认关闭
     * Set whether to use internal skin detection, which must be set before Initialize, and is disabled by default.
     * @param enable : default false
     * @return
     */
    XRealityImage &SetUseInnerSkinDetect(bool enable);

    /**
     * step1.
     * 设置是否使用内部的EGL环境, 默认不使用内部环境
     * Set whether to use the internal EGL environment, the default is not to use the internal environment.
     * @param enable : default false
     * @return
     */
    XRealityImage &SetUseInnerEGLEnv(bool enable);

    /**
     * step1.
     * 是否开启E2E祛斑祛痘, 请在Initialize前使用, 默认根据配置决定, 可强行关闭
     * Whether to enable E2E freckle and acne removal must be set before Initialize.
     * The default is determined according to the configuration, but it can be forcibly turned off
     * @param enable : 默认根据配置决定, 可强行关闭
     * @return
     */
    [[deprecated("Use SetEnableAIEffect() instead.")]]
    XRealityImage &SetEnableE2EAcne(bool enable);

    /**
     * step1.
     * 是否开启ai效果初始化, 请在Initialize前使用, 默认根据配置决定, 可强行关闭
     * Whether to enable AI Effect must be set before Initialize.
     * The default is determined according to the configuration, but it can be forcibly turned off
     * @param enable : 默认根据配置决定, 可强行关闭
     * @return
     */

    XRealityImage &SetEnableAIEffect(AIEffectType type, bool enable);

    /**
     * 设置硬件加速方案, 请在Initialize前使用
     * @param type :  default kHardwareAccelerationType_GPU
     * @return
     */
    XRealityImage &SetHardwareAccelerationType(AIEffectType aiEffectType, HardwareAccelerationType type);

    /**
     * step2.
     * 初始化
     * Initialize the face detection instance. Returns 0 on success; -1 on failure.
     */
    int Initialize();

    /**
     * 可选, 与Initialize配套使用
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
     * step4.
     * @param pImageData : 支持NV21, NV12
     * @param pImageData : Support NV21, NV12
     * @param pFaceInfo
     * @param pSkinMask
     * @return
     */
    int RunCaptureImage(Image *pImageData, FaceFeatures *pFaceInfo, Image *pSkinMask);

    /**
     * step4.
     * @param pSrcImage : 支持NV21, NV12
     * @param pSrcImage : Support NV21, NV12
     * @param pDstImage : 支持NV21, NV12
     * @param pDstImage : Support NV21, NV12
     * @param pFaceInfo
     * @param pSkinMask
     * @return
     */
    int RunCaptureImage(Image *pSrcImage, Image *pDstImage, FaceFeatures *pFaceInfo, Image *pSkinMask);

    /**
     * 可选, 保存算法所需的二进制文件, 必须放在Initialize之后
     */
    void SaveBinary(const std::string &strBasePath);

private:
    XRealityImageImpl *m_Handle;
};

} //  namespace TrueSight
