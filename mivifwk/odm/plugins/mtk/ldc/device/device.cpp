/**
 * @file		device.cpp
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

#include "device.h"

static U8 init_f = FALSE;

static const Device_t m_device = {DEV_DEVICE_INIT};

const Device_t *device_getter(void)
{
    return &m_device;
}

void device_Init(void)
{
    if (init_f == TRUE) {
        return;
    }
    init_f = TRUE;
    DEV_LOGI("DEVICE PROJECT [%s] VERSION[%d] RELEASE[%d] INIT", PROJECT_NAME, PROJECT_VERSION,
             PROJECT_RELEASE);
    Device->mutex->Init();
    Device->sem->Init();
    Device->msg->Init();
    Device->queue->Init();
    Device->detector->Init();
    //    Device->timer->Init();
    Device->test->Init();
}

void device_Handler(void)
{
    if (init_f != TRUE) {
        return;
    }
    Device->test->Handler();
}

void device_Uninit(void)
{
    if (init_f != TRUE) {
        return;
    }
    DEV_LOGI("DEVICE PROJECT [%s] VERSION[%d] RELEASE[%d] UNINIT", PROJECT_NAME, PROJECT_VERSION,
             PROJECT_RELEASE);
    init_f = FALSE;
    Device->test->Uninit();
    Device->timer->Uninit();
    Device->detector->Uninit();
    Device->queue->Uninit();
    Device->msg->Uninit();
    Device->sem->Uninit();
    Device->mutex->Uninit();
}
