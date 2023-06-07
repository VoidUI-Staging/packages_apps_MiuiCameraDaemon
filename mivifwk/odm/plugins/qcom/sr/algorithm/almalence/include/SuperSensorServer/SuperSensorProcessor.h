#ifndef __SUPERSENSOR_PROCESSOR_H__
#define __SUPERSENSOR_PROCESSOR_H__

#include <SuperSensorServer/AbstractSuperSensorProcessor.h>
#include <SuperSensorServer/Exposure.h>
#include <SuperSensorServer/SuperSensorInputBuffer.h>
#include <SuperSensorServer/SuperSensorUtil.h>
#include <SuperSensorServer/Yuv888.h>

#include <functional>
#include <list>
#include <memory>

#ifndef __ANDROID__
#define __unused
#endif

namespace almalence {

class SuperSensorProcessor : public AbstractSuperSensorProcessor
{
private:
    void *instanceSuperSensor;

protected:
    void callSuperSensorProcess(std::uint8_t *output, std::uint8_t *outputUV,
                                bool singleOutputBuffer, int cameraIndex, alma_config_t *config);

public:
    /**
     * @param cameraIndex Camera profile index of your device. Provided by Almalence.
     * @param sensorScanline Scanline of the sensor. Y + stride*scanline = UV.
     * @param outputSize Output image size.
     * @param outputStride Output image stride.
     * @param outputScanline Output image 'vertical stride'. Y + stride*scanline = UV.
     * @param framesMax Maximum frames amount.
     */
    SuperSensorProcessor(int cameraIndex, Size sensorSize, Size outputSize,
                         std::uint32_t outputStride, std::uint32_t outputScanline,
                         std::uint32_t framesMax);

    /**
     * This overload is designed to provide the same interface as CPU version.
     * sensorStride and sensorScanline arguments are ignored.
     *
     * @param cameraIndex Camera profile index of your device. Provided by Almalence.
     * @param sensorSize Sensor active size.
     * @param sensorStride Stride of the sensor.
     * @param sensorScanline Scanline of the sensor. Y + stride*scanline = UV.
     * @param outputSize Output image size.
     * @param outputStride Output image stride.
     * @param outputScanline Output image 'vertical stride'. Y + stride*scanline = UV.
     * @param framesMax Maximum frames amount.
     */
    SuperSensorProcessor(int cameraIndex, Size sensorSize, std::uint32_t sensorStride,
                         std::uint32_t sensorScanline, Size outputSize, std::uint32_t outputStride,
                         std::uint32_t outputScanline, std::uint32_t framesMax);
    /**
     * @param cameraIndex Camera profile index of your device. Provided by Almalence.
     * @param sensorSize Sensor active size.
     * @param outputSize Output image size.
     * @param outputStride Output image stride.
     * @param outputScanline Output image 'vertical stride'. Y + stride*scanline = UV.
     * @param framesMax Maximum frames amount.
     * @param config initial config.
     */
    SuperSensorProcessor(int cameraIndex, Size sensorSize, Size outputSize,
                         std::uint32_t outputStride, std::uint32_t outputScanline,
                         std::uint32_t framesMax, alma_config_t *config);

    /**
     * @param cameraIndex Camera profile index of your device. Provided by Almalence.
     * @param sensorSize Sensor active size.
     * @param outputSize Output image size.
     * @param outputStride Output image stride.
     * @param outputScanline Output image 'vertical stride'. Y + stride*scanline = UV.
     * @param framesMax Maximum frames amount.
     * @param config initial config.
     */
    SuperSensorProcessor(int cameraIndex, Size sensorSize, Size outputSize,
                         std::uint32_t outputStride, std::uint32_t outputScanline,
                         std::uint32_t framesMax, alma_config_t *config,
                         std::uint32_t sizeGuaranteeBorder);

    virtual ~SuperSensorProcessor();
};

} // namespace almalence

#endif
