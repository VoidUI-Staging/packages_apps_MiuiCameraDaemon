/**
 * @file        dev_socket.c
 * @brief
 * @details
 * @author      LZL
 * @date        2017.07.17
 * @version     0.1
 * @note
 * @warning
 * @par
 *
 */

#include "device.h"
#include "dev_socket.h"

static U8 init_f = FALSE;

S32 DevSocket_Status(S32 *pfd) {
    return SOC_CLOSE;
}

S32 DevSocket_Close(S32 fd) {
    DEV_IF_LOGE_RETURN_RET((fd < 0), RET_ERR, "SOCKET ERR");
    return RET_OK;
}

S32 DevSocket_Register(S32 *pfd, Callback_socket_Status callback) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "SOCKET ERR");
    return RET_OK;
}

S32 DevSocket_Connect(S32 *pfd, U8 *addr, U16 sPort, U16 lPort, U8 type, MARK_TAG tagName) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "SOCKET ERR");
    return RET_ERR;
}

S32 DevSocket_Keeplive(S32 fd) {
    return RET_OK;
}

S32 DevSocket_Send(S32 fd, U8 *pBuf, S32 len) {
    DEV_IF_LOGE_RETURN_RET((fd < 0), 0, "SOCKET ERR");
    S32 ret = 0;
    return ret;
}

S32 DevSocket_Recv(S32 fd, U8 *pBuf, S32 len) {
    DEV_IF_LOGE_RETURN_RET((fd < 0), 0, "SOCKET ERR");
    return 0;
}

S32 DevSocket_Handler(void) {
    return RET_OK;
}

S32 DevSocket_Init(void) {
    init_f = TRUE;
    return RET_OK;
}

S32 DevSocket_Deinit(void) {
    init_f = FALSE;
    return RET_OK;
}

const Dev_Socket m_dev_socket = {
        .Init       =DevSocket_Init,
        .Deinit     =DevSocket_Deinit,
        .Handler    =DevSocket_Handler,
        .Register   =DevSocket_Register,
        .Close      =DevSocket_Close,
        .Status     =DevSocket_Status,
        .Connect    =DevSocket_Connect,
        .Send       =DevSocket_Send,
        .Recv       =DevSocket_Recv,
};
