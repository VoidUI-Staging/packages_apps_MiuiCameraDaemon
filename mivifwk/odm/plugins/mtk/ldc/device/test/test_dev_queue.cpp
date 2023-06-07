/**
 * @file		test_dev_queue.cpp
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
#ifdef __TEST_DEV_QUEUE__
#include "../device.h"

static S64 m_pthread_1;
static S64 m_pthread_2;

#define QUEUE_ID_NUM 255
static S64 queue_id[QUEUE_ID_NUM];
static S64 ret;
static U8 s_test_data[128];
static U8 r_test_data[128];
static S64 w = 0;
static S64 j = 0;

static void *pthread_new_run_1(void *arg)
{
    DEV_LOGI("Receive message start\n");
    while (1) {
        while (!Device->queue->Empty(queue_id[j])) {
            int queue_cnt = Device->queue->Count(queue_id[j]);
            //			DEV_LOGI("queueId=%ld  message=%d queue_len=%d\n", queue_id[j],
            // queue_cnt, queue_len);
            S64 recv_queue_len = 0;
            memset(r_test_data, 0x00, sizeof(r_test_data));
            recv_queue_len = Device->queue->Read(queue_id[j], r_test_data, sizeof(r_test_data));
            DEV_LOGI("Rece message id=%" PRIi64 ",len= %" PRIi64 ",%d", queue_id[j], recv_queue_len,
                     r_test_data[recv_queue_len - 1]);
            Device->system->SleepUs(1);
            //		DEV_LOG_HEX(r_test_data, recv_queue_len);
            j++;
            if (j >= QUEUE_ID_NUM) {
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
        for (w = 0; w < QUEUE_ID_NUM; w++) {
            memset(s_test_data, w, sizeof(s_test_data));
            ret = Device->queue->Write(queue_id[w], s_test_data, (S64)sizeof(s_test_data));
            DEV_IF_LOGE((ret != (S64)sizeof(s_test_data)), "QUEUEQ_Send fail");
            DEV_LOGI("Send message id=%" PRIi64 ",len= %" PRIi64 " ,%" PRIi64 "", queue_id[w],
                     (S64)sizeof(s_test_data), w);
            Device->system->SleepMs(2);
        }
    }
    return NULL;
}

void test_dev_queue(void)
{
    DEV_LOGI("QUEUE QUEUE test\n");
    S64 ret = RET_ERR;
    //	Device->queue->Init();
    for (int i = 0; i < QUEUE_ID_NUM; i++) {
        queue_id[i] = Device->queue->Create(1024);
        DEV_IF_LOGE((queue_id[i] <= 0), "Create fail");
    }
    m_pthread_1 = Device->pthread->Create(pthread_new_run_1, NULL);
    m_pthread_2 = Device->pthread->Create(pthread_new_run_2, NULL);
    //	ret = Device->queue->Destroy(queue_id);
    //	info_err("Md_QUEUEQ_Destroy fail\n", ret);
}

#endif //__TEST_DEV_QUEUE__
