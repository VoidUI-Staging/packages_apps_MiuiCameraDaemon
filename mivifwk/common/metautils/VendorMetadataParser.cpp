/*
 * Copyright (c) 2018. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "VendorMetadataParser.h"

#include <camera_metadata_hidden.h>

#include <shared_mutex>
#include <unordered_map>

#include "CameraMetadata.h"
#include "MetaMappingHelper.hpp"
#include "MiDebugUtils.h"
#include "VendorTagDescriptor.h"

#undef LOG_TAG
#define LOG_TAG "VendorMetadataParser"

using android::hardware::camera::common::V1_0::helper::CameraMetadata;
using android::hardware::camera::common::V1_0::helper::VendorTagDescriptor;
using android::hardware::camera::common::V1_0::helper::VendorTagDescriptorCache;
using namespace midebug;

namespace VendorMetadataParser {

void setUpVendorTags(const vendor_tag_ops_t *vOps)
{
    MLOGI(Mia2LogGroupMeta, "vOps %p", vOps);
    android::sp<VendorTagDescriptor> descTags = VendorTagDescriptor::getGlobalVendorTagDescriptor();
    if (descTags.get() != NULL) {
        MLOGW(Mia2LogGroupMeta, "VendorTagDescriptor is already setup");
        return;
    }
    VendorTagDescriptor::createDescriptorFromOps(vOps, descTags);
    VendorTagDescriptor::setAsGlobalVendorTagDescriptor(descTags);
}

int getTagValue(camera_metadata_t *mdata, uint32_t tagId, void **data)
{
    int status;
    if (mdata == NULL) {
        MLOGW(Mia2LogGroupMeta, "null input metadata %p!", mdata);
        return VERROR_INVALID_PARAM;
    }

    int tagType = get_local_camera_metadata_tag_type(tagId, mdata);
    if (tagType == -1) {
        MLOGW(Mia2LogGroupMeta, "Tag %d: did not have a type", tagId);
        return VERROR_INVALID_TAG_TYPE;
    }

    // find the entry
    camera_metadata_entry_t entry;
    status = find_camera_metadata_entry(mdata, tagId, &entry);
    if (entry.count <= 0 || status != 0) {
        // MLOGW(Mia2LogGroupMeta, "readTagValues 0x%x failed", tagId);
        return VERROR_NO_TAG_ENTRY;
    }

    *data = entry.data.u8;
    return VOK;
}

int getTagValueCount(camera_metadata_t *mdata, uint32_t tagId, void **data, size_t *count)
{
    int status;
    if (mdata == NULL) {
        MLOGW(Mia2LogGroupMeta, "null input metadata %p!", mdata);
        return VERROR_INVALID_PARAM;
    }

    int tagType = get_local_camera_metadata_tag_type(tagId, mdata);
    if (tagType == -1) {
        MLOGW(Mia2LogGroupMeta, "Tag %d: did not have a type", tagId);
        return VERROR_INVALID_TAG_TYPE;
    }

    // find the entry
    camera_metadata_entry_t entry;
    status = find_camera_metadata_entry(mdata, tagId, &entry);
    if (entry.count <= 0 || status != 0) {
        MLOGW(Mia2LogGroupMeta, "readTagValues 0x%x failed", tagId);
        return VERROR_NO_TAG_ENTRY;
    }

    *count = entry.count;
    *data = entry.data.u8;
    return VOK;
}

int getVTagValue(camera_metadata_t *mdata, const char *tagName, void **data)
{
    int status;
    if (mdata == NULL || tagName == NULL) {
        MLOGW(Mia2LogGroupMeta, "null input metadata %p or tagName %p!", mdata, tagName);
        return VERROR_INVALID_PARAM;
    }

    // Get tag ID
    uint32_t tagId = 0;
    status = getTagID(tagName, &tagId);
    if (status) {
        return status;
    }

    // get tag value by tagId
    return getTagValue(mdata, tagId, data);
}

int getVTagValueCount(camera_metadata_t *mdata, const char *tagName, void **data, size_t *count)
{
    int status;
    if (mdata == NULL || tagName == NULL) {
        MLOGW(Mia2LogGroupMeta, "null input metadata %p or tagName %p!", mdata, tagName);
        return VERROR_INVALID_PARAM;
    }

    // Get tag ID
    uint32_t tagId = 0;
    status = getTagID(tagName, &tagId);
    if (status) {
        MLOGW(Mia2LogGroupMeta, "error %d in getTagID", status);
        return status;
    }

    // get tag value by tagId
    return getTagValueCount(mdata, tagId, data, count);
}

void *getTag(camera_metadata_t *mdata, const char *tagName, size_t *pCount)
{
    void *pData = NULL;
    size_t count = 0;

    getVTagValueCount(mdata, tagName, &pData, &count);

    if (pCount) {
        *pCount = count;
    }
    return pData;
}
void *getTag(camera_metadata_t *mdata, uint32_t tagId, size_t *pCount)
{
    void *pData = NULL;
    size_t count = 0;

    getTagValueCount(mdata, tagId, &pData, &count);

    if (pCount) {
        *pCount = count;
    }
    return pData;
}

int setVTagValue(camera_metadata_t *mdata, const char *tagName, void *data, size_t count)
{
    int status;
    if (mdata == NULL || tagName == NULL || data == NULL) {
        MLOGW(Mia2LogGroupMeta, "null input metadata %p or tagName %p or data:%p!", mdata, tagName,
              data);
        return VERROR_INVALID_PARAM;
    }

    // Get tag ID
    uint32_t tagId = 0;
    status = getTagID(tagName, &tagId);
    if (status) {
        MLOGW(Mia2LogGroupMeta, "error %d in getTagID", status);
        return status;
    }

    return setTagValue(mdata, tagId, data, count);
}

int setTagValue(camera_metadata_t *mdata, uint32_t tagId, void *data, size_t count)
{
    int status;
    if (mdata == NULL || data == NULL) {
        MLOGW(Mia2LogGroupMeta, "null input metadata %p or data:%p!", mdata, data);
        return VERROR_INVALID_PARAM;
    }

    // find and update the entry
    camera_metadata_entry_t entry;
    status = find_camera_metadata_entry(mdata, tagId, &entry);
    if (0 != status) {
        status = add_camera_metadata_entry(mdata, tagId, data, count);
    } else {
        status = update_camera_metadata_entry(mdata, entry.index, data, count, NULL);
    }

    return status;
}

camera_metadata_t *allocateMetaData(uint32_t entryCount, uint32_t dataCount)
{
    return allocate_camera_metadata(entryCount, dataCount);
}

camera_metadata_t *allocateCopyMetadata(camera_metadata_t *src)
{
    if (src == NULL) {
        MLOGW(Mia2LogGroupMeta, "null input src metadata!");
        return NULL;
    }

    const camera_metadata_t *metadata = (const camera_metadata_t *)src;
    camera_metadata_t *tmpMeta =
        allocate_copy_camera_metadata_checked(metadata, get_camera_metadata_size(metadata));
    if (!tmpMeta) {
        MLOGW(Mia2LogGroupMeta, "allocateCopyMetadata failed");
    }

    return tmpMeta;
}

void freeMetadata(camera_metadata_t *meta)
{
    if (meta == NULL) {
        MLOGW(Mia2LogGroupMeta, "null input metadata!");
        return;
    }

    free_camera_metadata(meta);
}

int updateVTagValue(camera_metadata_t *mdata, const char *tagName, void *data, size_t count,
                    camera_metadata_t **out)
{
    int status;
    if (mdata == NULL || tagName == NULL || data == NULL) {
        MLOGW(Mia2LogGroupMeta, "null input metadata %p or tagName %p or data:%p!", mdata, tagName,
              data);
        return VERROR_INVALID_PARAM;
    }
    MLOGD(Mia2LogGroupMeta, "%s for tag %s data count:%zu", __func__, tagName, count);

    // Get tag ID
    uint32_t tagId = 0;
    status = getTagID(tagName, &tagId);
    if (status) {
        MLOGW(Mia2LogGroupMeta, "error %d in getTagID", status);
        return status;
    }

    return updateTagValue(mdata, tagId, data, count, out);
}

int updateTagValue(camera_metadata_t *mdata, uint32_t tagID, void *data, size_t count,
                   camera_metadata_t **out)
{
    int status;
    uint32_t tagId = tagID;
    if (mdata == NULL || data == NULL) {
        MLOGW(Mia2LogGroupMeta, "null input metadata %p or data:%p!", mdata, data);
        return VERROR_INVALID_PARAM;
    }
    MLOGD(Mia2LogGroupMeta, "tag 0x%x data count:%zu", tagId, count);

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

camera_metadata_t *resetMetadata(camera_metadata *pMetadata)
{
    size_t entryCapacity = get_camera_metadata_entry_capacity(pMetadata);
    size_t dataCapacity = get_camera_metadata_data_capacity(pMetadata);
    size_t metadataSize = calculate_camera_metadata_size(entryCapacity, dataCapacity);

    // Reset the metadata to empty
    return place_camera_metadata(pMetadata, metadataSize, entryCapacity, dataCapacity);
}

int mergeMetadata(camera_metadata *dstMetadata, camera_metadata *srcMetadata)
{
    if (srcMetadata == NULL || dstMetadata == NULL) {
        MLOGW(Mia2LogGroupMeta, "input parameter error! srcMetadata:%p,dstMetadata:%p", srcMetadata,
              dstMetadata);
        return -1;
    }

    int result = 0;
    int status = 0;
    size_t totalEntries = get_camera_metadata_entry_count(srcMetadata);

    for (size_t i = 0; i < totalEntries; i++) {
        camera_metadata_entry_t srcEntry;
        camera_metadata_entry_t dstEntry;
        camera_metadata_entry_t updatedEntry;

        get_camera_metadata_entry(srcMetadata, i, &srcEntry);

        status = find_camera_metadata_entry(dstMetadata, srcEntry.tag, &dstEntry);

        if (0 != status) {
            status = add_camera_metadata_entry(dstMetadata, srcEntry.tag, srcEntry.data.i32,
                                               srcEntry.count);
        } else {
            status = update_camera_metadata_entry(dstMetadata, dstEntry.index, srcEntry.data.i32,
                                                  srcEntry.count, &updatedEntry);
        }

        if (0 != status) {
            uint32_t srcEntryCapacity = get_camera_metadata_entry_capacity(srcMetadata);
            uint32_t srcDataCapacity = get_camera_metadata_data_capacity(srcMetadata);
            uint32_t dstEntryCapacity = get_camera_metadata_entry_capacity(dstMetadata);
            uint32_t dstDataCapacity = get_camera_metadata_data_capacity(dstMetadata);
            MLOGW(Mia2LogGroupMeta,
                  "probably didn't find tag:0x%x count:%d !"
                  "srcEntryCapacity:%d srcDataCapacity:%d, dstEntryCapacity:%d dstDataCapacity:%d",
                  srcEntry.tag, srcEntry.count, srcEntryCapacity, srcDataCapacity, dstEntryCapacity,
                  dstDataCapacity);
        }
    }

    return result;
}

int eraseVendorTag(camera_metadata *meta, const char *tagName)
{
    int status;
    if (meta == NULL || tagName == NULL) {
        return VERROR_INVALID_PARAM;
    }

    // Get tag ID
    uint32_t tagId = 0;
    status = getTagID(tagName, &tagId);
    if (status) {
        return status;
    }

    camera_metadata_entry_t entry;
    status = find_camera_metadata_entry(meta, tagId, &entry);
    if (status == android::NAME_NOT_FOUND) {
        return android::OK;
    } else if (status != android::OK) {
        return status;
    }

    return delete_camera_metadata_entry(meta, entry.index);
}

void dumpMetadata(const camera_metadata *meta, const char *fileName, bool appendEnable)
{
    if ((NULL != meta) && (NULL != fileName)) {
        int fd = 0;
        if (appendEnable) {
            fd = ::open(fileName, O_RDWR | O_CREAT | O_APPEND, 0666);
        } else {
            fd = ::open(fileName, O_RDWR | O_CREAT, 0666);
        }
        if (0 <= fd) {
            dump_camera_metadata(meta, fd, 2);

            ::close(fd);
        }
    }
}

int VendorMetadataParser::getTagEntryCount(camera_metadata_t *mdata, uint32_t tagID, size_t *count)
{
    int status = 0;
    uint32_t tagId = tagID;
    *count = 0;

    if (mdata == NULL) {
        MLOGW(Mia2LogGroupMeta, "null input metadata %p!", mdata);
        return VERROR_INVALID_PARAM;
    }

    camera_metadata_entry_t entry;
    CLEAR(entry);
    status = find_camera_metadata_entry(mdata, tagId, &entry);
    if (status == 0) {
        *count = entry.count;
    }
    return status;
}

} // namespace VendorMetadataParser

DynamicMetadataWraper::DynamicMetadataWraper()
{
    MLOGI(Mia2LogGroupMeta, "default construct");

    mPresentMetadata = false;
    mPresentPhyMetadata = false;
    mMetadata = nullptr;
    mPhyMetadata = nullptr;

    uint32_t logicEntryCapacity = (uint32_t)(InnerMetadataEntryCapacity * 1.5);
    uint32_t logicDataCapacity = (uint32_t)(InnerMetadataDataCapacity * 1.5);
    uint32_t phyEntryCapacity = (uint32_t)(InnerMetadataEntryCapacity * 1.5);
    uint32_t phyDataCapacity = (uint32_t)(InnerMetadataDataCapacity * 1.5);

    mMetadata = VendorMetadataParser::allocateMetaData(logicEntryCapacity, logicDataCapacity);
    mPhyMetadata = VendorMetadataParser::allocateMetaData(phyEntryCapacity, phyDataCapacity);

    if (mMetadata != nullptr) {
        mPresentMetadata = true;
    } else {
        MLOGW(Mia2LogGroupMeta, "Failed to allocate logicMetaData");
    }

    if (mPhyMetadata != nullptr) {
        mPresentPhyMetadata = true;
    } else {
        MLOGW(Mia2LogGroupMeta, "Failed to allocate phyMetaData");
    }
}

DynamicMetadataWraper::DynamicMetadataWraper(camera_metadata_t *metadata,
                                             camera_metadata_t *phyMetadata)
{
    mPresentMetadata = false;
    mPresentPhyMetadata = false;
    mMetadata = metadata;
    mPhyMetadata = phyMetadata;

    if (metadata != nullptr) {
        mPresentMetadata = true;
    }

    if (phyMetadata != nullptr) {
        mPresentPhyMetadata = true;
    }
}

DynamicMetadataWraper::DynamicMetadataWraper(uint32_t logicEntryCapacity,
                                             uint32_t logicDataCapacity, uint32_t phyEntryCapacity,
                                             uint32_t phyDataCapacity)
{
    mPresentMetadata = false;
    mPresentPhyMetadata = false;
    mMetadata = nullptr;
    mPhyMetadata = nullptr;
    logicEntryCapacity = (uint32_t)(logicEntryCapacity * 1.5);
    logicDataCapacity = (uint32_t)(logicDataCapacity * 1.5);
    phyEntryCapacity = (uint32_t)(phyEntryCapacity * 1.5);
    phyDataCapacity = (uint32_t)(phyDataCapacity * 1.5);

    if (logicEntryCapacity && logicDataCapacity) {
        mMetadata = VendorMetadataParser::allocateMetaData(logicEntryCapacity, logicDataCapacity);
        if (mMetadata != nullptr) {
            mPresentMetadata = true;
        } else {
            MLOGW(Mia2LogGroupMeta, "Failed to allocateMetaData");
        }
    }

    if (phyEntryCapacity && phyDataCapacity) {
        mPhyMetadata = VendorMetadataParser::allocateMetaData(phyEntryCapacity, phyDataCapacity);
        if (mPhyMetadata != nullptr) {
            mPresentPhyMetadata = true;
        } else {
            MLOGW(Mia2LogGroupMeta, "Failed to allocateMetaData");
        }
    }
}

void DynamicMetadataWraper::updateMetadata(std::shared_ptr<DynamicMetadataWraper> &metadata)
{
    if (nullptr == metadata.get()) {
        MLOGI(Mia2LogGroupMeta, "metadata is null");
        return;
    }
    if (mPresentMetadata) {
        VendorMetadataParser::resetMetadata(mMetadata);
    }
    if (mPresentPhyMetadata) {
        VendorMetadataParser::resetMetadata(mPhyMetadata);
    }
    mPresentMetadata = false;
    mPresentPhyMetadata = false;

    if (nullptr != metadata->getLogicalMetadata()) {
        int result = VendorMetadataParser::mergeMetadata(mMetadata, metadata->getLogicalMetadata());
        if (result != -1) {
            mPresentMetadata = true;
        }
    }

    if (nullptr != metadata->getPhysicalMetadata()) {
        int result =
            VendorMetadataParser::mergeMetadata(mPhyMetadata, metadata->getPhysicalMetadata());
        if (result != -1) {
            mPresentPhyMetadata = true;
        }
    }
}

camera_metadata_t *DynamicMetadataWraper::getLogicalMetadata()
{
    if (mPresentMetadata) {
        return mMetadata;
    }
    return nullptr;
}

camera_metadata_t *DynamicMetadataWraper::getPhysicalMetadata()
{
    if (mPresentPhyMetadata) {
        return mPhyMetadata;
    }
    return nullptr;
}

DynamicMetadataWraper::~DynamicMetadataWraper()
{
    if (mMetadata != NULL) {
        VendorMetadataParser::freeMetadata(mMetadata);
        mMetadata = NULL;
    }

    if (mPhyMetadata != NULL) {
        VendorMetadataParser::freeMetadata(mPhyMetadata);
        mPhyMetadata = NULL;
    }
}

StaticMetadataWraper *StaticMetadataWraper::getInstance()
{
    static StaticMetadataWraper s_StaticMetadataSingleton;
    return &s_StaticMetadataSingleton;
}

void StaticMetadataWraper::setStaticMetadata(camera_metadata *staticMeta, uint32_t logicalCameraId)
{
    MLOGI(Mia2LogGroupMeta, "set mStaticInfo[%d] = %p", logicalCameraId, staticMeta);
    if (mStaticInfo.find(logicalCameraId) != mStaticInfo.end()) {
        VendorMetadataParser::freeMetadata(
            const_cast<camera_metadata *>(mStaticInfo[logicalCameraId]));
        mStaticInfo[logicalCameraId] = staticMeta;
    } else {
        mStaticInfo.insert({logicalCameraId, staticMeta});
    }
}

camera_metadata *StaticMetadataWraper::getStaticMetadata(uint32_t logicalCameraId)
{
    const camera_metadata *meta = nullptr;
    auto it = mStaticInfo.find(logicalCameraId);
    if (it != mStaticInfo.end()) {
        meta = it->second;
    } else {
        MLOGW(Mia2LogGroupMeta, "Failed to find static metadata from cameraId %u", logicalCameraId);
    }

    return const_cast<camera_metadata *>(meta);
}

StaticMetadataWraper::StaticMetadataWraper()
{
    mStaticInfo.clear();
}

StaticMetadataWraper::~StaticMetadataWraper()
{
    while (!mStaticInfo.empty()) {
        auto it = mStaticInfo.begin();
        VendorMetadataParser::freeMetadata(const_cast<camera_metadata *>(mStaticInfo[it->first]));
        mStaticInfo.erase(it);
    }
}

MiSDKInfo *MiSDKInfo::getInstance()
{
    static MiSDKInfo s_StaticMetadataSingleton;
    return &s_StaticMetadataSingleton;
}

void MiSDKInfo::setMiSDKInfo(std::string info)
{
    std::lock_guard<std::mutex> sdkInfoLock(mSDKInfoMutex);
    mSDKInfo = info;
}

std::string MiSDKInfo::getMiSDKInfo()
{
    std::lock_guard<std::mutex> sdkInfoLock(mSDKInfoMutex);
    return mSDKInfo;
}

MiSDKInfo::MiSDKInfo() {}

MiSDKInfo::~MiSDKInfo() {}
