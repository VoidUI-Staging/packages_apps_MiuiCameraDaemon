#ifndef __MI_GRALLOC_MAPPER_H__
#define __MI_GRALLOC_MAPPER_H__

#include <android/hardware/graphics/mapper/4.0/IMapper.h>
#include <cutils/native_handle.h>
#include <utils/StrongPointer.h>

#include "GraBuffer.h"

namespace mihal {

// NOTE: just a shabby mapper now only used to get and set buffer metadata in hidl format
class MiGrallocMapper
{
public:
    static MiGrallocMapper *getInstance()
    {
        static MiGrallocMapper mapper;
        return &mapper;
    };

    // NOTE: copy specific buffer metadata from src to dst
    // WARNING: preassume that dst buffer have enough metadata rom space
    android::status_t copyBufferMeta(
        buffer_handle_t src, buffer_handle_t dst,
        const android::hardware::graphics::mapper::V4_0::IMapper::MetadataType &metadataType);
    android::status_t copyBufferMeta(
        GraBuffer *src, GraBuffer *dst,
        const android::hardware::graphics::mapper::V4_0::IMapper::MetadataType &metadataType);

    // NOTE: get buffer metadata in hidl format
    android::status_t get(
        buffer_handle_t buffer,
        const android::hardware::graphics::mapper::V4_0::IMapper::MetadataType &metadataType,
        android::hardware::hidl_vec<uint8_t> &vec);

    // NOTE: set buffer metadata in hidl format
    android::status_t set(
        buffer_handle_t buffer,
        const android::hardware::graphics::mapper::V4_0::IMapper::MetadataType &metadataType,
        const android::hardware::hidl_vec<uint8_t> &vec);

private:
    MiGrallocMapper();
    ~MiGrallocMapper() = default;

    android::sp<::android::hardware::graphics::mapper::V4_0::IMapper> mMapper;
};

} // namespace mihal

#endif