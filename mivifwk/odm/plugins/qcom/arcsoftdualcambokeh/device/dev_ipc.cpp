/**
 * @file        dev_ipc.cpp
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

#include <sys/ipc.h>
#include <sys/msg.h>
#include "string.h"
#include "dev_type.h"
#include "dev_log.h"
#include "dev_ipc.h"

#define IPC_BUF_MAX     (8192-4)
#define IPC_MSG_TYPE    (1)

typedef struct __Dev_msg_st {
    S32 type;
    char data[IPC_BUF_MAX];
} Dev_msg_st;

S64 DevIpc_Init(void) {
    return RET_OK;
}

S64 DevIpc_Create(S64 *pId, S64 key, MARK_TAG tagName) {
    DEV_IF_LOGE_RETURN_RET(pId == NULL, RET_ERR, "IPC id err");
    S64 id = (S64) msgget((key_t)key, 0777 | IPC_CREAT);
    DEV_IF_LOGE_RETURN_RET((id < 0), RET_ERR, "IPC Create id err%" PRIi64, id);
    *pId = id;
    return RET_OK;
}

S64 DevIpc_Destroy(S64 *pId) {
    DEV_IF_LOGE_RETURN_RET(pId == NULL, RET_ERR, "IPC id err");
    DEV_IF_LOGE_RETURN_RET((*pId <= 0), RET_ERR, "IPC id err");
    S64 ret = msgctl((int )*pId, IPC_RMID, 0);
    *pId = -1;
    DEV_IF_LOGE_RETURN_RET((ret <= 0), RET_ERR, "IPC Destroy err");
    return RET_OK;
}

S64 DevIpc_Send(S64 id, U8* data, S64 size) {
    DEV_IF_LOGE_RETURN_RET((data == NULL), 0, "Send data err");
    DEV_IF_LOGE_RETURN_RET((size >= IPC_BUF_MAX), 0, "Send data Size err");
    DEV_IF_LOGE_RETURN_RET((id <= 0), 0, "IPC id err");
    Dev_msg_st st_msg_buf;
    st_msg_buf.type = IPC_MSG_TYPE;
    memcpy(st_msg_buf.data, data, size);
    S64 ret = msgsnd((int )id, (void *) &st_msg_buf, size, IPC_NOWAIT);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "Send data Size err");
    return size;
}

S64 DevIpc_Receive(S64 id, U8* data, S64 size) {
    DEV_IF_LOGE_RETURN_RET((data == NULL), 0, "Receive data err");
    DEV_IF_LOGE_RETURN_RET((size >= IPC_BUF_MAX), 0, "Receive data Size err");
    DEV_IF_LOGE_RETURN_RET((id <= 0), 0, "IPC id err");
    Dev_msg_st st_msg_buf;
    st_msg_buf.type = IPC_MSG_TYPE;
    S64 ret = msgrcv((int)id, &st_msg_buf, size, IPC_MSG_TYPE, IPC_NOWAIT);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "Receive data Size err");
    memcpy(data, st_msg_buf.data, ret);
    return ret;
}

const Dev_Ipc m_dev_ipc = {
        .Init       =DevIpc_Init,
        .Create     =DevIpc_Create,
        .Destroy    =DevIpc_Destroy,
        .Send       =DevIpc_Send,
        .Receive    =DevIpc_Receive,
};
