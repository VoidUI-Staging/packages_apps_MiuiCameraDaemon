#ifndef __CROPSERVICE_H__
#define __CROPSERVICE_H__

#include <SuperSensorServer/DroTableProcessor.h>
#include <SuperSensorServer/Rect.h>
#include <SuperSensorServer/Service.h>
#include <SuperSensorServer/Size.h>
#include <SuperSensorServer/SuperBufferRequest.h>
#include <SuperSensorServer/SuperCircularBuffer.h>
#include <SuperSensorServer/SuperInput.h>
#include <SuperSensorServer/Yuv888.h>

namespace almalence {

/**
 * Service running on CPU, cropping the input images.
 */
class CropService : public Service<SuperInput, SuperCrop, SuperBufferRequest>
{
private:
    const Size sensorSize;
    const Size outputSize;
    const Size inputCropMaxSize;
    DroTableProcessor dro;
    uint8_t *downscale_temp;
    const int maxStab;
    const int sizeGuaranteeBorder;
    Rect calculateCrop(const bool untouched, const Size &sensorSize, const Size &inputSize,
                       const Size &outputSize, const double zoom, const bool digitalES);

protected:
    virtual SuperBufferRequest composeOutputRequest(SuperInput &in);

    virtual void process(SuperInput &in, SuperCrop &out);

public:
    CropService(const std::function<void()> onStart, const std::function<void()> onStop,
                InputStream<SuperInput> *const input,
                OutputStream<SuperCrop, SuperBufferRequest> *const output, const Size &sensorSize,
                const Size &outputSize, const Size &inputCropMaxSize, const std::uint32_t maxStab,
                const std::uint32_t sizeGuaranteeBorder);

    virtual ~CropService();
};

} // namespace almalence

#endif