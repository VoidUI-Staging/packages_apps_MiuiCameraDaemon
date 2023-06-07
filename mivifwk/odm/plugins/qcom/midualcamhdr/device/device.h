/**
 * @file        device.h
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

#ifndef __DEVICE_H__
#define __DEVICE_H__

#include "dev_type.h"
#include "settings.h"
#include "dev_stdlib.h"
#include "dev_time.h"
#include "dev_log.h"
#include "dev_system.h"
#include "dev_file.h"
#include "dev_image.h"
#include "dev_detector.h"
#include "dev_pthread.h"
#include "dev_mutex.h"
#include "dev_semaphore.h"
#include "dev_msg.h"
#include "dev_queue.h"
#include "dev_verified.h"
#include "dev_timer.h"
#include "dev_pthreadPool.h"
#include "dev_memoryPool.h"
#include "dev_ipc.h"
#include "dev_socket.h"
#include "dev_settings.h"
#include "dev_libPool.h"

#include "dev_test.h"

void device_Init(void);
void device_Handler(void);
void device_Deinit(void);

typedef struct __Device Device_t;
struct __Device {
    void (*Init)(void);
    void (*Handler)(void);
    void (*Deinit)(void);
    const Dev_Log *log;
    const Dev_Test *test;
    const Dev_System *system;
    const Dev_Time *time;
    const Dev_File *file;
    const Dev_Image *image;
    const Dev_Detector *detector;
    const Dev_Pthread *pthread;
    const Dev_Mutex *mutex;
    const Dev_Semaphore *semaphore;
    const Dev_Msg *msg;
    const Dev_Queue *queue;
    const Dev_PthreadPool *pthreadPool;
    const Dev_MemoryPool *memoryPool;
    const Dev_Verified *verified;
    const Dev_Timer *timer;
    const Dev_Ipc *ipc;
    const Dev_Socket *socket;
    const Dev_Settings *settings;
    const Dev_LibPool *libPool;
};

extern const Device_t *Device;

#define DEV_DEVICE_LINK                 \
    .Init       =device_Init            ,\
    .Handler    =device_Handler         ,\
    .Deinit     =device_Deinit          ,\
    .log        =&m_dev_log             ,\
    .test       =&m_dev_test            ,\
    .system     =&m_dev_system          ,\
    .time       =&m_dev_time            ,\
    .file       =&m_dev_file            ,\
    .image      =&m_dev_image           ,\
    .detector   =&m_dev_detector        ,\
    .pthread    =&m_dev_pthread         ,\
    .mutex      =&m_dev_mutex           ,\
    .semaphore  =&m_dev_semaphore       ,\
    .msg        =&m_dev_msg             ,\
    .queue      =&m_dev_queue           ,\
    .pthreadPool=&m_dev_pthreadPool     ,\
    .memoryPool =&m_dev_memoryPool      ,\
    .verified   =&m_dev_verified        ,\
    .timer      =&m_dev_timer           ,\
    .ipc        =&m_dev_ipc             ,\
    .socket     =&m_dev_socket          ,\
    .settings   =&m_dev_settings        ,\
    .libPool    =&m_dev_libPool         ,\

#define DEV_DEVICE_INIT()               \
    do{                                 \
        Device->log->Init();            \
        DEV_LOGI("DEVICE [%s] VERSION[%s] RELEASE[%d] INIT", \
        Settings->SDK_PROJECT_NAME.value.char_value, Settings->SDK_PROJECT_VERSION.value.char_value, Settings->SDK_PROJECT_RELEASE.value.bool_value);\
        Device->settings->Init();       \
        Device->mutex->Init();          \
        Device->semaphore->Init();      \
        Device->pthread->Init();        \
        Device->msg->Init();            \
        Device->queue->Init();          \
        Device->timer->Init();          \
        Device->detector->Init();       \
        Device->pthreadPool->Init();    \
        Device->memoryPool->Init();     \
        Device->libPool->Init();        \
        Device->image->Init();          \
        /*Device->socket->Init();     */\
        Device->test->Init();           \
    }while(0)


#define DEV_DEVICE_HANDLER()            \
    do{                                 \
        /*Device->socket->Handler();  */\
        Device->test->Handler();        \
    }while(0)


#define DEV_DEVICE_DEINIT()             \
    do{                                 \
        DEV_LOGI("DEVICE [%s] VERSION[%s] RELEASE[%d] DEINIT", \
        Settings->SDK_PROJECT_NAME.value.char_value, Settings->SDK_PROJECT_VERSION.value.char_value, Settings->SDK_PROJECT_RELEASE.value.bool_value);\
        Device->test->Deinit();         \
        /*Device->socket->Deinit();   */\
        Device->image->Deinit();        \
        Device->libPool->Deinit();      \
        Device->memoryPool->Deinit();   \
        Device->pthreadPool->Deinit();  \
        Device->detector->Deinit();     \
        Device->timer->Deinit();        \
        Device->queue->Deinit();        \
        Device->msg->Deinit();          \
        Device->pthread->Deinit();      \
        Device->semaphore->Deinit();    \
        Device->mutex->Deinit();        \
        Device->settings->Deinit();     \
        Device->log->Deinit();          \
    }while(0)

#endif // __DEVICE_H__
