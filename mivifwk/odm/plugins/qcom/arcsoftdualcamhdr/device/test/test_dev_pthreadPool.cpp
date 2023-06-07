/**
 * @file        test_dev_pthreadPool.cpp
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

static S64 m_mutex_Id;
static S64 m_semaphore_Id;
static S64 m_pthread_Id;
static S64 m_pthread_Id0_1;
static S64 m_pthread_Id0_2;
static S64 m_pthread_Id0_3;
static S64 m_pthread_Id0_4;
static S64 m_pthread_Id0_5;
static S64 m_pthread_Id0_6;
static S64 m_pthread_Id0_7;
static S64 m_pthread_Id0_8;
static S64 m_pthread_Id0_9;
static S64 m_pthread_Id0_10;
static S32 testAdd = 0;
static S32 testNoLock = 0;
static S32 testNoLockResult = 0;
static S32 testLock = 0;
static S32 testLockResult = 0;

#define TEST_ADD_OK     (20)

static void* pthread_new_run1(void *arg1, void *arg2) {
    S32 i = 0;
    for (int w = 0; w < TEST_ADD_OK * 5; w++) {
        i++;
        testNoLock++;
        m_Result |= Device->system->SleepUs(1);
        testNoLock--;
        if (testNoLock != 0) {
            testNoLockResult++;
        }
    }
    return NULL;
}

static void* pthread_new_run2(void *arg1, void *arg2) {
    S32 i = 0;
    for (int w = 0; w < TEST_ADD_OK * 5; w++) {
        i++;
        testNoLock++;
        m_Result |= Device->system->SleepUs(1);
        testNoLock--;
        if (testNoLock != 0) {
            testNoLockResult++;
        }
    }
    return NULL;
}

static void* pthread_new_run3(void *arg1, void *arg2) {
    S32 i = 0;
    for (int w = 0; w < TEST_ADD_OK * 5; w++) {
        i++;
        testNoLock++;
        m_Result |= Device->system->SleepUs(1);
        testNoLock--;
        if (testNoLock != 0) {
            testNoLockResult++;
        }
    }
    return NULL;
}

static void* pthread_new_run4(void *arg1, void *arg2) {
    S32 i = 0;
    for (int w = 0; w < TEST_ADD_OK * 5; w++) {
        i++;
        testNoLock++;
        m_Result |= Device->system->SleepUs(1);
        testNoLock--;
        if (testNoLock != 0) {
            testNoLockResult++;
        }
    }
    return NULL;
}

static void* pthread_new_run5(void *arg1, void *arg2) {
    S32 i = 0;
    for (int w = 0; w < TEST_ADD_OK * 5; w++) {
        i++;
        testNoLock++;
        m_Result |= Device->system->SleepUs(1);
        testNoLock--;
        if (testNoLock != 0) {
            testNoLockResult++;
        }
    }
    return NULL;
}

static void* pthread_new_run6(void *arg1, void *arg2) {
    S32 i = 0;
    for (int w = 0; w < TEST_ADD_OK * 5; w++) {
        i++;
        testNoLock++;
        m_Result |= Device->system->SleepUs(1);
        testNoLock--;
        if (testNoLock != 0) {
            testNoLockResult++;
        }
    }
    return NULL;
}

static void* pthread_new_run7(void *arg1, void *arg2) {
    S32 i = 0;
    for (int w = 0; w < TEST_ADD_OK * 5; w++) {
        i++;
        testNoLock++;
        m_Result |= Device->system->SleepUs(1);
        testNoLock--;
        if (testNoLock != 0) {
            testNoLockResult++;
        }
    }
    return NULL;
}

static void* pthread_new_run8(void *arg1, void *arg2) {
    S32 i = 0;
    for (int w = 0; w < TEST_ADD_OK * 5; w++) {
        i++;
        testNoLock++;
        m_Result |= Device->system->SleepUs(1);
        testNoLock--;
        if (testNoLock != 0) {
            testNoLockResult++;
        }
    }
    return NULL;
}

static void* pthread_new_run9(void *arg1, void *arg2) {
    S32 i = 0;
    for (int w = 0; w < TEST_ADD_OK * 5; w++) {
        i++;
        testNoLock++;
        m_Result |= Device->system->SleepUs(1);
        testNoLock--;
        if (testNoLock != 0) {
            testNoLockResult++;
        }
    }
    return NULL;
}

static void* pthread_new_run10(void *arg1, void *arg2) {
    S32 i = 0;
    for (int w = 0; w < TEST_ADD_OK * 5; w++) {
        i++;
        testNoLock++;
        m_Result |= Device->system->SleepUs(1);
        testNoLock--;
        if (testNoLock != 0) {
            testNoLockResult++;
        }
    }
    return NULL;
}

static void* pthread_new_run11(void *arg1, void *arg2) {
    S32 i = 0;
    for (int w = 0; w < TEST_ADD_OK * 5; w++) {
        i++;
        m_Result |= Device->mutex->Lock(m_mutex_Id);
        testAdd++;
        testLock++;
        m_Result |= Device->system->SleepUs(1);
        testLock--;
        if (testLock != 0) {
            testLockResult++;
        }
        m_Result |= Device->mutex->Unlock(m_mutex_Id);
//        DEV_LOGI("in new pthread=%ld %ld", testAdd, ParallelTestAdd2);
    }
    return NULL;
}

static void* pthread_new_run12(void *arg1, void *arg2) {
    S32 i = 0;
    for (int w = 0; w < TEST_ADD_OK * 5; w++) {
        i++;
        m_Result |= Device->mutex->Lock(m_mutex_Id);
        testAdd++;
        testLock++;
        m_Result |= Device->system->SleepUs(1);
        testLock--;
        if (testLock != 0) {
            testLockResult++;
        }
        m_Result |= Device->mutex->Unlock(m_mutex_Id);
//        DEV_LOGI("in new pthread=%ld %ld", testAdd, ParallelTestAdd2);
    }
    return NULL;
}

static void* pthread_new_run13(void *arg1, void *arg2) {
    S32 i = 0;
    for (int w = 0; w < TEST_ADD_OK * 5; w++) {
        i++;
        m_Result |= Device->mutex->Lock(m_mutex_Id);
        testAdd++;
        testLock++;
        m_Result |= Device->system->SleepUs(1);
        testLock--;
        if (testLock != 0) {
            testLockResult++;
        }
        m_Result |= Device->mutex->Unlock(m_mutex_Id);
//        DEV_LOGI("in new pthread=%ld %ld", testAdd, ParallelTestAdd2);
    }
    return NULL;
}

static void* pthread_new_run14(void *arg1, void *arg2) {
    S32 i = 0;
    for (int w = 0; w < TEST_ADD_OK * 5; w++) {
        i++;
        m_Result |= Device->mutex->Lock(m_mutex_Id);
        testAdd++;
        testLock++;
        m_Result |= Device->system->SleepUs(1);
        testLock--;
        if (testLock != 0) {
            testLockResult++;
        }
        m_Result |= Device->mutex->Unlock(m_mutex_Id);
//        DEV_LOGI("in new pthread=%ld %ld", testAdd, ParallelTestAdd2);
    }
    return NULL;
}

static void* pthread_new_run15(void *arg1, void *arg2) {
    S32 i = 0;
    for (int w = 0; w < TEST_ADD_OK * 5; w++) {
        i++;
        m_Result |= Device->mutex->Lock(m_mutex_Id);
        testAdd++;
        testLock++;
        m_Result |= Device->system->SleepUs(1);
        testLock--;
        if (testLock != 0) {
            testLockResult++;
        }
        m_Result |= Device->mutex->Unlock(m_mutex_Id);
//        DEV_LOGI("in new pthread=%ld %ld", testAdd, ParallelTestAdd2);
    }
    return NULL;
}

static void* pthread_new_run16(void *arg1, void *arg2) {
    S32 i = 0;
    for (int w = 0; w < TEST_ADD_OK * 5; w++) {
        i++;
        m_Result |= Device->mutex->Lock(m_mutex_Id);
        testAdd++;
        testLock++;
        m_Result |= Device->system->SleepUs(1);
        testLock--;
        if (testLock != 0) {
            testLockResult++;
        }
        m_Result |= Device->mutex->Unlock(m_mutex_Id);
//        DEV_LOGI("in new pthread=%ld %ld", testAdd, ParallelTestAdd2);
    }
    return NULL;
}

static void* pthread_new_run17(void *arg1, void *arg2) {
    S32 i = 0;
    for (int w = 0; w < TEST_ADD_OK * 5; w++) {
        i++;
        m_Result |= Device->mutex->Lock(m_mutex_Id);
        testAdd++;
        testLock++;
        m_Result |= Device->system->SleepUs(1);
        testLock--;
        if (testLock != 0) {
            testLockResult++;
        }
        m_Result |= Device->mutex->Unlock(m_mutex_Id);
//        DEV_LOGI("in new pthread=%ld %ld", testAdd, ParallelTestAdd2);
    }
    return NULL;
}

static void* pthread_new_run18(void *arg1, void *arg2) {
    S32 i = 0;
    for (int w = 0; w < TEST_ADD_OK * 5; w++) {
        i++;
        m_Result |= Device->mutex->Lock(m_mutex_Id);
        testAdd++;
        testLock++;
        m_Result |= Device->system->SleepUs(1);
        testLock--;
        if (testLock != 0) {
            testLockResult++;
        }
        m_Result |= Device->mutex->Unlock(m_mutex_Id);
//        DEV_LOGI("in new pthread=%ld %ld", testAdd, ParallelTestAdd2);
    }
    return NULL;
}

static void* pthread_new_run19(void *arg1, void *arg2) {
    S32 i = 0;
    for (int w = 0; w < TEST_ADD_OK * 5; w++) {
        i++;
        m_Result |= Device->mutex->Lock(m_mutex_Id);
        testAdd++;
        testLock++;
        m_Result |= Device->system->SleepUs(1);
        testLock--;
        if (testLock != 0) {
            testLockResult++;
        }
        m_Result |= Device->mutex->Unlock(m_mutex_Id);
//        DEV_LOGI("in new pthread=%ld %ld", testAdd, ParallelTestAdd2);
    }
    return NULL;
}

static void* pthread_new_run20(void *arg1, void *arg2) {
    S32 i = 0;
    for (int w = 0; w < TEST_ADD_OK * 5; w++) {
        i++;
        m_Result |= Device->mutex->Lock(m_mutex_Id);
        testAdd++;
        testLock++;
        m_Result |= Device->system->SleepUs(1);
        testLock--;
        if (testLock != 0) {
            testLockResult++;
        }
        m_Result |= Device->mutex->Unlock(m_mutex_Id);
//        DEV_LOGI("in new pthread=%ld %ld", testAdd, ParallelTestAdd2);
    }
    return NULL;
}

static int pthread_test_add = 0;
static void* pthread_new_test0_1(void *arg1, void *arg2) {
    pthread_test_add++;
    return NULL;
}
static void* pthread_new_test0_2(void *arg1, void *arg2) {
    pthread_test_add++;
    return NULL;
}
static void* pthread_new_test0_3(void *arg1, void *arg2) {
    pthread_test_add++;
    return NULL;
}
static void* pthread_new_test0_4(void *arg1, void *arg2) {
    pthread_test_add++;
    return NULL;
}
static void* pthread_new_test0_5(void *arg1, void *arg2) {
    pthread_test_add++;
    return NULL;
}
static void* pthread_new_test0_6(void *arg1, void *arg2) {
    pthread_test_add++;
    return NULL;
}
static void* pthread_new_test0_7(void *arg1, void *arg2) {
    pthread_test_add++;
    return NULL;
}
static void* pthread_new_test0_8(void *arg1, void *arg2) {
    pthread_test_add++;
    return NULL;
}
static void* pthread_new_test0_9(void *arg1, void *arg2) {
    pthread_test_add++;
    return NULL;
}
static void* pthread_new_test0_10(void *arg1, void *arg2) {
    pthread_test_add++;
    return NULL;
}

static void* pthread_new_run0_1(void *arg) {
    m_Result |= Device->pthreadPool->Post(pthread_new_test0_1, NULL, NULL, MARK("pthread_new_test0_1"));
    m_Result |= Device->pthread->Exit(0);
    return NULL;
}

static void* pthread_new_run0_2(void *arg) {
    m_Result |= Device->pthreadPool->Post(pthread_new_test0_2, NULL, NULL, MARK("pthread_new_test0_2"));
    m_Result |= Device->pthread->Exit(0);
    return NULL;
}

static void* pthread_new_run0_3(void *arg) {
    m_Result |= Device->pthreadPool->Post(pthread_new_test0_3, NULL, NULL, MARK("pthread_new_test0_3"));
    m_Result |= Device->pthread->Exit(0);
    return NULL;
}

static void* pthread_new_run0_4(void *arg) {
    m_Result |= Device->pthreadPool->Post(pthread_new_test0_4, NULL, NULL, MARK("pthread_new_test0_4"));
    m_Result |= Device->pthread->Exit(0);
    return NULL;
}

static void* pthread_new_run0_5(void *arg) {
    m_Result |= Device->pthreadPool->Post(pthread_new_test0_5, NULL, NULL, MARK("pthread_new_test0_5"));
    m_Result |= Device->pthread->Exit(0);
    return NULL;
}

static void* pthread_new_run0_6(void *arg) {
    m_Result |= Device->pthreadPool->Post(pthread_new_test0_6, NULL, NULL, MARK("pthread_new_test0_6"));
    m_Result |= Device->pthread->Exit(0);
    return NULL;
}

static void* pthread_new_run0_7(void *arg) {
    m_Result |= Device->pthreadPool->Post(pthread_new_test0_7, NULL, NULL, MARK("pthread_new_test0_7"));
    m_Result |= Device->pthread->Exit(0);
    return NULL;
}

static void* pthread_new_run0_8(void *arg) {
    m_Result |= Device->pthreadPool->Post(pthread_new_test0_8, NULL, NULL, MARK("pthread_new_test0_8"));
    m_Result |= Device->pthread->Exit(0);
    return NULL;
}

static void* pthread_new_run0_9(void *arg) {
    m_Result |= Device->pthreadPool->Post(pthread_new_test0_9, NULL, NULL, MARK("pthread_new_test0_9"));
    m_Result |= Device->pthread->Exit(0);
    return NULL;
}

static void* pthread_new_run0_10(void *arg) {
    m_Result |= Device->pthreadPool->Post(pthread_new_test0_10, NULL, NULL, MARK("pthread_new_test0_10"));
    m_Result |= Device->pthread->Exit(0);
    return NULL;
}


static void* pthread_new_run(void *arg) {
    S32 ret = Device->semaphore->TimedWait(m_semaphore_Id, (1 * 20) + 2000);
    if (ret != RET_ERR) {
        DEV_LOGE("SEMAPHORE TEST ERR!!!!!!!!!!!!!!!!!%d", ret);
        m_Result |= RET_ERR;
    }
    if (testLock != 0) {
        DEV_LOGE("PTHREAD POOL TEST ERR!!!!!!!!!!!!!!!!![%d]", testLock);
        m_Result |= RET_ERR;
    }
    if (testLockResult != 0) {
        DEV_LOGE("PTHREAD POOL TEST ERR!!!!!!!!!!!!!!!!![%d]", testLock);
        m_Result |= RET_ERR;
    }
    if (testNoLockResult == 0) {
        DEV_LOGE("PTHREAD POOL TEST ERR!!!!!!!!!!!!!!!!![%d]", testLock);
        m_Result |= RET_ERR;
    }
    if (testAdd != TEST_ADD_OK * 5 * 10) {
        DEV_LOGE("PTHREAD POOL TEST ERR!!!!!!!!!!!!!!!!![%d]", testLock);
        m_Result |= RET_ERR;
    }
    if (testNoLock == TEST_ADD_OK * 5 * 10) {
        DEV_LOGE("PTHREAD POOL TEST ERR!!!!!!!!!!!!!!!!![%d]", testNoLock);
        m_Result |= RET_ERR;
    }
    if (pthread_test_add != 10) {
        DEV_LOGE("PTHREAD POOL TEST ERR!!!!!!!!!!!!!!!!![%d]", testNoLock);
        m_Result |= RET_ERR;
    }
    m_Result |= Device->pthread->Exit(0);
    return NULL;
}
S32 test_dev_pthreadPool(void) {
    m_Result = RET_OK;
    testAdd = 0;
    testLock = 0;
    testLockResult = 0;
    testNoLock = 0;
    testNoLockResult = 0;
    pthread_test_add = 0;
    m_Result |= Device->mutex->Create(&m_mutex_Id, MARK("m_mutex_Id"));
    m_Result |= Device->semaphore->Create(&m_semaphore_Id, MARK("m_semaphore_Id"));
    m_Result |= Device->pthreadPool->Post(pthread_new_run1, NULL, NULL, MARK("pthread_new_run1"));
    m_Result |= Device->pthreadPool->Post(pthread_new_run2, NULL, NULL, MARK("pthread_new_run2"));
    m_Result |= Device->pthreadPool->Post(pthread_new_run3, NULL, NULL, MARK("pthread_new_run3"));
    m_Result |= Device->pthreadPool->Post(pthread_new_run4, NULL, NULL, MARK("pthread_new_run4"));
    m_Result |= Device->pthreadPool->Post(pthread_new_run5, NULL, NULL, MARK("pthread_new_run5"));
    m_Result |= Device->pthreadPool->Post(pthread_new_run6, NULL, NULL, MARK("pthread_new_run6"));
    m_Result |= Device->pthreadPool->Post(pthread_new_run7, NULL, NULL, MARK("pthread_new_run7"));
    m_Result |= Device->pthreadPool->Post(pthread_new_run8, NULL, NULL, MARK("pthread_new_run8"));
    m_Result |= Device->pthreadPool->Post(pthread_new_run9, NULL, NULL, MARK("pthread_new_run9"));
    m_Result |= Device->pthreadPool->Post(pthread_new_run10, NULL, NULL, MARK("pthread_new_run10"));
    m_Result |= Device->pthreadPool->Post(pthread_new_run11, NULL, NULL, MARK("pthread_new_run11"));
//    m_Result |= Device->pthreadPool->Report();
    m_Result |= Device->pthreadPool->Post(pthread_new_run12, NULL, NULL, MARK("pthread_new_run12"));
    m_Result |= Device->pthreadPool->Post(pthread_new_run13, NULL, NULL, MARK("pthread_new_run13"));
    m_Result |= Device->pthreadPool->Post(pthread_new_run14, NULL, NULL, MARK("pthread_new_run14"));
    m_Result |= Device->pthreadPool->Post(pthread_new_run15, NULL, NULL, MARK("pthread_new_run15"));
    m_Result |= Device->pthreadPool->Post(pthread_new_run16, NULL, NULL, MARK("pthread_new_run16"));
    m_Result |= Device->pthreadPool->Post(pthread_new_run17, NULL, NULL, MARK("pthread_new_run17"));
    m_Result |= Device->pthreadPool->Post(pthread_new_run18, NULL, NULL, MARK("pthread_new_run18"));
    m_Result |= Device->pthreadPool->Post(pthread_new_run19, NULL, NULL, MARK("pthread_new_run19"));
    m_Result |= Device->pthreadPool->Post(pthread_new_run20, NULL, NULL, MARK("pthread_new_run20"));
//    m_Result |= Device->pthreadPool->Flush();
//    m_Result |= Device->pthreadPool->Report();
    m_Result |= Device->pthread->Create(&m_pthread_Id0_1, pthread_new_run0_1, NULL, MARK("m_pthread_Id0_1"));
    m_Result |= Device->pthread->Create(&m_pthread_Id0_2, pthread_new_run0_2, NULL, MARK("m_pthread_Id0_2"));
    m_Result |= Device->pthread->Create(&m_pthread_Id0_3, pthread_new_run0_3, NULL, MARK("m_pthread_Id0_3"));
    m_Result |= Device->pthread->Create(&m_pthread_Id0_4, pthread_new_run0_4, NULL, MARK("m_pthread_Id0_4"));
    m_Result |= Device->pthread->Create(&m_pthread_Id0_5, pthread_new_run0_5, NULL, MARK("m_pthread_Id0_5"));
    m_Result |= Device->pthread->Create(&m_pthread_Id0_6, pthread_new_run0_6, NULL, MARK("m_pthread_Id0_6"));
    m_Result |= Device->pthread->Create(&m_pthread_Id0_7, pthread_new_run0_7, NULL, MARK("m_pthread_Id0_7"));
    m_Result |= Device->pthread->Create(&m_pthread_Id0_8, pthread_new_run0_8, NULL, MARK("m_pthread_Id0_8"));
    m_Result |= Device->pthread->Create(&m_pthread_Id0_9, pthread_new_run0_9, NULL, MARK("m_pthread_Id0_9"));
    m_Result |= Device->pthread->Create(&m_pthread_Id0_10, pthread_new_run0_10, NULL, MARK("m_pthread_Id0_10"));

    m_Result |= Device->pthread->Join(m_pthread_Id0_1, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id0_2, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id0_3, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id0_4, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id0_5, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id0_6, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id0_7, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id0_8, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id0_9, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id0_10, NULL);

    m_Result |= Device->pthread->Destroy(&m_pthread_Id0_1);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id0_2);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id0_3);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id0_4);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id0_5);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id0_6);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id0_7);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id0_8);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id0_9);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id0_10);

    m_Result |= Device->pthread->Create(&m_pthread_Id, pthread_new_run, NULL, MARK("m_pthread_Id"));
    m_Result |= Device->pthread->Join(m_pthread_Id, NULL);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id);

    m_Result |= Device->semaphore->Destroy(&m_semaphore_Id);
    m_Result |= Device->mutex->Destroy(&m_mutex_Id);
    m_Result |= Device->pthreadPool->Report();
    return m_Result;
}
