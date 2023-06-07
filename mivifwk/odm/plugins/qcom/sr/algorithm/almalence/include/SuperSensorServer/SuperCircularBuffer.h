#ifndef __SUPER_CIRCULAR_BUFFER_H__
#define __SUPER_CIRCULAR_BUFFER_H__

#include <SuperSensorServer/Lock.h>
#include <SuperSensorServer/Rect.h>
#include <SuperSensorServer/Size.h>
#include <SuperSensorServer/SuperBufferRequest.h>
#include <SuperSensorServer/SuperParcel.h>
#include <SuperSensorServer/SuperSensorUtil.h>
#include <SuperSensorServer/SyncedMediator.h>

#include <memory>
#include <tuple>

namespace almalence {

/**
 * SuperSensor process parameters.
 */
class SuperParametrization
{
public:
    Size size;
    Rect crop;

    float zoom;

    bool borderValid;

    int sensitivity;

    bool digitalIS;

    bool scaled;

    bool processingEnabled;

    float droGammaCorrected = 0.0;

    alma_config_t config[MAX_CONFIG_SIZE];

    std::int32_t droLookupTable[3][3][256];
};

/**
 * SuperCircularBuffer input type (written by the previous stage).
 */
class SuperCrop : public SuperParcel
{
public:
    SuperParametrization parametrization;

    // Pointer to buffer from SuperCircularBuffer shifted to position where new image can be
    // written.
    std::uint8_t *buffer;
};

/**
 * SuperCircularBuffer output type (read by the next stage).
 */
class SuperInputBuffer : public SuperParcel
{
public:
    SuperParametrization parametrization;

    // Pointer to buffer from SuperCircularBuffer shifted to position from where input images for SS
    // processing can be read.
    std::uint8_t *buffer;

    std::uint32_t frameCount;
    std::uint32_t frameReference;
};

/**
 * SyncMediator implementation controlling slots in a circular buffers that can be simultaneously
 * read and written. The width of the write window is defined by skip frames constant.
 */
class SuperCircularBuffer : public SyncedMediator<SuperCrop, SuperInputBuffer, SuperBufferRequest>
{
private:
    FrameNumberFunction frameNumberFunction;
    std::uint32_t capacity;

    std::uint8_t *buffer;

    bool inputLocked;
    std::uint32_t inputLockIndex; // Based on inputWritten index, shows how much images we need to
                                  // skeep after frameReference for writing new image.
    std::uint32_t inputWritten;   // How much new images were written. It could be bigger than 1 if
                                // processing is slower than camera FPS (we have received new image
                                // before previous have been taken to processing).

    bool outputLocked;

    SuperParametrization parametrization;

    std::uint32_t frameCountMax; // Maximum possible frames for processing (depends on input image
                                 // size, zoom and SUPERSENSOR_RT_FRAMES_MAX). Example for 12mpix
                                 // images and SUPERSENSOR_RT_FRAMES_MAX=10: zoom=4 -
                                 // frameCountMax=4, zoom=12 - frameCountMax=10.
    std::uint32_t frameCount;
    std::uint32_t frameReference;

    SuperParcel parcel;

    SuperCrop input;
    SuperInputBuffer output;

    const std::uint32_t sizeGuaranteeBorder;

    void shift(); // Update frameCount and frameReference info.

protected:
    bool isItemReadyForWriting(SuperBufferRequest request);
    bool isItemReadyForReading();
    SuperCrop *acquireItemForWriting(SuperBufferRequest request);
    SuperInputBuffer *acquireItemForReading();
    void onItemWritingFinished(SuperCrop *);
    void onItemReadingFinished(SuperInputBuffer *);

public:
    SuperCircularBuffer(const Size &inputSizeMax, const FrameNumberFunction &frameNumberFunction,
                        const std::uint32_t sizeGuaranteeBorder);

    virtual ~SuperCircularBuffer();
};

} // namespace almalence

#endif // __SUPER_CIRCULAR_BUFFER_H__
