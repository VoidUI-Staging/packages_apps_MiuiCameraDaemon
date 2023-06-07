/**
 * @file        dev_file.cpp
 * @brief
 * @details
 * @author
 * @date        2019.07.03
 * @version     0.1
 * @note
 * @warning
 * @par
 *
 */

#include <string.h>
#include <stdlib.h>
#include "dev_type.h"
#include "dev_log.h"
#include "dev_file.h"

S64 DevFile_Open(S64 *pFd, const char *name, MARK_TAG tagName) {
    DEV_IF_LOGE_RETURN_RET((pFd == NULL), RET_ERR, "open file err");
    DEV_IF_LOGE_RETURN_RET((name == NULL), RET_ERR, "open file name err");
    FILE *fd = fopen(name, "wb+");
    DEV_IF_LOGE_RETURN_RET((fd == NULL), RET_ERR, "open file err");
    *pFd = (S64)(fd);
    return RET_OK;
}

S64 DevFile_Read(S64 fd, void *buf, S64 size, S64 offset) {
    DEV_IF_LOGE_RETURN_RET((fd == 0), 0, "Read file err");
    S64 ret = fseek((FILE*)fd, offset, SEEK_SET);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "Read file err");
    ret = (S64) fread(buf, sizeof(char), size, (FILE *) fd);
    DEV_IF_LOGE((ret != (S64 )size), "read File size err %" PRIi64" !=%" PRIi64, ret, size);
    return ret;
}

S64 DevFile_Write(S64 fd, void *buf, S64 size, S64 offset) {
    DEV_IF_LOGE_RETURN_RET((fd == 0), 0, "Write file err");
    S64 ret = fseek((FILE*)fd, offset, SEEK_SET);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "Write file err");
    ret = (S64)fwrite(buf, sizeof(char), size, (FILE*)fd);
    DEV_IF_LOGE((ret != (S64 )size), "write File size err %" PRIi64" !=%" PRIi64, ret, size);
    return ret;
}

S64 DevFile_Close(S64 *pFd) {
    DEV_IF_LOGE_RETURN_RET((pFd == NULL), RET_OK, "close file err");
    DEV_IF_LOGE_RETURN_RET((*pFd == 0), RET_OK, "close file err");
    fclose((FILE*)*pFd);
    *pFd = 0;
    return RET_OK;
}

S64 DevFile_WriteOnce(const char *name, void *buf, S64 size, S64 offset) {
    DEV_IF_LOGE_RETURN_RET(((name == NULL)||(buf==NULL)), 0, "WriteOnce file name err");
    FILE *fd = fopen(name, "wb+");
    DEV_IF_LOGE_RETURN_RET((fd == NULL), 0, "open file err");
    S64 ret = fseek(fd, offset, SEEK_SET);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), fclose(fd), "WriteOnce file err");
    ret = (S64)fwrite(buf, sizeof(char), size, fd);
    DEV_IF_LOGE((ret != (S64 )size), "write File size err %" PRIi64" !=%" PRIi64, ret, size);
    fclose(fd);
    return ret;
}

S64 DevFile_ReadOnce(const char *name, void *buf, S64 size, S64 offset) {
    DEV_IF_LOGE_RETURN_RET(((name == NULL)||(buf==NULL)), 0, "ReadOnce file name err");
    FILE *fd = fopen(name, "rb");
    DEV_IF_LOGE_RETURN_RET((fd == NULL), 0, "open file err");
    S64 ret = fseek(fd, offset, SEEK_SET);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), fclose(fd), "ReadOnce file err");
    ret = (S64)fread(buf, sizeof(char), size, fd);
    DEV_IF_LOGE((ret != (S64 )size), "read File size err %" PRIi64" !=%" PRIi64, ret, size);
    fclose(fd);
    return ret;
}

S64 DevFile_Delete(const char *name) {
    DEV_IF_LOGE_RETURN_RET((name == NULL), RET_ERR_ARG, "Delete file name err");
    DEV_IF_LOGE_RETURN_RET((remove(name) != RET_OK), RET_ERR, "del file err");
    return RET_OK;
}

S64 DevFile_GetSize(const char *name) {
    DEV_IF_LOGE_RETURN_RET((name == NULL), 0, "GetSize file name err");
    FILE *fd = fopen(name, "rb");
    DEV_IF_LOGE_RETURN_RET((fd == NULL), 0, "open file err");
    S64 ret = fseek(fd, 0L, SEEK_END);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), fclose(fd), "GetSize file err");
    ret = ftell(fd);
    fseek(fd, 0L, SEEK_SET);
    fclose(fd);
    return ret;
}

const Dev_File m_dev_file = {
        .Open = DevFile_Open,
        .Read = DevFile_Read,
        .Write = DevFile_Write,
        .Close = DevFile_Close,
        .Delete = DevFile_Delete,
        .GetSize = DevFile_GetSize,
        .WriteOnce = DevFile_WriteOnce,
        .ReadOnce = DevFile_ReadOnce,
};
