#ifndef __REQUEST_H__
#define __REQUEST_H__

#include "MiaPluginWraper.h"

class Request
{
public:
    Request(const ProcessRequestInfo *request) : request(request) {}
    unsigned int getInputCount() const
    {
        auto &inputBuffers = request->inputBuffersMap.begin()->second;
        return inputBuffers.size();
    }

    const ImageParams *getInput() const
    {
        auto &inputBuffers = request->inputBuffersMap.begin()->second;
        return inputBuffers.data();
    }
    const ImageParams *getOutput() const
    {
        auto &outputBuffers = request->outputBuffersMap.begin()->second;
        return outputBuffers.data();
    }
    uint32_t getframeNum() const { return request->frameNum; }

    int64_t getTimeStamp() const { return request->timeStamp; }

private:
    const ProcessRequestInfo *const request;
};

inline uint32_t getFormat(const ImageParams *buffer)
{
    return buffer->format.format;
}

inline uint32_t getWidth(const ImageParams *buffer)
{
    return buffer->format.width;
}

inline uint32_t getHeight(const ImageParams *buffer)
{
    return buffer->format.height;
}

inline uint32_t getStride(const ImageParams *buffer)
{
    return buffer->planeStride;
}

inline uint32_t getScanline(const ImageParams *buffer)
{
    return buffer->sliceheight;
}

inline uint8_t *const *getPlane(const ImageParams *buffer)
{
    return buffer->pAddr;
}

inline camera_metadata_t *getLogicalMetaDataPointer(const ImageParams *buffer)
{
    return buffer->pMetadata;
}

inline camera_metadata_t *getPhysicalMetaDataPointer(const ImageParams *buffer)
{
    return buffer->pPhyCamMetadata;
}

#endif
