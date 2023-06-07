/**
 * @file        test_dev_queue.c
 * @brief
 * @details
 * @author
 * @date        2016.06.15
 * @version     0.1
 * @note
 * @warning
 * @par         History:
 *
 */

#include "../device.h"

static S32 m_Result = 0;

#define TEST_MAX_TIMES      512
#define TEST_DATA_SIZE      (1024*1024*40)
#define TEST_RETRY_NUM      1000

static S64 queue_id;
static U8 s_test_data[TEST_DATA_SIZE];
static U8 r_test_data[TEST_DATA_SIZE];

static S64 m_pthread_1;
static S64 m_pthread_2;

static void* pthread_new_run_1(void *arg) {
    S64 addrOffset = 0;
    S64 retry = 0;
    while (addrOffset != TEST_DATA_SIZE) {
        for (S64 j = 0; j < TEST_MAX_TIMES; j++) {
            while (!Device->queue->Empty(queue_id)) {
                S64 recvLen = 0;
                S64 size = (TEST_DATA_SIZE / TEST_MAX_TIMES);
                recvLen = Device->queue->Read(queue_id, &r_test_data[addrOffset], size);
                addrOffset += recvLen;
                DEV_IF_LOGW((recvLen != size), "READ Size is not enough[%ld:%ld]", size, recvLen);
//                DEV_LOGI("Rece message id=%0.f,len= %0.f,%ld", (double )queue_id, (double )recvLen, r_test_data[0]);
            }
        }
        Device->system->SleepUs(1);
        retry++;
        if (retry > TEST_RETRY_NUM) {
            break;
        }
    }
    m_Result |= Device->pthread->Exit(0);
    return NULL;
}

static void* pthread_new_run_2(void *arg) {
    for (S64 j = 0; j < TEST_MAX_TIMES; j++) {
        S64 size = (TEST_DATA_SIZE / TEST_MAX_TIMES);
        U8 *addr = &s_test_data[(j * size)];
        S32 sendLen = Device->queue->Write(queue_id, addr, size);
        if (sendLen != size) {
            DEV_LOGE("QUEUEQ_Send fail=%d", sendLen);
            m_Result |= RET_ERR;
            break;
        }
//        DEV_LOGI("Send message id= %0d,len= %0.f ,%0.f send OK = %ld", queue_id, size, sendLen);
    }
    m_Result |= Device->pthread->Exit(0);
    return NULL;
}

S32 test_dev_queue(void) {
    m_Result = RET_OK;
//  m_Result |= Device->queue->Init();
    Device->queue->Create(&queue_id, TEST_DATA_SIZE, MARK("TEST queue_id"));
    DEV_IF_LOGE((queue_id <= 0), "Create fail");
    memset(s_test_data, 0x34, TEST_DATA_SIZE);
    memset(r_test_data, 0x00, TEST_DATA_SIZE);

//    Device->queue->Report();
    m_Result |= Device->pthread->Create(&m_pthread_1, pthread_new_run_1, NULL, MARK("m_pthread_Id1"));
    m_Result |= Device->pthread->Create(&m_pthread_2, pthread_new_run_2, NULL, MARK("m_pthread_Id2"));
    m_Result |= Device->pthread->Join(m_pthread_1, NULL);
    m_Result |= Device->pthread->Join(m_pthread_2, NULL);

    U16 msg_crc16Send = 0;
    U16 msg_crc16Recv = 0xFF;

    msg_crc16Send = Device->verified->Crc16(s_test_data, TEST_DATA_SIZE, 0);
    msg_crc16Recv = Device->verified->Crc16(r_test_data, TEST_DATA_SIZE, 0);
    if (msg_crc16Send != msg_crc16Recv) {
        DEV_LOGE("QUEUE TEST ERR [%d:%d] Empty[%ld]!!!!!!!!!!!!!!!!!", msg_crc16Send, msg_crc16Recv, Device->queue->Empty(queue_id));
        m_Result |= RET_ERR;
    }
    m_Result |= Device->queue->Destroy(&queue_id);
    m_Result |= Device->pthread->Destroy(&m_pthread_1);
    m_Result |= Device->pthread->Destroy(&m_pthread_2);
    return m_Result;
}
