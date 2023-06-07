/**
 * @file        dev_pthread.cpp
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

#include "dev_pthread.h"

#include <pthread.h>

#include "dev_log.h"
#include "dev_type.h"

S64 DevPthread_Create(void *(*function)(void *), void *arg)
{
    DEV_IF_LOGE_RETURN_RET((function == NULL), 0, "ARG ERR");
    S64 ret = RET_ERR;
    pthread_attr_t attr;
    pthread_t pthread;
    ret = pthread_attr_init(&attr);
    //    pthread_attr_setscope (&attr, PTHREAD_SCOPE_SYSTEM);
    //    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);
    DEV_IF_LOGE(ret, "pthread_attr_init");
    ret = pthread_create(&pthread, &attr, function, arg);
    if (ret == RET_OK) {
        return (S64)pthread;
    }
    DEV_LOGE("Pthread Create err");
    return 0;
}

S64 DevPthread_Destroy(S64 id, pthread_attr_t *attr)
{
    // TODO
    return pthread_attr_destroy(attr);
}

S64 DevPthread_Exit(void *value)
{
    pthread_exit(value);
    return RET_OK;
}

S64 DevPthread_Join(S64 id, void *value)
{
    return pthread_join((pthread_t)id, &value);
}

S64 DevPthread_Detach(S64 id)
{
    return pthread_detach((pthread_t)id);
}

S64 DevPthread_Equal(S64 id1, S64 id2)
{
    return pthread_equal((pthread_t)id1, (pthread_t)id2);
}

S64 DevPthread_Self(void)
{
    return pthread_self();
}

S64 DevPthread_Event_Send(S64 id, S64 event)
{
    return RET_ERR; // TODO
}

S64 DevPthread_Event_Receive(S64 event, S64 mode, S64 timeout, S64 *eventsout)
{
    return RET_ERR; // TODO
}

const Dev_Pthread m_dev_pthread = {
    .Create = DevPthread_Create,
    .Exit = DevPthread_Exit,
    .Join = DevPthread_Join,
    .Detach = DevPthread_Detach,
    .Equal = DevPthread_Equal,
    .Self = DevPthread_Self,
    .Event_Send = DevPthread_Event_Send,
    .Event_Receive = DevPthread_Event_Receive,
};
