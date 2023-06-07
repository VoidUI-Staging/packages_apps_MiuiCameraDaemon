#include "MiGrallocMapper.h"

#include "LogUtil.h"

namespace mihal {

using ::android::hardware::graphics::mapper::V4_0::IMapper;
using MetadataType = android::hardware::graphics::mapper::V4_0::IMapper::MetadataType;
using android::status_t;
using android::hardware::hidl_vec;
using android::hardware::graphics::mapper::V4_0::Error;

MiGrallocMapper::MiGrallocMapper()
{
    mMapper = IMapper::getService();
    if (mMapper == nullptr) {
        MLOGE(LogGroupHAL, "[MiGrallocMapper]: get vendor mapper FAIL!!");
        return;
    }
    if (mMapper->isRemote()) {
        MLOGE(LogGroupHAL, "[MiGrallocMapper]: gralloc-mapper must be in passthrough mode!!!");
        MASSERT(false);
    }
}

status_t MiGrallocMapper::copyBufferMeta(GraBuffer *src, GraBuffer *dst,
                                         const MetadataType &metadataType)
{
    return copyBufferMeta(src->getHandle(), dst->getHandle(), metadataType);
}

status_t MiGrallocMapper::copyBufferMeta(buffer_handle_t src, buffer_handle_t dst,
                                         const MetadataType &metadataType)
{
    // NOTE: some sanity checks
    if (!mMapper) {
        MLOGE(LogGroupHAL, "[MiGrallocMapper]: mapper impl is NULL, cannot copy buffer meta");
        return -EINVAL;
    }

    if (src == nullptr || dst == nullptr) {
        MLOGE(LogGroupHAL, "[MiGrallocMapper]: input buffer NULL");
        return -EINVAL;
    }

    // NOTE: read src buffer metadata
    hidl_vec<uint8_t> vec;
    auto status = get(src, metadataType, vec);
    if (status != 0) {
        MLOGE(LogGroupHAL, "[MiGrallocMapper]: mapper get buffer meta FAIL");
        return -EINVAL;
    }

    // NOTE: set meta into dst buffer metadata
    status = set(dst, metadataType, vec);
    if (status != 0) {
        MLOGE(LogGroupHAL, "[MiGrallocMapper]: mapper set buffer meta FAIL");
        return -EINVAL;
    }

    return 0;
}

status_t MiGrallocMapper::get(buffer_handle_t buffer, const MetadataType &metadataType,
                              hidl_vec<uint8_t> &vec)
{
    // NOTE: some sanity checks
    if (!mMapper) {
        MLOGE(LogGroupHAL, "[MiGrallocMapper]: mapper impl is NULL, cannot get buffer meta");
        return -EINVAL;
    }

    if (buffer == nullptr) {
        MLOGE(LogGroupHAL, "[MiGrallocMapper]: buffer is NULL, cannot get buffer meta");
        return -EINVAL;
    }

    Error error = Error::NONE;
    auto ret = mMapper->get(const_cast<native_handle_t *>(buffer), metadataType,
                            [&](const auto &tmpError, const hidl_vec<uint8_t> &tmpVec) {
                                error = tmpError;
                                vec = tmpVec;
                            });

    if (!ret.isOk()) {
        error = Error::NO_RESOURCES;
    }

    if (error != Error::NONE) {
        MLOGE(LogGroupHAL, "[MiGrallocMapper]: get(%s, %" PRIu64 ", ...) failed with %d",
              metadataType.name.c_str(), metadataType.value, error);
    }

    return static_cast<status_t>(error);
}

status_t MiGrallocMapper::set(buffer_handle_t buffer, const MetadataType &metadataType,
                              const hidl_vec<uint8_t> &vec)
{
    // NOTE: some sanity checks
    if (!mMapper) {
        MLOGE(LogGroupHAL, "[MiGrallocMapper]: mapper impl is NULL, cannot set buffer meta");
        return -EINVAL;
    }

    if (buffer == nullptr) {
        MLOGE(LogGroupHAL, "[MiGrallocMapper]: buffer is NULL, cannot set buffer meta");
        return -EINVAL;
    }

    auto ret = mMapper->set(const_cast<native_handle_t *>(buffer), metadataType, vec);

    auto error = ret.withDefault(Error::NO_RESOURCES);
    switch (error) {
    case Error::BAD_DESCRIPTOR:
    case Error::BAD_BUFFER:
    case Error::BAD_VALUE:
    case Error::NO_RESOURCES:
        MLOGE(LogGroupHAL, "[MiGrallocMapper]: set(%s, %" PRIu64 ", ...) failed with %d",
              metadataType.name.c_str(), metadataType.value, error);
        break;
    // NOTE: It is not an error to attempt to set metadata that a particular gralloc implementation
    // happens to not support.
    case Error::UNSUPPORTED:
    case Error::NONE:
        break;
    }

    return static_cast<status_t>(error);
}

} // namespace mihal
