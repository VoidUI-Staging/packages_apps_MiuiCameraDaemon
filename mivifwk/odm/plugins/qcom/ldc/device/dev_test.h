/**
 * @file        test.h
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

#ifndef __DEV_TEST_H__
#define __DEV_TEST_H__

#include "dev_type.h"
#include "dev_stdlib.h"

#define DEV_TEST_NUM_MAX                        (128)
#define DEV_TESTS_NAEM_MAX                      (64)

typedef struct __Dev_TestNode Dev_TestNode;
struct __Dev_TestNode {
    S32 (*function)(void);
    char name[DEV_TESTS_NAEM_MAX];
    S64 useTime;
    U8 status;
};

extern Dev_TestNode __m_devTestsTable__[DEV_TEST_NUM_MAX];
extern S32 __devTestIndex__;

#define ADD_TESTS_START   static inline void DevTest_Assembly(void) {
#define ADD_TESTS_END  }

#define ADD_TESTS(functionName)                                \
    extern S32 functionName(void);                                      \
    __m_devTestsTable__[__devTestIndex__].function = functionName;      \
    Dev_sprintf(__m_devTestsTable__[__devTestIndex__].name,"%s",#functionName);\
    __m_devTestsTable__[__devTestIndex__].useTime = 0;                  \
    __m_devTestsTable__[__devTestIndex__].status = 0;                   \
    if (__devTestIndex__ >= 0 && __devTestIndex__ < DEV_TEST_NUM_MAX) { \
        __devTestIndex__++;                                             \
    }                                                                   \

typedef struct __Dev_Test Dev_Test;
struct __Dev_Test {
    void (*Init)(void);
    void (*Handler)(void);
    S32 (*Report)(void);
    void (*Deinit)(void);
};

extern const Dev_Test m_dev_test;

#endif // __DEV_TEST_H__
