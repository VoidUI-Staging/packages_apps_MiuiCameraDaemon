#ifndef __HALSUPERSENSORSERVER_H__
#define __HALSUPERSENSORSERVER_H__

#include <SuperSensorServer/SuperSensorServer.h>
#include <SuperSensorServer/common.h>
#include <almashot/hvx/superzoom.h>

namespace almalence {

class HalSuperSensorServer : public SuperSensorServer
{
private:
    const int outputStride;
    const int outputScanline;

public:
    HalSuperSensorServer(const Size &sensorSize, const Size &outputSize, const int outputStride,
                         const int outputScanline, const int cameraIndex,
                         const FrameNumberFunction &frameNumberFunction,
                         const alma_config_t *config, const std::uint32_t maxStab,
                         const std::uint32_t sizeGuaranteeBorder);

    HalSuperSensorServer(const Size &sensorSize, const Size &outputSize, const int outputStride,
                         const int outputScanline, const int cameraIndex,
                         const FrameNumberFunction &frameNumberFunction,
                         const alma_config_t *config);

    HalSuperSensorServer(const Size &sensorSize, const Size &outputSize, const int outputStride,
                         const int outputScanline, const int cameraIndex,
                         const FrameNumberFunction &frameNumberFunction);

    HalSuperSensorServer(const Size &sensorSize, const Size &outputSize, const int outputStride,
                         const int outputScanline, const int cameraIndex);

    virtual ~HalSuperSensorServer();

    int getLastProcessedImage(bool next, uint8_t *out, char *errorMessage);
};

} // namespace almalence

#endif // __HALSUPERSENSORSERVER_LIGHT_H__
