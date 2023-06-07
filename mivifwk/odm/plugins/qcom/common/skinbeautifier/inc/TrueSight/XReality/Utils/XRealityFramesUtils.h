#pragma once

#include <thread>
#include <vector>
#include <string>
#include <map>

#include <TrueSight/Common/Image.h>
#include <TrueSight/Common/FrameInfo.h>
#include <TrueSight/Utils/IFramesUtils.h>
#include <TrueSight/face/FaceFeatures.h>
#include <TrueSight/XReality/XRealityParams.h>

namespace TrueSight {

/**
 * 用于Debug, 保存序列帧
 */
class TrueSight_PUBLIC XRealityFramesUtils : public IFramesUtils {

public:
    XRealityFramesUtils();
    ~XRealityFramesUtils() override;

    const char * GetNameBase();
    void SaveImage(const Image *image, const char *post_fix);

    int SaveInput(const Image* image, const FaceFeatures *face);
    int SaveFaceRetouchOutput(const Image *image, const FaceFeatures *face);
    int SaveOutput(const Image *image, const FaceFeatures *face);
    int SaveParams(const Image *image, const std::map<ParameterFlag, float> &paramsMap, FrameInfo &frameInfo, const std::vector<std::string> &texts = std::vector<std::string>());
};

} // namespace TrueSight