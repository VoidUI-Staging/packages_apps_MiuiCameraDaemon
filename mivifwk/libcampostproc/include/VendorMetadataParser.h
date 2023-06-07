/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef _VENDOR_METADATA_PARSER_H_
#define _VENDOR_METADATA_PARSER_H_

#include <camera/CameraMetadata.h>
#include <camera_metadata_hidden.h>
#include <stdint.h>

namespace mialgo {

#define CLEAR(x) memset(&(x), 0, sizeof(x))

enum {
    VOK = 0,
    VERROR_INVALID_PARAM,
    VERROR_INVALID_TAG,
    VERROR_INVALID_TAG_TYPE,
    VERROR_NO_TAG_ENTRY,
    VERROR_NUM
};

class VendorMetadataParser
{
public:
    static int getVTagValue(camera_metadata_t *metadata, const char *tagName, void **data);
    static int getTagValue(camera_metadata_t *metadata, uint32_t tagID, void **data);
    static int setVTagValue(camera_metadata_t *metadata, const char *tagName, void *data,
                            size_t count);
    static int setTagValue(camera_metadata_t *metadata, uint32_t tagID, void *data, size_t count);
    static camera_metadata_t *allocateCopyMetadata(void *src);
    static void freeMetadata(camera_metadata_t *meta);
    static int getVTagValueExt(camera_metadata_t *metadata, const char *tagName, void **data,
                               size_t &count);
    static int getTagValueExt(camera_metadata_t *metadata, uint32_t tagID, void **data,
                              size_t &count);

    static int updateVTagValue(camera_metadata_t *mdata, const char *tagName, void *data,
                               size_t count, camera_metadata_t **out);
    static int updateTagValue(camera_metadata_t *mdata, uint32_t tagID, void *data, size_t count,
                              camera_metadata_t **out);
};

} // namespace mialgo

#endif // _VENDOR_METADATA_PARSER_H_
