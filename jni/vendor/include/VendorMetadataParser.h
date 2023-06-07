/*
 * Copyright (c) 2018. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef _VENDOR_METADATA_PARSER_H_
#define _VENDOR_METADATA_PARSER_H_

#include <stdint.h>

namespace VendorMetadataParser {

#ifndef CLEAR_MACRO
#define CLEAR_MACRO
#define CLEAR(x) memset(&(x), 0, sizeof (x))
#endif//CLEAR_MACRO

enum {
    VOK = 0,
    VERROR_INVALID_PARAM,
    VERROR_INVALID_TAG,
    VERROR_INVALID_TAG_TYPE,
    VERROR_NO_TAG_ENTRY,
    VERROR_NUM
};

const char* getErrorMsg(int error);
int getVTagValue(void* mdata, const char *tagname, void **ppdata);
int getTagValue(void* mdata, uint32_t tagID, void **ppdata);
int setVTagValue(void* mdata, const char *tagname, void *data, size_t count);
int setTagValue(void* mdata, uint32_t tagID, void *data, size_t count);
void* allocateCopyMetadata(void* src);
void* allocateCopyInterMetadata(void* src);
void  freeMetadata(void* meta);

int getVTagValueExt(void* mdata, const char *tagname, void **ppdata, size_t& count);
int getTagValueExt(void* mdata, uint32_t tagID, void **ppdata, size_t& count);

}; // VendorMetadataParser

#endif// _VENDOR_METADATA_PARSER_H_
