/**
 * @file        dev_trace.cpp
 * @brief
 * @details
 * @author
 * @date        2022.10.27
 * @version     0.1
 * @note
 * @warning
 * @par
 *
 */


#include "dev_stdlib.h"
#include "device.h"
#include "dev_trace.h"
#include <fcntl.h>
#include <errno.h>

#define DEV_TRACES_ENABLED      1

#define TRACE_MARK_FILE         "/sys/kernel/tracing/trace_marker"
#define TRACE_MARK_FILE_DEBUG   "/sys/kernel/debug/tracing/trace_marker"
#define TRACE_MESSAGE_MAX_LEN   (512)

U32 m_DEV_TRACE_group = Settings->SDK_TRACE_MASK.value.int_value;

static U32 init_f = FALSE;
static S64 m_mutex_Id;
static S64 fd = -1;

static const char* DevTrace_NoDirFileName(const char *pFilePath) {
    if (pFilePath == NULL) {
        return NULL;
    }
    const char *pFileName = strrchr(pFilePath, '/');
    if (NULL != pFileName) {
        pFileName += 1;
    } else {
        pFileName = pFilePath;
    }
    return pFileName;
}

S64 DevTrace_Create(S64 *pId, MARK_TAG tagName) {
    return RET_OK;
}

S64 DevTrace_Destroy(S64 *pId) {
    return RET_OK;
}

S64 DevTrace_Begin(S64 id, MARK_TAG tag) {
    if ((m_DEV_TRACE_group & 0xFF) == 0) {
        return RET_OK;
    }
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "TRACE Init err");
    DEV_IF_LOGE_RETURN_RET((-1 == fd), 0, "Write file err");
    char buffer[TRACE_MESSAGE_MAX_LEN] = { 0 };
    S32 len = Dev_snprintf(buffer, TRACE_MESSAGE_MAX_LEN, "B|%d|%s", getpid(), tag);
    write(fd, buffer, len);
    return RET_OK;
}

S64 DevTrace_End(S64 id, MARK_TAG tag) {
    if ((m_DEV_TRACE_group & 0xFF) == 0) {
        return RET_OK;
    }
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "TRACE Init err");
    DEV_IF_LOGE_RETURN_RET((-1 == fd), 0, "Write file err");
    char buffer[TRACE_MESSAGE_MAX_LEN] = { 0 };
    S32 len = Dev_snprintf(buffer, TRACE_MESSAGE_MAX_LEN, "E|%d|%s", getpid(), tag);
    write(fd, buffer, len);
    return RET_OK;
}

S64 DevTrace_Message(S64 id, MARK_TAG message) {
    if ((m_DEV_TRACE_group & 0xFF) == 0) {
        return RET_OK;
    }
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "TRACE Init err");
    DEV_IF_LOGE_RETURN_RET((-1 == fd), 0, "Write file err");
    char buffer[TRACE_MESSAGE_MAX_LEN] = { 0 };
    S32 len = Dev_snprintf(buffer, TRACE_MESSAGE_MAX_LEN, "B|%d|%s[%s:%u][%lu %lu]", getpid(), message, DevTrace_NoDirFileName(__fileName),
            __fileLine, __pid, __tid);
    write(fd, buffer, len);
    len = Dev_snprintf(buffer, TRACE_MESSAGE_MAX_LEN, "E|%d|%s", getpid(), "");
    write(fd, buffer, len);
    return RET_OK;
}

S64 DevTrace_AsyncBegin(S64 id, MARK_TAG tag, S32 cookie) {
    if ((m_DEV_TRACE_group & 0xFF) == 0) {
        return RET_OK;
    }
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "TRACE Init err");
    DEV_IF_LOGE_RETURN_RET((-1 == fd), 0, "Write file err");
    char buffer[TRACE_MESSAGE_MAX_LEN] = { 0 };
    S32 len = Dev_snprintf(buffer, TRACE_MESSAGE_MAX_LEN, "S|%d|%s|%i", getpid(), tag, cookie);
    write(fd, buffer, len);
    return RET_OK;
}

S64 DevTrace_AsyncEnd(S64 id, MARK_TAG tag, S32 cookie) {
    if ((m_DEV_TRACE_group & 0xFF) == 0) {
        return RET_OK;
    }
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "TRACE Init err");
    DEV_IF_LOGE_RETURN_RET((-1 == fd), 0, "Write file err");
    char buffer[TRACE_MESSAGE_MAX_LEN] = { 0 };
    S32 len = Dev_snprintf(buffer, TRACE_MESSAGE_MAX_LEN, "F|%d|%s|%i", getpid(), tag, cookie);
    write(fd, buffer, len);
    return RET_OK;
}

S64 DevTrace_Report(void) {
    if ((m_DEV_TRACE_group & 0xFF) == 0) {
        return RET_OK;
    }
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    return RET_OK;
}

S64 DevTrace_Init(void) {
    if ((m_DEV_TRACE_group & 0xFF) == 0) {
        return RET_OK;
    }
    if (init_f == TRUE) {
        return RET_OK;
    }
    S32 ret = Device->mutex->Create(&m_mutex_Id, MARK("m_mutex_Id"));
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), ret, "Create mutex err!");
    Device->mutex->Lock(m_mutex_Id);
    fd = open(TRACE_MARK_FILE, O_WRONLY | O_CLOEXEC);
    if (-1 == fd) {
        DEV_LOGW("OPEN FILE [%s] errno=%d", TRACE_MARK_FILE, errno);
        fd = open(TRACE_MARK_FILE_DEBUG, O_WRONLY | O_CLOEXEC);
    }
    Device->mutex->Unlock(m_mutex_Id);
    DEV_IF_LOGE_RETURN_RET((-1 == fd), ret, "open TRACE %s file err! errno=%d", TRACE_MARK_FILE_DEBUG, errno);
    init_f = TRUE;
    return ret;
}

S64 DevTrace_Deinit(void) {
    if ((m_DEV_TRACE_group & 0xFF) == 0) {
        return RET_OK;
    }
    if (init_f != TRUE) {
        return RET_OK;
    }
    Device->mutex->Lock(m_mutex_Id);
    close(fd);
    Device->mutex->Unlock(m_mutex_Id);
    Device->mutex->Destroy(&m_mutex_Id);
    init_f = FALSE;
    return RET_OK;
}

const Dev_Trace m_dev_trace = {
        .Init       = DevTrace_Init,
        .Deinit     = DevTrace_Deinit,
        .Create     = DevTrace_Create,
        .Destroy    = DevTrace_Destroy,
        .Begin      = DevTrace_Begin,
        .End        = DevTrace_End,
        .Message    = DevTrace_Message,
        .AsyncBegin = DevTrace_AsyncBegin,
        .AsyncEnd   = DevTrace_AsyncEnd,
        .Report     = DevTrace_Report,
};
