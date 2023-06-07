/*
 * Copyright (c) 2018. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "VendorMetadataParser.h"

#include <camera/CameraMetadata.h>
#include <camera/VendorTagDescriptor.h>
#include <camera_metadata_hidden.h>
#include <string.h>
#include <system/camera_vendor_tags.h>
#include <utils/Log.h>
#include <utils/RefBase.h>

#include "LogUtil.h"

using namespace android;
namespace mialgo {

int getTagID(camera_metadata_t *metadata, const char *tagName, uint32_t *tagId)
{
    sp<VendorTagDescriptor> vTags;
    sp<VendorTagDescriptorCache> cache = VendorTagDescriptorCache::getGlobalVendorTagCache();
    if (cache.get()) {
        metadata_vendor_id_t vendorId = get_camera_metadata_vendor_id(metadata);
        ALOGV("vendorId=%lld", vendorId);
        cache->getVendorTagDescriptor(vendorId, &vTags);
    }

    if (android::CameraMetadata::getTagFromName(tagName, vTags.get(), tagId)) {
        MLOGW("could not find tag ID for tagName %s", tagName);
        return VERROR_INVALID_TAG;
    }

    ALOGV("tag %s corresponding tagId=%x", tagName, *tagId);
    return VOK;
}
int VendorMetadataParser::getTagValue(camera_metadata_t *metadata, uint32_t tagId, void **data)
{
    size_t count = 0;
    return getTagValueExt(metadata, tagId, data, count);
}

int VendorMetadataParser::getTagValueExt(camera_metadata_t *metadata, uint32_t tagid, void **data,
                                         size_t &count)
{
    int status = 0;
    count = 0;
    if (metadata == NULL) {
        MLOGW("null input metadata %p!", metadata);
        return VERROR_INVALID_PARAM;
    }
    ALOGV("%s for tagID %d", __func__, tagid);

    int tagType = get_local_camera_metadata_tag_type(tagid, metadata);
    if (tagType == -1) {
        MLOGW("Tag %d: did not have a type", tagid);
        return VERROR_INVALID_TAG_TYPE;
    }

    // find the entry
    camera_metadata_entry_t entry;
    CLEAR(entry);
    entry.tag = tagid;
    status = find_camera_metadata_entry(metadata, entry.tag, &entry);
    ALOGV("index=%d tagid=%x type=%d count=%d", entry.index, entry.tag, entry.type, entry.count);
    count = entry.count;
    if (entry.count <= 0 || status != 0) {
        return VERROR_NO_TAG_ENTRY;
    }

    // adapte the data
    void *localData = NULL;
    switch (tagType) {
    case 0: // TYPE_BYTE
        ALOGV("bValue=%d", entry.data.u8[0]);
        localData = (void *)entry.data.u8;
        break;
    case 1: // TYPE_INT32
        ALOGV("i32Value=%d", entry.data.i32[0]);
        localData = (void *)entry.data.i32;
        break;
    case 2: // TYPE_FLOAT
        ALOGV("fValue=%.2f", entry.data.f[0]);
        localData = (void *)entry.data.f;
        break;
    case 3: // TYPE_INT64
        ALOGV("i64Value=%lld", entry.data.i64[0]);
        localData = (void *)entry.data.i64;
        break;
    case 4: // TYPE_DOUBLE
        ALOGV("dValue=%f", entry.data.d[0]);
        localData = (void *)entry.data.d;
        break;
    case 5: // TYPE_RATIONAL
    default:
        break;
    }

    *data = localData;
    return VOK;
}

int VendorMetadataParser::getVTagValue(camera_metadata_t *metadata, const char *tagName,
                                       void **data)
{
    size_t count = 0;
    return getVTagValueExt(metadata, tagName, data, count);
}

int VendorMetadataParser::getVTagValueExt(camera_metadata_t *metadata, const char *tagName,
                                          void **data, size_t &count)
{
    int status = 0;
    if (metadata == NULL || tagName == NULL) {
        MLOGW("null input metadata %p or tagName %p!", metadata, tagName);
        return VERROR_INVALID_PARAM;
    }
    ALOGV("%s for tag %s", __func__, tagName);

    // Get tag ID
    uint32_t tagId = 0;
    status = getTagID(metadata, tagName, &tagId);
    if (status) {
        MLOGW("error %d in getTagID", status);
        return status;
    }

    // get tag value by tagId
    return getTagValueExt(metadata, tagId, data, count);
}

int VendorMetadataParser::setVTagValue(camera_metadata_t *metadata, const char *tagName, void *data,
                                       size_t count)
{
    int status = 0;
    if (metadata == NULL || tagName == NULL || data == NULL) {
        MLOGE("null input metadata %p or tagName %p or data:%p!", metadata, tagName, data);
        return VERROR_INVALID_PARAM;
    }
    ALOGV("%s for tag %s data count:%d", __func__, tagName, count);

    // Get tag ID
    uint32_t tagId = 0;
    status = getTagID(metadata, tagName, &tagId);
    if (status) {
        MLOGW("error %d in getTagID", status);
        return status;
    }

    // find and update the entry
    camera_metadata_entry_t entry;
    CLEAR(entry);
    status = find_camera_metadata_entry(metadata, tagId, &entry);
    if (0 != status) {
        status = add_camera_metadata_entry(metadata, tagId, data, count);
    } else {
        status = update_camera_metadata_entry(metadata, entry.index, data, count, NULL);
    }

    return status;
}

int VendorMetadataParser::setTagValue(camera_metadata_t *metadata, uint32_t tagId, void *data,
                                      size_t count)
{
    int status = 0;
    if (metadata == NULL || data == NULL) {
        MLOGE("null input metadata %p or data:%p!", metadata, data);
        return VERROR_INVALID_PARAM;
    }
    ALOGV("%s for tagID %d data count:%d", __func__, tagId, count);

    // find and update the entry
    camera_metadata_entry_t entry;
    CLEAR(entry);
    status = find_camera_metadata_entry(metadata, tagId, &entry);
    if (0 != status) {
        status = add_camera_metadata_entry(metadata, tagId, data, count);
    } else {
        status = update_camera_metadata_entry(metadata, entry.index, data, count, NULL);
    }

    return status;
}

int VendorMetadataParser::updateVTagValue(camera_metadata_t *mdata, const char *tagName, void *data,
                                          size_t count, camera_metadata_t **out)
{
    int status = 0;
    if (mdata == NULL || tagName == NULL || data == NULL) {
        MLOGE("null input metadata %p or tagName %p or data:%p!", mdata, tagName, data);
        return VERROR_INVALID_PARAM;
    }

    // Get tag ID
    uint32_t tagId = 0;
    status = getTagID(mdata, tagName, &tagId);
    if (status) {
        MLOGE("error %d in getTagID", status);
        return status;
    }

    return updateTagValue(mdata, tagId, data, count, out);
}

int VendorMetadataParser::updateTagValue(camera_metadata_t *mdata, uint32_t tagID, void *data,
                                         size_t count, camera_metadata_t **out)
{
    int status = 0;
    uint32_t tagId = tagID;
    if (mdata == NULL || data == NULL) {
        MLOGE("null input metadata %p or data:%p!", mdata, data);
        return VERROR_INVALID_PARAM;
    }
    MLOGD("tag 0x%x data count:%zu", tagId, count);

    CameraMetadata meta(mdata);
    uint8_t tagType = get_local_camera_metadata_tag_type(tagId, mdata);
    camera_metadata_ro_entry entry = {
        .tag = tagId,
        .type = tagType,
        .count = count,
        .data.u8 = static_cast<uint8_t *>(data),
    };
    status = meta.update(entry);
    camera_metadata_t *newRaw = meta.release();
    if (out)
        *out = newRaw;

    return status;
}

camera_metadata_t *VendorMetadataParser::allocateCopyMetadata(void *src)
{
    if (src == NULL) {
        MLOGE("null input src metadata!");
        return NULL;
    }
    android::CameraMetadata *cMetadata = (android::CameraMetadata *)src;
    const camera_metadata_t *metadataTemp = cMetadata->getAndLock();
    camera_metadata_t *copyMetadata = NULL;
    if (metadataTemp) {
        MLOGD("get input request metadata size %d", get_camera_metadata_size(metadataTemp));
        copyMetadata = allocate_copy_camera_metadata_checked(
            metadataTemp, get_camera_metadata_size(metadataTemp));
    } else {
        MLOGE("the input request metadata is null %p", cMetadata);
    }
    cMetadata->unlock(metadataTemp);

    return copyMetadata;
}

void VendorMetadataParser::freeMetadata(camera_metadata_t *meta)
{
    if (meta == NULL) {
        MLOGE("null input metadata!");
        return;
    }

    free_camera_metadata(meta);
}

} // namespace mialgo
