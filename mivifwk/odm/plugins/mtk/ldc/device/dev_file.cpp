/**
 * @file		dev_file.cpp
 * @brief
 * @details
 * @author
 * @date        2019.07.03
 * @version		0.1
 * @note
 * @warning
 * @par
 *
 */

#include "dev_file.h"

#include <stdlib.h>
#include <string.h>

#include "dev_log.h"
#include "dev_type.h"

S64 DevFile_Open(const char *name)
{
    DEV_IF_LOGE_RETURN_RET((name == NULL), 0, "open file name err");
    FILE *fd = fopen(name, "wb+");
    DEV_IF_LOGE_RETURN_RET((fd == NULL), 0, "open file err");
    return (S64)(fd);
}

S64 DevFile_Read(S64 fd, void *buf, S64 size, S64 offset)
{
    DEV_IF_LOGE_RETURN_RET((fd == 0), 0, "open file err");
    S64 ret = fseek((FILE *)fd, offset, SEEK_SET);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "open file err");
    ret = fread(buf, sizeof(char), size, (FILE *)fd);
    DEV_IF_LOGE((ret != (S64)size), "read File size err %" PRIi64 " !=%" PRIi64, ret, size);
    return ret;
}

S64 DevFile_Write(S64 fd, void *buf, S64 size, S64 offset)
{
    DEV_IF_LOGE_RETURN_RET((fd == 0), 0, "open file err");
    S64 ret = fseek((FILE *)fd, offset, SEEK_SET);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "open file err");
    ret = fwrite(buf, sizeof(char), size, (FILE *)fd);
    DEV_IF_LOGE((ret != (S64)size), "write File size err %" PRIi64 " !=%" PRIi64, ret, size);
    return ret;
}

S64 DevFile_Close(S64 *fd)
{
    DEV_IF_LOGE_RETURN_RET((*fd == 0), 0, "close file err");
    fclose((FILE *)*fd);
    *fd = 0;
    return RET_OK;
}

S64 DevFile_WriteOnce(const char *name, void *buf, S64 size, S64 offset)
{
    DEV_IF_LOGE_RETURN_RET(((name == NULL) || (buf == NULL)), 0, "open file name err");
    FILE *fd = fopen(name, "wb+");
    DEV_IF_LOGE_RETURN_RET((fd == NULL), 0, "open file err");
    S64 ret = fseek(fd, offset, SEEK_SET);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "open file err");
    ret = fwrite(buf, sizeof(char), size, fd);
    DEV_IF_LOGE((ret != (S64)size), "write File size err %" PRIi64 " !=%" PRIi64, ret, size);
    fclose(fd);
    return ret;
}

S64 DevFile_ReadOnce(const char *name, void *buf, S64 size, S64 offset)
{
    DEV_IF_LOGE_RETURN_RET(((name == NULL) || (buf == NULL)), 0, "open file name err");
    FILE *fd = fopen(name, "rb");
    DEV_IF_LOGE_RETURN_RET((fd == NULL), 0, "open file err");
    S64 ret = fseek(fd, offset, SEEK_SET);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "open file err");
    ret = fread(buf, sizeof(char), size, fd);
    DEV_IF_LOGE((ret != (S64)size), "read File size err %" PRIi64 " !=%" PRIi64, ret, size);
    fclose(fd);
    return ret;
}

S64 DevFile_Delete(const char *name)
{
    DEV_IF_LOGE_RETURN_RET((name == NULL), RET_ERR_ARG, "open file name err");
    DEV_IF_LOGE_RETURN_RET((remove(name) != RET_OK), RET_ERR, "del file err");
    return RET_OK;
}

S64 DevFile_GetSize(const char *name)
{
    DEV_IF_LOGE_RETURN_RET((name == NULL), 0, "open file name err");
    FILE *fd = fopen(name, "rb");
    DEV_IF_LOGE_RETURN_RET((fd == NULL), 0, "open file err");
    S64 ret = fseek(fd, 0L, SEEK_END);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "open file err");
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
