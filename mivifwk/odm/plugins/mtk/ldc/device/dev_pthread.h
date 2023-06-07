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
#include "dev_type.h"

enum {
    PTHREAD_NO_TIMEOUT = 0,
    //  PTHREAD_WAIT = 0,
    //  PTHREAD_NO_WAIT = 0,
    //  PTHREAD_EVENT_ALL = 0,
    //  PTHREAD_EVENT_ANY = 0,
    //  PTHREAD_PENDING_EVENTS = 0,
};

typedef struct __Dev_Pthread Dev_Pthread;
struct __Dev_Pthread
{
    S64 (*Create)(void *(*function)(void *), void *arg);
    S64 (*Exit)(void *value);
    S64 (*Join)(S64 id, void *value);
    S64 (*Detach)(S64 id);
    S64 (*Equal)(S64 id1, S64 id2);
    S64 (*Self)(void);
    S64 (*Event_Send)(S64 id, S64 event);
    S64 (*Event_Receive)(S64 event, S64 mode, S64 timeout, S64 *eventsout);
};

extern const Dev_Pthread m_dev_pthread;

#endif //__DEV_PTHREAD_H__
