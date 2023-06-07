/**
 * @file		device.h
 * @brief
 * @details
 * @author
 * @date        2019.07.03
 * @version		0.1
 * @note
 * @warning
 * @par
 *
 */

#ifndef __DEVICE_H__
#define __DEVICE_H__

#include "config.h"
#include "dev_detector.h"
#include "dev_file.h"
#include "dev_image.h"
#include "dev_log.h"
#include "dev_msg.h"
#include "dev_mutex.h"
#include "dev_pthread.h"
#include "dev_queue.h"
#include "dev_sem.h"
#include "dev_system.h"
#include "dev_time.h"
#include "dev_timer.h"
#include "dev_type.h"
#include "dev_verified.h"
#include "test/test.h"

#define Device device_getter()

void device_Init(void);
void device_Handler(void);
void device_Uninit(void);

typedef struct __Device Device_t;
struct __Device
{
    void (*Init)(void);
    void (*Handler)(void);
    void (*Uninit)(void);
    const Dev_Test *test;
    const Dev_System *system;
    const Dev_Time *time;
    const Dev_File *file;
    const Dev_Image *image;
    const Dev_Detector *detector;
    const Dev_Pthread *pthread;
    const Dev_Mutex *mutex;
    const Dev_Sem *sem;
    const Dev_Msg *msg;
    const Dev_Queue *queue;
    const Dev_Verified *verified;
    const Dev_Timer *timer;
};
const Device_t *device_getter(void);

#define DEV_DEVICE_INIT                                                                           \
    .Init = device_Init, .Handler = device_Handler, .Uninit = device_Uninit, .test = &m_dev_test, \
    .system = &m_dev_system, .time = &m_dev_time, .file = &m_dev_file, .image = &m_dev_image,     \
    .detector = &m_dev_detector, .pthread = &m_dev_pthread, .mutex = &m_dev_mutex,                \
    .sem = &m_dev_sem, .msg = &m_dev_msg, .queue = &m_dev_queue, .verified = &m_dev_verified,     \
    .timer = &m_dev_timer,

#endif // __DEVICE_H__
