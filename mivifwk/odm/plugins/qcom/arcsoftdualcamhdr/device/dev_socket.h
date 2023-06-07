/**
 * @file        dev_socket.h
 * @brief
 * @details
 * @author
 * @date        2017.07.17
 * @version     0.1
 * @note
 * @warning
 * @par         History:
 *
 */

#ifndef __DEV_SOCKET_H__
#define __DEV_SOCKET_H__

typedef enum {
    DEV_SOC_TYPE_TCP = 0,
    DEV_SOC_TYPE_UDP = 1,
} Enum_SOC_Type;

typedef enum {
    SOC_RECV = 1,
    SOC_CONN = RET_OK,
    SOC_CLOSE = RET_ERR,
} Enum_SOC_Notify;

typedef void (*Callback_socket_Status)(S32 fd, S32 status);

typedef struct __Dev_Socket Dev_Socket;
struct __Dev_Socket {
    S32 (*Init)(void);
    S32 (*Deinit)(void);
    S32 (*Handler)(void);
    S32 (*Register)(S32 *pfd, Callback_socket_Status callback);
    S32 (*Close)(S32 fd);
    S32 (*Status)(S32 *pfd);
    S32 (*Connect)(S32 *pfd, U8* addr, U16 sPort, U16 lPort, U8 type, MARK_TAG tagName);
    S32 (*Send)(S32 fd, U8* pBuf, S32 len);
    S32 (*Recv)(S32 fd, U8* pBuf, S32 len);
};

extern const Dev_Socket m_dev_socket;

#endif //__DEV_SOCKET_H__
