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
static pthread_mutex_t m_mutex;

static U8 init_f = FALSE;
static U32 m_InitCounter = 0;

static const Device_t m_device = {DEV_DEVICE_LINK };

const Device_t *Device = &m_device;

void device_Init(void) {
    if (Settings->SDK_PTHREAD_SUPPORT.value.bool_value == TRUE) {
        pthread_mutex_lock(&m_mutex);
    }
    if ((m_InitCounter == 0) && (init_f != TRUE)) {
        DEV_DEVICE_INIT();
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
        DEV_DEVICE_DEINIT();
        m_InitCounter = 0;
        init_f = FALSE;
    }
    if (Settings->SDK_PTHREAD_SUPPORT.value.bool_value == TRUE) {
        pthread_mutex_unlock(&m_mutex);
    }
}
