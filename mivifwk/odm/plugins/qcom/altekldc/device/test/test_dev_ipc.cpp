/**
 * @file        test_dev_ipc.c
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

#include "../device.h"

static S32 m_Result = 0;

#define TEST_DEV_IPC_KEY    (44533)

#define TEST_DEV_IPC_MSG_SIZE   64
static S64 m_pthread_Id = 0;
static S64 m_ipc_Id = 0;
static S64 m_add = 0;

static void* pthread_new_run(void *arg) {
    while (1) {
        S64 sendLen = 0;
        U8 message[TEST_DEV_IPC_MSG_SIZE];
        memset(message, 0x55, sizeof(message));
        sendLen = Device->ipc->Send(m_ipc_Id, message, sizeof(message));
        memset(message, 0, sizeof(message));
        if (sendLen != sizeof(message)) {
            DEV_LOGE("IPC TEST ERR!!!!!!!!!!!!!!!!!");
            m_Result |= RET_ERR;
        }
//        DEV_LOGI("IPC Send =%" PRIi64, sendLen);
        sendLen = Device->ipc->Receive(m_ipc_Id, message, sizeof(message));
//        DEV_LOGI("IPC Receive =%" PRIi64, sendLen);
//        DEV_LOG_HEX("IPC Receive", message, (int )sizeof(message));
        m_Result |= Device->system->SleepMs(1);
        m_add++;
        if (m_add >= 10) {
            break;
        }
    }
    Device->pthread->Exit(0);
    return NULL;
}

S32 test_dev_ipc(void) {
    m_Result = RET_OK;
    m_Result |= Device->ipc->Create(&m_ipc_Id, TEST_DEV_IPC_KEY, MARK("m_ipc_Id"));
    m_Result |= Device->pthread->Create(&m_pthread_Id, pthread_new_run, NULL, MARK("m_pthread_Id"));
    m_Result |= Device->pthread->Join(m_pthread_Id, NULL);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id);
    return m_Result;
}
