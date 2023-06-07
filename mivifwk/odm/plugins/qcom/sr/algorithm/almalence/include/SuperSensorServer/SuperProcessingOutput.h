#ifndef __SUPER_PROCESSING_OUTPUT_H__
#define __SUPER_PROCESSING_OUTPUT_H__

#include <SuperSensorServer/Rect.h>
#include <SuperSensorServer/Size.h>
#include <SuperSensorServer/SuperParcel.h>

#include <cstdint>

namespace almalence {

class SuperProcessingOutput : public SuperParcel
{
private:
    enum State { IDLE, LOCKED, BOUND };

    State state;

    std::uint8_t *dataPtr;
    bool dataMapped;

public:
    Size size;

    static const std::uint32_t BUFFER_WIDTH = 2560;
    static const std::uint32_t BUFFER_HEIGHT = 1440;

    SuperProcessingOutput();

    virtual ~SuperProcessingOutput();

    std::uint32_t getWidth() const;

    std::uint32_t getHeight() const;

    std::uint32_t getStride() const;

    bool isDataMapped() const;

    std::uint8_t *lock();
    void unlock();

#ifdef PRINT_LOGS_ADB_DEBUG
    int frameLastWrite = 0;
    int frameLastRead = 0;
#endif
};

} // namespace almalence

#endif // __SUPER_PROCESSING_OUTPUT_H__
