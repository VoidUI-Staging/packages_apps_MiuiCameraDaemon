#ifndef __SUPERSENSOR_RT_PROCESSOR_H__
#define __SUPERSENSOR_RT_PROCESSOR_H__

#include <SuperSensorServer/CropService.h>
#include <SuperSensorServer/MultiBuffer.h>
#include <SuperSensorServer/Plane.h>
#include <SuperSensorServer/ProcessingService.h>
#include <SuperSensorServer/Size.h>
#include <SuperSensorServer/SuperCircularBuffer.h>
#include <SuperSensorServer/SuperSensorUtil.h>
#include <SuperSensorServer/TimeMeasurer.h>
#include <almashot/hvx/superzoom.h>

#include "SuperProcessingOutput.h"

#ifndef HAL_3_SDK
#include <SuperSensorServer/SuperProcessingOutput.h>
#define SuperOutputBuffer SuperProcessingOutput
#else
#include <SuperSensorServer/SuperProcessingOutputBeforeUpscaling.h>
#define SuperOutputBuffer SuperProcessingOutputBeforeUpscaling
#endif

namespace almalence {

/**
 * SuperSensorServer simplified class. Scaling step removed, because scaling is done during
 * SuperSensor processing (on HVX). Holds 2 services doing their part and passing the image data
 * further:
 * 1. CropService
 * 2. ProcessingService
 *
 * They pass data using one of the Mediator class implementations.
 */
class SuperSensorServer
{
private:
    static const std::uint32_t CROP_BASE = 4;
    static std::uint32_t adjust(const std::uint32_t x) { return (x / CROP_BASE) * CROP_BASE; }

    static void assertInputDimensions(const Size &size);
    static void assertOutputDimensions(const Size &size);
    void assertBorders();

    const Size sensorSize;
    const Size inputCropMaxSize;
    const std::uint32_t outputStride;

    bool alive;

    float zoomLast;

    TimeMeasurer measurerFramerate;
    TimeMeasurer measurerLatency;

protected:
    const Size outputSize;
    const std::uint32_t maxStab;
    const std::uint32_t sizeGuaranteeBorder;

    MultiBuffer<SuperInput> bufferInput;
    SuperCircularBuffer bufferCrop;
    MultiBuffer<SuperOutputBuffer> bufferOutput;

    CropService serviceCrop;
    ProcessingService serviceProcess;

    /**
     * Blocks the calling thread until the latest output to be read at least once.
     */
    void waitInputDrained();

public:
    /**
     *
     * @param sensorSize sensor size
     * @param outputSize output size
     * @param outputStride output strides
     * @param cameraIndex camera profile index within AlmaShot
     * @param frameNumberFunction function which returns the amount of frames to be used at provided
     * frame size (in pixels).
     * @param processingOnStart function which is called on start processing
     * @param processingOnStop function which is called on stop processing
     * @param egl function for processing thread to be able to synchronously readdress calls to egl
     * if graphic buffer is used.
     * @param config initial config.
     */
    SuperSensorServer(const Size &sensorSize, const Size &outputSize,
                      const std::uint32_t outputStride, const int cameraIndex,
                      const FrameNumberFunction &frameNumberFunction,
                      const std::function<void()> processingOnStart,
                      const std::function<void()> processingOnStop,
                      const std::function<void *(const std::function<void *()> &)> egl,
                      const alma_config_t *config, const std::uint32_t maxStab,
                      const std::uint32_t sizeGuaranteeBorder);
    /**
     *
     * @param sensorSize sensor size
     * @param outputSize output size
     * @param outputStride output strides
     * @param cameraIndex camera profile index within AlmaShot
     * @param frameNumberFunction function which returns the amount of frames to be used at provided
     * frame size (in pixels).
     * @param config initial config.
     */
    SuperSensorServer(const Size &sensorSize, const Size &outputSize,
                      const std::uint32_t outputStride, const int cameraIndex,
                      const FrameNumberFunction &frameNumberFunction, const alma_config_t *config);

    /**
     *
     * @param sensorSize sensor size
     * @param outputSize output size
     * @param outputStride output strides
     * @param cameraIndex camera profile index within AlmaShot
     */
    SuperSensorServer(const Size &sensorSize, const Size &outputSize,
                      const std::uint32_t outputStride, const int cameraIndex);

    /**
     *
     * @param sensorSize sensor size
     * @param outputSize output size
     * @param outputStride output strides
     * @param cameraIndex camera profile index within AlmaShot
     * @param frameNumberFunction function which returns the amount of frames to be used at provided
     * frame size (in pixels).
     * @param config initial config.
     * @param maxStab maximum number of pixels compensated by stabilization
     * @param sizeGuaranteeBorder
     */
    SuperSensorServer(const Size &sensorSize, const Size &outputSize,
                      const std::uint32_t outputStride, const int cameraIndex,
                      const FrameNumberFunction &frameNumberFunction, const alma_config_t *config,
                      const std::uint32_t maxStab, const std::uint32_t sizeGuaranteeBorder);

    virtual ~SuperSensorServer();

    /**
     * @return current frame rate.
     */
    double getFramerate() const;

    /**
     * @return current average latency. Latency is the delay between frame coming from camera and
     * SuperSensor processing being finished.
     */
    double getLatency() const;

    /**
     * Queues the frame to be processed.
     * @param sensitivity Sensitivity (ISO) of the frame.
     * @param scaled Frame scaled status.
     * @param processingEnabled Processing enabled. Pass false to use as passthrough.
     * @param superresolution SuperResolution enabled.
     * @param droGamma DRO gamma value.
     * @param NF Post noise filtering strength.
     * @param digitalIS enable digital image stabilization.
     * @param zoom Desired zoom at the moment.
     * @param inputSize Input frame size.
     * @param input Input frame.
     */
    void queue(const int sensitivity, const bool scaled, const bool processingEnabled,
               const bool superresolution, const float droGamma, const float NF, const float sharp,
               const bool digitalIS, const float zoom, const Size inputSize, const Yuv888 input);

    /**
     * Queues the frame to be processed.
     * @param sensitivity Sensitivity (ISO) of the frame.
     * @param scaled Frame scaled status.
     * @param processingEnabled Processing enabled. Pass false to use as passthrough.
     * @param droGamma DRO gamma value.
     * @param digitalIS enable digital image stabilization.
     * @param zoom Desired zoom at the moment.
     * @param inputSize Input frame size.
     * @param input Input frame.
     * @param config config with processing params.
     */
    void queue(const int sensitivity, const bool scaled, const bool processingEnabled,
               const float droGamma, const bool digitalIS, const float zoom, const Size inputSize,
               const Yuv888 input, const alma_config_t *config);

    /**
     * Locks for output read.
     * @param next if only unread frame required.
     */
    Lock<SuperOutputBuffer> lockRead(const bool next = true);

    void dump();
};

} // namespace almalence

#endif
