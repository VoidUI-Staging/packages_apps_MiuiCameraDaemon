#ifndef __RESULTCALLBACK_H__
#define __RESULTCALLBACK_H__

#include <utils/RefBase.h>

#include <string>
#include <vector>

namespace mihal {

struct ImageFormat
{
    int32_t format;
    int32_t width;
    int32_t height;
};

struct ResultData
{
    int32_t cameraId;
    int32_t frameNumber;
    uint32_t sessionId;
    int64_t timeStampUs;
    bool isParallelCamera;
    std::vector<ImageFormat> imageForamts;
    std::string imageName;
    int32_t clientId;
    bool isSDK;
};

}; // namespace mihal

#endif // __RESULTCALLBACK_H__
