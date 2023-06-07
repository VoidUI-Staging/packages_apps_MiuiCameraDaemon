/**
 * @file		test.h
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

#ifndef __DEV_TEST_H__
#define __DEV_TEST_H__

//#define __TEST_DEV_LOG__
//#define __TEST_DEV_CREATE__
//#define __TEST_DEV_TIME__
//#define __TEST_DEV_PTHREAD__
//#define __TEST_DEV_MUTEX__
//#define __TEST_DEV_SEM__
//#define __TEST_DEV_MSG__
//#define __TEST_DEV_QUEUE__
//#define __TEST_DEV_TIMER__

typedef struct __Dev_Test Dev_Test;
struct __Dev_Test
{
    void (*Init)(void);
    void (*Handler)(void);
    void (*Uninit)(void);
};

extern const Dev_Test m_dev_test;

#endif // __DEV_TEST_H__
