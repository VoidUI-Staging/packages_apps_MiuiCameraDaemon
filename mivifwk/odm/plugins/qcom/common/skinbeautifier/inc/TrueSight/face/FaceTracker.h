#pragma once
#include <string>

#include <TrueSight/TrueSightDefine.hpp>
#include <TrueSight/Common/Image.h>
#include <TrueSight/Common/NonCopyable.h>

#include "FaceScheduleConfig.h"
#include "FaceFeatures.h"

namespace TrueSight {

class FaceTrackerImpl;
class TrueSight_PUBLIC FaceTracker : NonCopyable {

public:
    FaceTracker();
    ~FaceTracker();

    /**
     * setp1.
     * @return
     */
    FaceScheduleConfig& getScheduleConfig();

    /**
     * step2.
     * @return
     */
    bool initialize();

    /**
     * step3.
     * @param image
     * @param feature
     * @return
     */
    int detect(const Image& image, FaceFeatures& feature);

    /**
     * 重置状态:
     * 1. 前后置切换时调用
     * 2. 中间有中断时调用
     * 3. 出现一定程度不连续序列帧时调用
     * reset the state:
     * 1. the camera switches
     * 2. SDK calls are interrupted
     * 3. Call SDK when unsequenced frame occurred
     */
    void reset();

private:
    FaceTrackerImpl *impl_ = nullptr;
};

} // namespace TrueSight