#ifndef __PROCESSINGSERVICE_H__
#define __PROCESSINGSERVICE_H__

#include <SuperSensorServer/None.h>
#include <SuperSensorServer/Service.h>
#include <SuperSensorServer/SuperCircularBuffer.h>

#include <cstdint>

#ifndef HAL_3_SDK
#include <SuperSensorServer/SuperProcessingOutput.h>
#define SuperOutputBuffer SuperProcessingOutput
#else
#include <SuperSensorServer/SuperProcessingOutputBeforeUpscaling.h>
#define SuperOutputBuffer SuperProcessingOutputBeforeUpscaling
#endif

#ifdef ENABLE_DUMPING
#include <SuperSensorServer/ImageDumper.h>
#endif

namespace almalence {

/**
 * Processing class which is intended to run on DSP processor doing heavy processing.
 */
class ProcessingService : public Service<SuperInputBuffer, SuperOutputBuffer, None>
{
private:
    Size outputSize;
    int cameraIndex;
    const int maxStab;
    const int sizeGuaranteeBorder;

    void *instance_sz;

#ifdef ENABLE_DUMPING
    ImageDumper dumper;
#endif

protected:
    virtual None composeOutputRequest(SuperInputBuffer &in);

    virtual void process(SuperInputBuffer &in, SuperOutputBuffer &out);

public:
    ProcessingService(const std::function<void()> onStart, const std::function<void()> onStop,
                      InputStream<SuperInputBuffer> *const input,
                      OutputStream<SuperOutputBuffer, None> *const output, const Size inputSizeMax,
                      const Size outputSize, const int cameraIndex,
                      const FrameNumberFunction &frameNumberFunction, const alma_config_t *config,
                      const int maxStab, const int sizeGuaranteeBorder);

    virtual ~ProcessingService();

    void dump();
};

} // namespace almalence

#endif // __PROCESSINGSERVICE_H__
