/**
 * @file		test_dev_msg.cpp
 * @brief
 * @details
 * @author
 * @date        2019.07.03
 * @version		0.1
 * @note
 * @warning
 * @par			History:
 *
 */

#include "test.h"
#ifdef __TEST_DEV_MSG__
#include "../device.h"

static S64 m_pthread_1;
static S64 m_pthread_2;

#define MSG_ID_NUM 255
static S64 msg_id[MSG_ID_NUM];
static S64 ret;
static U8 s_test_data[128];
static U8 r_test_data[128];
static S64 w = 0;
static S64 j = 0;

static void *pthread_new_run_1(void *arg)
{
    DEV_LOGI("Receive message start\n");
    while (1) {
        while (!Device->msg->Empty(msg_id[j])) {
            int msg_cnt = Device->msg->Count(msg_id[j]);
            int msg_len = Device->msg->Length(msg_id[j]);
            //			DEV_LOGI("msgId=%ld  message=%d msg_len=%d\n", msg_id[j], msg_cnt,
            // msg_len);
            S64 recv_msg_len = 0;
            memset(r_test_data, 0x00, sizeof(r_test_data));
            recv_msg_len = Device->msg->Read(msg_id[j], r_test_data, sizeof(r_test_data));
            DEV_LOGI("Rece message id=%" PRIi64 ",len= %" PRIi64 ",%d", msg_id[j], recv_msg_len,
                     r_test_data[recv_msg_len - 1]);
            Device->system->SleepMs(10);
            //		DEV_LOG_HEX(r_test_data, recv_msg_len);
            j++;
            if (j >= MSG_ID_NUM) {
                j = 0;
            }
        }
        Device->system->SleepMs(100);
    }
    return NULL;
}

static void *pthread_new_run_2(void *arg)
{
    DEV_LOGI("Send message start\n");
    while (1) {
        for (w = 0; w < MSG_ID_NUM; w++) {
            memset(s_test_data, w, sizeof(s_test_data));
            ret = Device->msg->Write(msg_id[w], s_test_data, (S64)sizeof(s_test_data));
            DEV_IF_LOGE((ret != (S64)sizeof(s_test_data)), "MSGQ_Send fail");
            DEV_LOGI("Send message id=%" PRIi64 ",len= %" PRIi64 " ,%" PRIi64 "", msg_id[w],
                     (S64)sizeof(s_test_data), w);
            Device->system->SleepMs(10);
        }
    }
    return NULL;
}

void test_dev_msg(void)
{
    DEV_LOGI("MSG QUEUE test\n");
    S64 ret = RET_ERR;
    //	Device->msg->Init();
    for (int i = 0; i < MSG_ID_NUM; i++) {
        msg_id[i] = Device->msg->Create((Enum_MSG_TYPE)(MSG_TYPE_DATA | MSG_TYPE_FIFO), 64);
        DEV_IF_LOGE((msg_id[i] <= 0), "Create fail");
    }
    m_pthread_1 = Device->pthread->Create(pthread_new_run_1, NULL);
    m_pthread_2 = Device->pthread->Create(pthread_new_run_2, NULL);
    //	ret = Device->msg->Destroy(msg_id);
    //	info_err("Md_MSGQ_Destroy fail\n", ret);
}

#endif //__TEST_DEV_MSG__
