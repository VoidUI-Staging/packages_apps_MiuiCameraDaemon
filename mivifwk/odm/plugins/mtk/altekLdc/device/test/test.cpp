/**
 * @file		test.c
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

#include "../config.h"

void DevTest_Init(void)
{
#if (DEBUG_TEST_CODE == TRUE)
#ifdef __TEST_DEV_LOG__
    extern void test_dev_log(void);
    test_dev_log();
#endif
#ifdef __TEST_DEV_CREATE__
    extern void test_dev_create(void);
    test_dev_create();
#endif
#ifdef __TEST_DEV_TIME__
    extern void test_dev_time(void);
    test_dev_time();
#endif
#ifdef __TEST_DEV_PTHREAD__
    extern void test_dev_pthread(void);
    test_dev_pthread();
#endif
#ifdef __TEST_DEV_MUTEX__
    extern void test_dev_mutex(void);
    test_dev_mutex();
#endif
#ifdef __TEST_DEV_SEM__
    extern void test_dev_sem(void);
    test_dev_sem();
#endif
#ifdef __TEST_DEV_MSG__
    extern void test_dev_msg(void);
    test_dev_msg();
#endif
#ifdef __TEST_DEV_QUEUE__
    extern void test_dev_queue(void);
    test_dev_queue();
#endif
#ifdef __TEST_DEV_TIMER__
    extern void test_dev_timer(void);
    test_dev_timer();
#endif
#endif
}

void DevTest_Handler(void)
{
#if (DEBUG_TEST_CODE == TRUE)
#endif
}

void DevTest_Uninit(void)
{
#if (DEBUG_TEST_CODE == TRUE)
#endif
}

const Dev_Test m_dev_test = {
    .Init = DevTest_Init,
    .Handler = DevTest_Handler,
    .Uninit = DevTest_Uninit,
};
