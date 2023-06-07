/**
 * @file		test_dev_log.c
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
#ifdef __TEST_DEV_LOG__
#include "../device.h"

void test_dev_log(void)
{
    U8 p[10] = {0xFF, 0x55, 0x33};
    DEV_LOGI("TEST info OUT =%d\n", 10);
    DEV_LOG_HEX(p, (int)sizeof(p));
    DEV_IF_LOGE((p != NULL), "TEST if LOGE !");
    DEV_LOGE("aaaaaaaaaaaaaaaaaaaaaa");
    DEV_IF_LOGE_RETURN((p != NULL), "TEST if LOGE RETURN !");
}

#endif //__TEST_DEV_LOG__
