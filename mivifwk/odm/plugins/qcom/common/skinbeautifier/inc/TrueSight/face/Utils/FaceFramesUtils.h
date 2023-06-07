#pragma once

#include <thread>
#include <vector>
#include <string>

#include <TrueSight/Common/Image.h>
#include <TrueSight/Utils/IFramesUtils.h>
#include <TrueSight/face/FaceFeatures.h>
#include <TrueSight/face/FaceScheduleConfig.h>

namespace TrueSight {

/**
 * 用于Debug, 保存序列帧
 */
class TrueSight_PUBLIC FaceFramesUtils : public IFramesUtils {

public:
    FaceFramesUtils();
    ~FaceFramesUtils() override;

    int SaveInput(const Image* image, const FaceFeatures *face, const FaceScheduleConfig &cfg);
    int SaveOutputFace(const Image* image, const FaceFeatures *face, const FaceScheduleConfig &cfg);
    int SaveOutput(const Image *image, const FaceFeatures *face, const FaceScheduleConfig &cfg);
};

} // namespace TrueSight