#ifndef __ABSTRACT_SUPERSENSOR_PROCESSOR_H__
#define __ABSTRACT_SUPERSENSOR_PROCESSOR_H__

#include <SuperSensorServer/Size.h>
#include <SuperSensorServer/SuperSensorInputBuffer.h>
#include <almashot/hvx/superzoom.h>

#include <cstdint>
#include <list>

namespace almalence {

/**
 * @private
 * SuperSensor processor base class.
 * Carries a SuperSensorInputBuffer, input & output dimensions and camera profile index.
 */
class AbstractSuperSensorProcessor
{
protected:
    int cameraIndex;
    Size sensorSize;
    Size outputSize;
    std::uint32_t outputStride;
    std::uint32_t outputScanline;
    alma_config_t *initialConfig;
    std::uint32_t sizeGuaranteeBorder;

    virtual void callSuperSensorProcess(std::uint8_t *output, std::uint8_t *outputUV,
                                        bool singleOutputBuffer, int cameraIndex,
                                        alma_config_t *config) = 0;

private:
    SuperSensorInputBuffer inputBuffer;

public:
    /**
     * @param cameraIndex Camera profile index of your device. Provided by Almalence.
     * @param sensorSize Sensor active size.
     * @param outputSize Output image size.
     * @param outputStride Output image stride.
     * @param outputScanline Output image 'vertical stride'. Y + stride*scanline = UV.
     * @param framesMax Maximum frames amount.
     * @param externalMode External mode enabled. Use to use images directly when possible.
     */
    AbstractSuperSensorProcessor(
        int cameraIndex, Size sensorSize, Size outputSize, std::uint32_t outputStride,
        std::uint32_t outputScanline, std::function<void *(std::uint32_t)> allocator,
        std::function<void(void *)> deleter,
        std::function<std::uint32_t(std::uint32_t)> functionStride,
        std::function<std::uint32_t(Size)> functionImageSize,
        std::function<std::uint32_t(Size, std::uint32_t)> functionBufferSize,
        std::uint32_t framesMax, bool externalMode, alma_config_t *config,
        std::uint32_t sizeGuarateeBorder);

    virtual ~AbstractSuperSensorProcessor();

    /**
     * @return SuperSensorInputBuffer instance of the SuperSensorProcessor
     */
    SuperSensorInputBuffer &getInputBuffer();

    /**
     * This method is for use when input is provided via SuperSensorInputBuffer directly.
     * @param output YUV output pointer.
     * @param config config containing all non-default behavior flags.
     */
    void process(std::uint8_t *output, alma_config_t *config);

    /**
     * @param output YUV output pointer.
     * @param outputUV UV output pointer.
     * @param input Input images list.
     * @param cropRect crop region which should be used for processing. If cropRect == nullptr, then
     * @param zoom is used to calculate input region for processing.
     * @param zoom Output zoom factor. If @param cropRect != nullptr then zoom value is ignored.
     * @param config config containing all non-default behavior flags.
     */
    template <class YuvImage>
    void process(std::uint8_t *output, std::uint8_t *outputUV, std::list<YuvImage> &input,
                 Rect *cropRect, double zoom, alma_config_t *config);

    /**
     * @param output YUV output pointer.
     * @param input Input images array.
     * @param inputCount Input images count.
     * @param zoom Output zoom factor.
     * @config config with processing params.
     */

    void process(std::uint8_t *output, YuvImage<Yuv888> *input, std::uint32_t inputCount,
                 double zoom, alma_config_t *config);

    /**
     * @param output YUV output pointer.
     * @param outputUV UV outputUV pointer. If UV plane is not placed just after Y.
     * @param input Input images array.
     * @param inputCount Input images count.
     * @param zoom Output zoom factor.
     * @config config with processing params.
     */

    void process(std::uint8_t *output, std::uint8_t *outputUV, YuvImage<Yuv888> *input,
                 std::uint32_t inputCount, double zoom, alma_config_t *config);

    void process8to10(std::uint16_t *output, YuvImage<Yuv888> *input, std::uint32_t inputCount,
                      double zoom, alma_config_t *config);

    /**
     * @param output YUV output pointer.
     * @param input Input images array.
     * @param inputCount Input images count.
     * @param cropRect crop region which should be used for processing.
     * @config config with processing params.
     */
    void process(std::uint8_t *output, YuvImage<Yuv888> *input, std::uint32_t inputCount,
                 Rect cropRect, alma_config_t *config);

    void process8to10(std::uint16_t *output, YuvImage<Yuv888> *input, std::uint32_t inputCount,
                      Rect cropRect, alma_config_t *config);

    void process10to10(std::uint16_t *output, YuvImage<Yuv161616> *input, std::uint32_t inputCount,
                       double zoom, alma_config_t *config);

    void process10to10(std::uint16_t *output, YuvImage<Yuv161616> *input, std::uint32_t inputCount,
                       Rect cropRect, alma_config_t *config);
};
} // namespace almalence

#endif
