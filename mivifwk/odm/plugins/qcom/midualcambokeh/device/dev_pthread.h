/**
 * @file        dev_pthread.h
 * @brief
 * @details
 * @author
 * @date        2019.07.03
 * @version     0.1
 * @note
 * @warning
 * @par         History:
 *
 */

#ifndef __DEV_PTHREAD_H__
#define __DEV_PTHREAD_H__

typedef enum {
    DEV_EVENT_FLAG0 = 0x00000001,
    DEV_EVENT_FLAG1 = 0x00000002,
    DEV_EVENT_FLAG2 = 0x00000004,
    DEV_EVENT_FLAG3 = 0x00000008,
    DEV_EVENT_FLAG4 = 0x00000010,
    DEV_EVENT_FLAG5 = 0x00000020,
    DEV_EVENT_FLAG6 = 0x00000040,
    DEV_EVENT_FLAG7 = 0x00000080,
    DEV_EVENT_FLAG8 = 0x00000100,
    DEV_EVENT_FLAG9 = 0x00000200,
    DEV_EVENT_FLAG_END
}Dev_Enum_EventFlag;

typedef struct __Dev_Pthread Dev_Pthread;
struct __Dev_Pthread {
    S64 (*Init)(void);
    S64 (*Deinit)(void);
    S64 (*Create)(S64 *pId, void* (*function)(void*), void *arg, MARK_TAG tagName);
    S64 (*Destroy)(S64 *pId);
    S64 (*Exit)(void *value);
    S64 (*Join)(S64 id, void *value);
    S64 (*Detach)(S64 id);
    S64 (*Equal)(S64 id1, S64 id2);
    S64 (*Self)(void);
    S64 (*Event_Create)(void);
    S64 (*Event_Send)(S64 id, U32 flag);
    S64 (*Event_Wait)(S64 id, U32 flag, S64 timeout);
    S64 (*Report)(void);
};

extern const Dev_Pthread m_dev_pthread;

#endif //__DEV_PTHREAD_H__
