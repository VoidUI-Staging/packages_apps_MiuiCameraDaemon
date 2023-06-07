/*
 * Copyright (c) 2019. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef _VENDOR_METADATA_PARSER_H_
#define _VENDOR_METADATA_PARSER_H_

#include <stdint.h>
#include <stdio.h>
#include <system/camera_metadata.h>
#include <system/camera_vendor_tags.h>

#include <map>
#include <mutex>
#include <string>

// static const uint32_t MaxNumCameras = 16;  //already define in camxdef.h

///@ todo Optimize this
static const uint32_t InnerMetadataEntryCapacity = 1024;
static const uint32_t InnerMetadataDataCapacity = 256 * 1024;

namespace VendorMetadataParser {

#define CLEAR(x) memset(&(x), 0, sizeof(x))
enum {
    VOK = 0,
    VERROR_INVALID_PARAM,
    VERROR_INVALID_TAG,
    VERROR_INVALID_TAG_TYPE,
    VERROR_NO_TAG_ENTRY,
    VERROR_NUM
};

void setUpVendorTags(const vendor_tag_ops_t *vOps);
int getVTagValue(camera_metadata_t *mdata, const char *tagName, void **data);
int getTagValue(camera_metadata_t *mdata, uint32_t tagID, void **data);
int getTagValueCount(camera_metadata_t *mdata, uint32_t tagId, void **data, size_t *count);
int getVTagValueCount(camera_metadata_t *mdata, const char *tagName, void **data, size_t *count);
void *getTag(camera_metadata_t *mdata, const char *tagName, size_t *pCount = NULL);
void *getTag(camera_metadata_t *mdata, uint32_t tagId, size_t *pCount = NULL);
int setVTagValue(camera_metadata_t *mdata, const char *tagName, void *data, size_t count);
int setTagValue(camera_metadata_t *mdata, uint32_t tagID, void *data, size_t count);
camera_metadata_t *allocateMetaData(uint32_t entryCount, uint32_t dataCount);
camera_metadata_t *allocateCopyMetadata(camera_metadata_t *src);
void freeMetadata(camera_metadata_t *meta);
int updateVTagValue(camera_metadata_t *mdata, const char *tagName, void *data, size_t count,
                    camera_metadata_t **out);
int updateTagValue(camera_metadata_t *mdata, uint32_t tagID, void *data, size_t count,
                   camera_metadata_t **out);
camera_metadata_t *resetMetadata(camera_metadata *pMetadata);
int mergeMetadata(camera_metadata *dstMetadata, camera_metadata *srcMetadata);
int eraseVendorTag(camera_metadata *meta, const char *tagName);
void dumpMetadata(const camera_metadata *meta, const char *filename, bool appendEnable = false);
int getTagEntryCount(camera_metadata_t *mdata, uint32_t tagID, size_t *count);

}; // namespace VendorMetadataParser

class DynamicMetadataWraper
{
public:
    DynamicMetadataWraper();
    DynamicMetadataWraper(camera_metadata_t *metadata, camera_metadata_t *phyMetadata);
    DynamicMetadataWraper(uint32_t logicEntryCapacity, uint32_t logicDataCapacity,
                          uint32_t phyEntryCapacity, uint32_t phyDataCapacity);

    ~DynamicMetadataWraper();

    void updateMetadata(std::shared_ptr<DynamicMetadataWraper> &metadata);

    camera_metadata_t *getLogicalMetadata();
    camera_metadata_t *getPhysicalMetadata();

private:
    camera_metadata_t *mMetadata;
    camera_metadata_t *mPhyMetadata;
    bool mPresentMetadata;
    bool mPresentPhyMetadata;
};

class StaticMetadataWraper
{
public:
    static StaticMetadataWraper *getInstance();
    void setStaticMetadata(camera_metadata *staticMeta, uint32_t logicalCameraId);
    camera_metadata *getStaticMetadata(uint32_t logicalCameraId);

private:
    StaticMetadataWraper();

    ~StaticMetadataWraper();

    std::map<uint32_t, const camera_metadata_t *> mStaticInfo;
};

class MiSDKInfo
{
public:
    static MiSDKInfo *getInstance();

    void setMiSDKInfo(std::string info);
    std::string getMiSDKInfo();

private:
    MiSDKInfo();

    ~MiSDKInfo();
    std::mutex mSDKInfoMutex;

    std::string mSDKInfo;
};

#endif // _VENDOR_METADATA_PARSER_H_
