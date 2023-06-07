/* FaceDetector provides face detection functions for images.
 * Before performing face detection, obtain face detection configuration object FaceScheduleConfig through getScheduleConfig() ,
 * and configure the parameters; after configuring the parameters, execute initalize() to initialize;
 * After the initialization is successful, call detect() to perform face detection and obtain the detection result FaceFeatures. */
#pragma once
#include <string>

#include <TrueSight/TrueSightDefine.hpp>
#include <TrueSight/Common/Image.h>
#include <TrueSight/Common/NonCopyable.h>

#include "FaceScheduleConfig.h"
#include "FaceFeatures.h"

namespace TrueSight {

class FaceDetectorImpl;
class TrueSight_PUBLIC FaceDetector : NonCopyable {

public:
    FaceDetector();
    ~FaceDetector();

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

private:
    FaceDetectorImpl *impl_ = nullptr;
};

} // namespace TrueSight