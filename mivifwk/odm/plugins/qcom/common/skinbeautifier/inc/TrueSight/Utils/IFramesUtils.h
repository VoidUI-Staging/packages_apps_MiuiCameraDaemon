#pragma once

#include <thread>
#include <vector>
#include <string>

#include <TrueSight/TrueSightDefine.hpp>
#include <TrueSight/Common/Image.h>
#include <TrueSight/Common/IString.h>

namespace TrueSight {

/**
 * 用于Debug, 保存序列帧
 */
class TrueSight_PUBLIC IFramesUtils {

public:
    IFramesUtils();
    virtual ~IFramesUtils();

    /*
     * such as : save to /data/vendor/camera/truesight/
     */
    virtual int SavePrepare(const Image* image, IString dir);

protected:
    int frame_count_base_ = 0;
    std::string frame_name_base_;

    int frames_count_ = 0;
    std::string name_base_;
    std::vector<std::thread> threads_;
};

} // namespace TrueSight