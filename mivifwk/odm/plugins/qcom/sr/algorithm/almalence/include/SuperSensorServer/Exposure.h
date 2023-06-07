#ifndef __ALMALENCE_EXPOSURE_H__
#define __ALMALENCE_EXPOSURE_H__

#include <cstdint>

namespace almalence {

/**
 * Struct representing exposure as composition of sensitivity (ISO) and exposure time.
 */
struct Exposure
{
    std::uint32_t sensitivity;
    std::uint64_t exposureTime;

    /**
     * Creates an empty Exposure.
     */
    Exposure();

    /**
     * Creates Exposure based on the provided sensitivity (ISO) and exposure time.
     * @param sensitivity Sensitivity (ISO).
     * @param exposureTime Exposure time.
     */
    Exposure(const std::uint32_t sensitivity, const std::uint64_t exposureTime);

    virtual ~Exposure();
};

} // namespace almalence

#endif
