/**
 * @file        dev_msg.h
 * @brief
 * @details     根据目标芯片的内存适当调整 MSG_QUEUE_MSG_MAX 最大消息的存储量，防止内存溢出
 * @author
 * @date        2019.07.03
 * @version     0.1
 * @note
 * @warning
 * @par         History:
 *
 */

#ifndef __DEV_MSG_H__
#define __DEV_MSG_H__

typedef enum {
    MSG_TYPE_DATA = (0 << 0),
    MSG_TYPE_INDEX = (1 << 0),
    MSG_TYPE_FIFO = (0 << 1),
    MSG_TYPE_STACK = (1 << 1),
    MSG_TYPE_ABANDON_LAST = (0 << 2),
    MSG_TYPE_ABANDON_FIRST = (1 << 2),
} Enum_MSG_TYPE;

typedef struct __Dev_Msg Dev_Msg;
struct __Dev_Msg {
    S64 (*Init)(void);
    S64 (*Deinit)(void);
    S64 (*Create)(S64 *pId, Enum_MSG_TYPE type, S64 msg_num_max, MARK_TAG tagName);
    S64 (*Destroy)(S64 *pId);
    S64 (*Write)(S64 id, void* data, S64 size);
    S64 (*Read)(S64 id, void* data, S64 size);
    S64 (*Try)(S64 id, void* data, S64 size);
    S64 (*Empty)(S64 id);
    S64 (*Full)(S64 id);
    S64 (*Count)(S64 id);
    S64 (*Available)(S64 id);
    S64 (*Size)(S64 id);
    S64 (*Clean)(S64 id);
    S64 (*Report)(void);
};
extern const Dev_Msg m_dev_msg;

#endif //__DEV_MSG_H__

