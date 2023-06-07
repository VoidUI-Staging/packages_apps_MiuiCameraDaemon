/**
 * @file        device.c
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

#include "device.h"
#include <pthread.h>

#define DEVICE_INIT_TIMEOUT_MS        2000

static pthread_mutex_t m_mutex;

static U8 init_f = FALSE;
static U32 m_InitCounter = 0;

static const Device_t m_device = {DEV_DEVICE_LINK };

const Device_t *Device = &m_device;

static int pid = 0;
static int tid = 0;
static S64 m_TimerId_Init;
static S64 m_TimerId_DeInit;

static void Dev_Timer_handler_init(void *arg) {
    DEV_LOGW("[-------------------------------------DEVICE INIT TIME OUT REPORT START  --------------------------------------]");
    Device->system->ShowStack(pid, tid);
    DEV_LOGW("[-------------------------------------DEVICE INIT TIME OUT REPORT END    --------------------------------------]");
}

static void Dev_Timer_handler_deinit(void *arg) {
    DEV_LOGW("[-------------------------------------DEVICE DEINIT TIME OUT REPORT START  ------------------------------------]");
    Device->system->ShowStack(pid, tid);
    DEV_LOGW("[-------------------------------------DEVICE DEINIT TIME OUT REPORT END    ------------------------------------]");
}

void device_Init(void) {
    if (Settings->SDK_PTHREAD_SUPPORT.value.bool_value == TRUE) {
        pthread_mutex_lock(&m_mutex);
    }
    if ((m_InitCounter == 0) && (init_f != TRUE)) {
        pid = __pid();
        tid = __tid();
        Device->timer->CreateBase(&m_TimerId_Init, Dev_Timer_handler_init);
        Device->timer->StartBase(m_TimerId_Init, DEVICE_INIT_TIMEOUT_MS);
        DEV_DEVICE_INIT();
        Device->timer->StopBase(m_TimerId_Init);
        Device->timer->DestroyBase(m_TimerId_Init);
        init_f = TRUE;
    }
    m_InitCounter++;
    if (Settings->SDK_PTHREAD_SUPPORT.value.bool_value == TRUE) {
        pthread_mutex_unlock(&m_mutex);
    }
}

void device_Handler(void) {
    if (init_f != TRUE) {
        return;
    }
    DEV_DEVICE_HANDLER();
}

void device_Deinit(void) {
    if (Settings->SDK_PTHREAD_SUPPORT.value.bool_value == TRUE) {
        pthread_mutex_lock(&m_mutex);
    }
    m_InitCounter--;
    if ((init_f != FALSE) && (m_InitCounter <= 0)) {
        pid = __pid();
        tid = __tid();
        Device->timer->CreateBase(&m_TimerId_DeInit, Dev_Timer_handler_deinit);
        Device->timer->StartBase(m_TimerId_DeInit, DEVICE_INIT_TIMEOUT_MS);
        DEV_DEVICE_DEINIT();
        Device->timer->StopBase(m_TimerId_DeInit);
        Device->timer->DestroyBase(m_TimerId_DeInit);
        m_InitCounter = 0;
        init_f = FALSE;
    }
    if (Settings->SDK_PTHREAD_SUPPORT.value.bool_value == TRUE) {
        pthread_mutex_unlock(&m_mutex);
    }
}
