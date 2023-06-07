/**
 * @file        dev_ipc.h
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

#ifndef __DEV_IPC_H__
#define __DEV_IPC_H__

typedef struct __Dev_Ipc Dev_Ipc;
struct __Dev_Ipc {
    S64 (*Init)(void);
    S64 (*Create)(S64 *pId, S64 key, MARK_TAG tagName);
    S64 (*Destroy)(S64 *pId);
    S64 (*Send)(S64 id, U8* data, S64 size);
    S64 (*Receive)(S64 id, U8* data, S64 size);
};

extern const Dev_Ipc m_dev_ipc;

#endif //__DEV_IPC_H__
