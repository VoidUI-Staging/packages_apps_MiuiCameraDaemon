#ifndef SKINBEAUTIFIER_H_
#define SKINBEAUTIFIER_H_

#include <MiaPluginWraper.h>
#include <stdint.h>

// #include "../../common/skinbeautifier/inc/BeautyShot_Image_Algorithm.h"
// #include "../../common/skinbeautifier/inc/BeautyShot_Video_Algorithm.h"

#include "../../common/skinbeautifier/inc/SkinBeautifierDefV2.h"
#include "merror.h"

typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef int32_t INT32;

#include "chioemvendortag.h"
#include "chivendortag.h"

namespace TrueSight {
class XRealityImage;
class FaceFeatures;
} // namespace TrueSight

#define CLASSICAL_FIXED_VALUE      0.8
#define NATIVE_FIXED_VALUE         0.3
#define BEAUTY_WHITEN_FIXED_VALUE  30
#define MAX_RESOURCES_PATH_LEN     256
#define MAX_FOREHEAD_POINT_NUM     40
#define RESOURCES_PATH             "/vendor/etc/camera/resources/"
#define CLASSICAL_PATH             "render/Effect/effect_mode_classical.json"
#define NATIVE_PATH                "render/Effect/effect_mode_native.json"
#define OTHER_FRONT_PATH           "render/Effect/effect_mode_otherFront.json"
#define OTHER_FRONT_CLASSICAL_PATH "render/Effect/effect_mode_classical_otherFront.json"
#define OTHER_FRONT_NATIVE_PATH    "render/Effect/effect_mode_native_otherFront.json"
#define OTHER_REAR_PATH            "render/Effect/effect_mode_otherRear.json"
#define VIDEO_FRONT_PATH           "render/Effect/effect_mode_video.json"

class SkinBeautifier
{
public:
    SkinBeautifier();
    virtual ~SkinBeautifier();
    int Init(bool isFrontCamera);
    void UnInit();
    void SetFeatureParams(BeautyFeatureParams &beautyFeatureParams, int &appModule);
    int Process(struct MiImageBuffer *input, struct MiImageBuffer *output,
                MiFDBeautyParam &faceDetectedParams, int sdkYUVMode);
    void IsFullFeature(bool flag) { this->mBeautyFullFeatureFlag = flag; };
    void SetCameraPosition(bool isFrontCamera); /// <set camera position is front
    void ResetPreviewOutput();
    void SetImageOrientation(int orientation); /// <set oriention for mialgo process
    void SetFrameInfo(FrameInfo &frameInfo);   /// <set frame info for mialgo process
    void SetBeautyDimensionEnable(bool beautyDimensionEnable)
    {
        this->mBeautyDimensionEnable = beautyDimensionEnable;
    }; /// <set BeautyDimensionEnable

private:
    /* int ProcessCapture(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output,
                       MiFDBeautyParam &faceDetectedParams);
    int ProcessPreview(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output); */
    int ProcessCapture(struct MiImageBuffer *input, struct MiImageBuffer *output,
                       MiFDBeautyParam &faceDetectedParams, int sdkYUVMode);
    int ProcessPreview(struct MiImageBuffer *input, struct MiImageBuffer *output);
    double GetCurrentSecondValue();
    void LoadBeautyResources(int beautyMode);
    int SetChangedEffectParamsValue(BeautyFeatureInfo &info, bool isSwitches, int beautyMode,
                                    bool isReset);
    void ResetAllParameters(BeautyFeatureParams &beautyFeatureParams);

    // int mWidth;
    // int mHeight;
    int mInitReady;
    bool mBeautyFullFeatureFlag;
    bool mIsFrontCamera;
    // int mfeatureCounts;
    int mImageOrientation;
    bool mIsResetFixInfo; /// <点击无美颜之后，内置固定美颜参数重置

    // bool SkinGlossRatioIsLoad;
    bool mBeautyDimensionEnable;

    // BeautyFeatureParams *m_pBeautyFeatureParams; /// <mialgo effect params needed, from APP
    // BeautyFeatureInfo *m_pFeatureParams;         /// <mialgo effect params needed, from APP

    TrueSight::XRealityImage *m_pBeautyShotCaptureAlgorithm; /// <mialgo capture pointer
    TrueSight::FaceDetector *m_pFaceDetector;                /// <mialgo face detect pointer
    // TrueSight::FaceFeatures *m_pFaceFeatures;                /// <mialgo face features pointer
    TrueSight::FrameInfo *m_pFrameInfo;                      /// <mialgo frame info pointer
};

#endif /* SKINBEAUTIFIER_H_ */
