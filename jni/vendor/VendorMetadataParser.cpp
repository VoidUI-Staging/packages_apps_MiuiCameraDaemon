/*
 * Copyright (c) 2018. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include <utils/Log.h>
#include <utils/RefBase.h>
#include <camera/CameraMetadata.h>
#include <camera/VendorTagDescriptor.h>
#include <system/camera_vendor_tags.h>
#include <camera_metadata_hidden.h>
#include <string.h>
#include "include/VendorMetadataParser.h"

using namespace android;
namespace VendorMetadataParser {

const char *VErrorString[VERROR_NUM] = {
    "no error",
    "invalid param",
    "invalid tag",
    "invalid tag type",
    "no tag entry"
};

const char* getErrorMsg(int error)
{
    return VErrorString[error];
}

int getTagID(camera_metadata_t* metadata, const char *tagname, uint32_t *tagid)
{
    sp<VendorTagDescriptor> vTags;
    sp<VendorTagDescriptorCache> cache = VendorTagDescriptorCache::getGlobalVendorTagCache();
    if (cache.get()) {
        metadata_vendor_id_t vendorId = get_camera_metadata_vendor_id(metadata);
        ALOGV("vendorId=%lld", vendorId);
        cache->getVendorTagDescriptor(vendorId, &vTags);
    }

    if (android::CameraMetadata::getTagFromName(tagname, vTags.get(), tagid)) {
        ALOGE("could not find tag ID for tagname %s", tagname);
        return VERROR_INVALID_TAG;
    }

    ALOGV("tag %s corresponding tagid=%x", tagname, *tagid);
    return VOK;
}

int getTagValue(void* mdata, uint32_t tagid, void** ppdata)
{
    size_t count = 0;
    return getTagValueExt(mdata, tagid, ppdata, count);
}

int getTagValueExt(void* mdata, uint32_t tagid, void** ppdata, size_t& count)
{
    int status;
    count = 0;
    if (mdata == NULL) {
        ALOGE("null input metadata %p!", mdata);
        return VERROR_INVALID_PARAM;
    }
    ALOGV("%s for tagID %d", __func__, tagid);

    camera_metadata_t* metadata = (camera_metadata_t*) mdata;
    int tagType = get_local_camera_metadata_tag_type(tagid, metadata);
    if (tagType == -1) {
        ALOGE("Tag %d: did not have a type", tagid);
        return VERROR_INVALID_TAG_TYPE;
    }

    // find the entry
    camera_metadata_entry_t entry;
    CLEAR(entry);
    entry.tag = tagid;
    status = find_camera_metadata_entry(const_cast<camera_metadata_t*>(metadata), entry.tag, &entry);
    ALOGV("index=%d tagid=%x type=%d count=%d", entry.index, entry.tag, entry.type, entry.count);
    count = entry.count;
    if (entry.count <= 0 || status != 0) {
        ALOGE("readTagValues %d failed", tagid);
        return VERROR_NO_TAG_ENTRY;
    }

    // adapte the data
    void *pdata = NULL;
    switch(tagType) {
        case 0: //TYPE_BYTE
            ALOGV("bValue=%d", entry.data.u8[0]);
            pdata = (void *)entry.data.u8;
            break;
        case 1: //TYPE_INT32
            ALOGV("i32Value=%d", entry.data.i32[0]);
            pdata = (void *)entry.data.i32;
            break;
        case 2: //TYPE_FLOAT
            ALOGV("fValue=%.2f", entry.data.f[0]);
            pdata = (void *)entry.data.f;
            break;
        case 3: //TYPE_INT64
            ALOGV("i64Value=%lld", entry.data.i64[0]);
            pdata = (void *)entry.data.i64;
            break;
        case 4: //TYPE_DOUBLE
            ALOGV("dValue=%f", entry.data.d[0]);
            pdata = (void *)entry.data.d;
            break;
        case 5: //TYPE_RATIONAL
        default:
            break;
    }

    *ppdata = pdata;
    return VOK;
}
int getVTagValue(void* mdata, const char *tagname, void** ppdata)
{
    size_t count = 0;
    return getVTagValueExt(mdata, tagname, ppdata, count);
}
int getVTagValueExt(void* mdata, const char *tagname, void** ppdata, size_t& count)
{
    int status;
    if (mdata == NULL || tagname == NULL) {
        ALOGE("null input metadata %p or tagname %p!", mdata, tagname);
        return VERROR_INVALID_PARAM;
    }
    ALOGV("%s for tag %s", __func__, tagname);

    camera_metadata_t* metadata = (camera_metadata_t*) mdata;

    // Get tag ID
    uint32_t tagid = 0;
    status = getTagID(metadata, tagname, &tagid);
    if (status) {
        ALOGE("error %d in getTagID", status);
        return status;
    }

    // get tag value by tagid
    return getTagValueExt(mdata, tagid, ppdata, count);
}

int setVTagValue(void* mdata, const char *tagname, void *data, size_t count)
{
    int status;
    if (mdata == NULL || tagname == NULL || data == NULL) {
        ALOGE("null input metadata %p or tagname %p or data:%p!", mdata, tagname, data);
        return VERROR_INVALID_PARAM;
    }
    ALOGV("%s for tag %s data count:%d", __func__, tagname, count);

    camera_metadata_t* metadata = (camera_metadata_t*) mdata;

    // Get tag ID
    uint32_t tagid = 0;
    status = getTagID(metadata, tagname, &tagid);
    if (status) {
        ALOGE("error %d in getTagID", status);
        return status;
    }

    // find and update the entry
    camera_metadata_entry_t entry;
    CLEAR(entry);
    status = find_camera_metadata_entry(metadata, tagid, &entry);
    if (0 != status) {
        status = add_camera_metadata_entry(metadata, tagid, data, count);
    } else {
        status = update_camera_metadata_entry(metadata, entry.index, data, count, NULL);
    }

    return status;
}

int setTagValue(void* mdata, uint32_t tagid, void *data, size_t count)
{
    int status;
    if (mdata == NULL || data == NULL) {
        ALOGE("null input metadata %p or data:%p!", mdata, data);
        return VERROR_INVALID_PARAM;
    }
    ALOGV("%s for tagID %d data count:%d", __func__, tagid, count);

    camera_metadata_t* metadata = (camera_metadata_t*) mdata;

    // find and update the entry
    camera_metadata_entry_t entry;
    CLEAR(entry);
    status = find_camera_metadata_entry(metadata, tagid, &entry);
    if (0 != status) {
        status = add_camera_metadata_entry(metadata, tagid, data, count);
    } else {
        status = update_camera_metadata_entry(metadata, entry.index, data, count, NULL);
    }

    return status;
}

void* allocateCopyMetadata(void* src)
{
    if (src == NULL) {
        ALOGE("null input src metadata!");
        return NULL;
    }
    android::CameraMetadata *cMetadata = (android::CameraMetadata*)src;
    const camera_metadata_t *pMetadata = cMetadata->getAndLock();
    camera_metadata_t *pCopyMetadata = NULL;
    if (pMetadata) {
        ALOGE("get input request metadata size %d", get_camera_metadata_size(pMetadata));
        pCopyMetadata = allocate_copy_camera_metadata_checked(pMetadata, get_camera_metadata_size(pMetadata));
    } else {
        ALOGE("the input request metadata is null %p", cMetadata);
    }
    cMetadata->unlock(pMetadata);

    return (void *)pCopyMetadata;
}

void* allocateCopyInterMetadata(void* src)
{
    if (src == NULL) {
        ALOGE("null input src metadata!");
        return NULL;
    }

    const camera_metadata_t *pMetadata = (const camera_metadata_t *)src;
    ALOGE("allocateCopyInterMetadata metadata size %d", get_camera_metadata_size(pMetadata));
    camera_metadata_t *pCopyMetadata = allocate_copy_camera_metadata_checked(pMetadata, get_camera_metadata_size(pMetadata));
    return (void *)pCopyMetadata;
}

void freeMetadata(void* meta)
{
    if (meta == NULL) {
        ALOGE("null input metadata!");
        return;
    }

    free_camera_metadata((camera_metadata_t *)meta);
}

} // namespace
