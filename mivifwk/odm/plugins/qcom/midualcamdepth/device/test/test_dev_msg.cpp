/**
 * @file       test_dev_msg.cpp
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

#define TEST_MAX_TIMES      10

static S64 m_pthread_1;
static S64 m_pthread_2;

#define MSG_ID_NUM 128
static S64 msg_id[MSG_ID_NUM];
static U8 s_test_data[128];
static U8 r_test_data[128];
static S64 w = 0;
static S64 j = 0;
static S64 addSend = 0;
static S64 addRece = 0;

static U16 msg_crc16Send[MSG_ID_NUM];
static U16 msg_crc16Recv[MSG_ID_NUM];

static void* pthread_new_run_1(void *arg) {
    while (addRece <= TEST_MAX_TIMES * 2) {
        while (!Device->msg->Empty(msg_id[j])) {
//            int msg_cnt = Device->msg->Count(msg_id[j]);
//            int msg_len = Device->msg->Length(msg_id[j]);
//            DEV_LOGI("msgId=%ld  message=%ld msg_len=%ld\n", msg_id[j], msg_cnt, msg_len);
            S64 recvLen = 0;
            Dev_memset(r_test_data, 0x00, sizeof(r_test_data));
            recvLen = Device->msg->Read(msg_id[j], r_test_data, sizeof(r_test_data));
            msg_crc16Recv[j] = Device->verified->Crc16(r_test_data, recvLen, msg_crc16Recv[j]);
//            DEV_LOGI("Rece message id=%" PRIi64 ",len= %0.f,%ld", msg_id[j], (double )recv_msg_len, r_test_data[recv_msg_len - 1]);
//            m_Result |= Device->system->SleepUs(1);
//            DEV_LOG_HEX(r_test_data, recv_msg_len);
            j++;
            if (j >= MSG_ID_NUM) {
                j = 0;
            }
        }
        m_Result |= Device->system->SleepMs(10); //Wait for the pthread_new_run_2 to exit
        addRece++;
    }
    addRece = 0;
    m_Result |= Device->pthread->Exit(0);
    return NULL;
}

static void* pthread_new_run_2(void *arg) {
    while (addSend <= TEST_MAX_TIMES) {
        for (w = 0; w < MSG_ID_NUM; w++) {
            Dev_memset(s_test_data, w, sizeof(s_test_data));
            S64 sendLen = Device->msg->Write(msg_id[w], s_test_data, (S64)sizeof(s_test_data));
            if (sendLen != (S64)sizeof(s_test_data)) {
                DEV_LOGE("MSGQ_Send fail");
                m_Result |= RET_ERR;
                break;
            }
            msg_crc16Send[w] = Device->verified->Crc16(s_test_data, sizeof(s_test_data), msg_crc16Send[w]);
//            DEV_LOGI("Send message id=%" PRIi64 ",len= %0.f ,%0.f", msg_id[w], (double )sizeof(s_test_data), (double )w);
//            m_Result |= Device->system->SleepUs(1);
        }
        addSend++;
    }
//    m_Result |= Device->msg->Report();
    addSend = 0;
    m_Result |= Device->pthread->Exit(0);
    return NULL;
}

S32 test_dev_msg(void) {
    m_Result = RET_OK;
//    m_Result |= Device->msg->Init();
    for (int i = 0; i < MSG_ID_NUM; i++) {
        m_Result |= Device->msg->Create(&msg_id[i], (Enum_MSG_TYPE) (MSG_TYPE_DATA | MSG_TYPE_FIFO), 64, MARK("test msg_id"));
        DEV_IF_LOGE((msg_id[i] <= 0), "Create fail");
    }
    m_Result |= Device->msg->Report();
    m_Result |= Device->pthread->Create(&m_pthread_1, pthread_new_run_1, NULL, MARK("m_pthread_Id1"));
    m_Result |= Device->pthread->Create(&m_pthread_2, pthread_new_run_2, NULL, MARK("m_pthread_Id2"));
    m_Result |= Device->pthread->Join(m_pthread_1, NULL);
    m_Result |= Device->pthread->Join(m_pthread_2, NULL);
    for (int i = 0; i < MSG_ID_NUM; i++) {
        if (msg_crc16Send[i] != msg_crc16Recv[i]) {
            DEV_LOGE("MSG TEST ERR!!!!!!!!!!!!!!!!!");
            m_Result |= RET_ERR;
        }
    }
    for (int i = 0; i < MSG_ID_NUM; i++) {
        m_Result |= Device->msg->Destroy(&msg_id[i]);
    }
    m_Result |= Device->pthread->Destroy(&m_pthread_1);
    m_Result |= Device->pthread->Destroy(&m_pthread_2);
    return m_Result;
}
