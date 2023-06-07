/**
 * @file       test_dev_memoryPool.cpp
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

static S64 m_pthread_Id1;
static S64 m_pthread_Id2;
static S64 m_pthread_Id3;
static S64 m_pthread_Id4;
static S64 m_pthread_Id5;
static S64 m_pthread_Id6;
static S64 m_pthread_Id7;
static S64 m_pthread_Id8;
static S64 m_pthread_Id9;
static S64 m_pthread_Id10;
static S64 m_pthread_Id11;
static S64 m_pthread_Id12;
static S64 m_pthread_Id13;
static S64 m_pthread_Id14;
static S64 m_pthread_Id15;

void *m_data[128] = {0};

#define TEST_MAX_TIMES      128

static void* pthread_new_run1(void *arg) {
    S32 ret = RET_OK;
    void *testData = Device->memoryPool->Malloc(128, MARK("TEST_DATA_MEMORY"));
    ret = Device->memoryPool->Free(testData);
    if (ret != RET_OK) {
        DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
        m_Result |= RET_ERR;
    }
#if 0
    //double free Test
    ret = Device->memoryPool->Free(testData);
    if (ret == RET_OK) {
        DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
        m_Result |= RET_ERR;
    }
#endif
    void *data[128] = {0};
    for (int i = 0; i < TEST_MAX_TIMES; i++) {
        data[i] = Device->memoryPool->Malloc(640, MARK("MT_TEST_MEMORY"));
        if (data[i] == NULL) {
            DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!![%d]", i);
            m_Result |= RET_ERR;
        }
    }
    testData = Device->memoryPool->Malloc(1024 * 1024 * 2,  MARK("TEST_DATA_MEMORY"));
//    Device->memoryPool->Report();
    ret = Device->memoryPool->Free(testData);
    if (ret != RET_OK) {
        DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
        m_Result |= RET_ERR;
    } else {
        testData = NULL;
    }
    for (int i = 0; i < TEST_MAX_TIMES; i++) {
        if (data[i] != NULL) {
            ret = Device->memoryPool->Free(data[i]);
            if (ret != RET_OK) {
                DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
                m_Result |= RET_ERR;
            }
        }
    }

    ////////////////////////////////////
#if 0
    U8 *testErrData = NULL;
    S32 size = 1024;
    testErrData = (U8*)Device->memoryPool->Malloc(size, MARK("MT_TEST_MEMORY"));
    if (testErrData == NULL) {
        m_Result |= RET_ERR;
    }
    for (int i = 0; i < size + 32; i++) {
        testErrData[i] = i;
    }
    if (testErrData != NULL) {
        ret = Device->memoryPool->Free(testErrData);
        if (ret == RET_OK) {
            DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
            m_Result |= RET_ERR;
        }
    }
#endif
    ////////////////////////////////////
    return NULL;
}

static void* pthread_new_run2(void *arg) {
    S32 ret = RET_OK;
    void *data[128] = {0};
    for (int i = 0; i < TEST_MAX_TIMES; i++) {
        data[i] = Device->memoryPool->Malloc(1024 * 1024 * 2, MARK("MT_TEST_MEMORY"));
        if (data[i] == NULL) {
            DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!![%d]", i);
            m_Result |= RET_ERR;
        }

        if (data[i] != NULL) {
            ret = Device->memoryPool->Free(data[i]);
            if (ret != RET_OK) {
                DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
                m_Result |= RET_ERR;
            }
        }
    }
//    Device->memoryPool->Report();
    if (ret != RET_OK) {
        DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
        m_Result |= RET_ERR;
    }
    return NULL;
}

static void* pthread_new_run3(void *arg) {
    for (int i = 0; i < TEST_MAX_TIMES; i++) {
        if (m_data[i] == NULL) {
            m_data[i] = Device->memoryPool->Malloc(1024 * 1024 * 3, MARK("MT_TEST_MEMORY"));
            if (m_data[i] == NULL) {
                DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!![%d]", i);
                m_Result |= RET_ERR;
                break;
            }
        }
    }
    return NULL;
}

static void* pthread_new_run4(void *arg) {
    S32 ret = RET_OK;
    int j = 0;
    int exit = 0;
    do {
        for (int i = 0; i < TEST_MAX_TIMES; i++) {
            if (m_data[i] != NULL) {
                ret = Device->memoryPool->Free(m_data[i]);
                if (ret != RET_OK) {
                    DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
                    m_Result |= RET_ERR;
                    break;
                } else {
                    m_data[i] = NULL;
                    j++;
                }
            }
            if (m_Result != RET_OK) {
                break;
            }
        }
        Device->system->SleepUs(1);
        exit++;
        if (exit > 1000000) {
            DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
            m_Result |= RET_ERR;
            break;
        }
    } while (j < TEST_MAX_TIMES);
    return NULL;
}

static void* pthread_new_run5(void *arg) {
    S32 ret = RET_OK;
    void *data[128] = {0};
    for (int i = 0; i < TEST_MAX_TIMES; i++) {
        data[i] = Device->memoryPool->Malloc(1024 * 1024 * 5, MARK("MT_TEST_MEMORY"));
        if (data[i] == NULL) {
            DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!![%d]", i);
            m_Result |= RET_ERR;
        }
    }
//    Device->memoryPool->Report();
    if (ret != RET_OK) {
        DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
        m_Result |= RET_ERR;
    }
    for (int i = 0; i < TEST_MAX_TIMES; i++) {
        if (data[i] != NULL) {
            ret = Device->memoryPool->Free(data[i]);
            if (ret != RET_OK) {
                DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
                m_Result |= RET_ERR;
            }
        }
    }

    for (int i = 0; i < TEST_MAX_TIMES; i++) {
        data[i] = Device->memoryPool->Malloc(1024 * 1024 * 5, MARK("MT_TEST_MEMORY"));
        if (data[i] == NULL) {
            DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!![%d]", i);
            m_Result |= RET_ERR;
        }
    }
//    Device->memoryPool->Report();
    if (ret != RET_OK) {
        DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
        m_Result |= RET_ERR;
    }
    for (int i = 0; i < TEST_MAX_TIMES; i++) {
        if (data[i] != NULL) {
            ret = Device->memoryPool->Free(data[i]);
            if (ret != RET_OK) {
                DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
                m_Result |= RET_ERR;
            }
        }
    }

    for (int i = 0; i < TEST_MAX_TIMES; i++) {
        data[i] = Device->memoryPool->Malloc(1024 * 1024 * 5, MARK("MT_TEST_MEMORY"));
        if (data[i] == NULL) {
            DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!![%d]", i);
            m_Result |= RET_ERR;
        }
    }
//    Device->memoryPool->Report();
    if (ret != RET_OK) {
        DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
        m_Result |= RET_ERR;
    }
    for (int i = TEST_MAX_TIMES - 1; i >= 0; i--) {
        if (data[i] != NULL) {
            ret = Device->memoryPool->Free(data[i]);
            if (ret != RET_OK) {
                DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
                m_Result |= RET_ERR;
            }
        }
    }

    return NULL;
}

static void* pthread_new_run6(void *arg) {
    S32 ret = RET_OK;
    void *data[128] = {0};
    for (int i = 0; i < TEST_MAX_TIMES; i++) {
        data[i] = Device->memoryPool->Malloc(1024 * 1024 * 6, MARK("MT_TEST_MEMORY"));
        if (data[i] == NULL) {
            DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!![%d]", i);
            m_Result |= RET_ERR;
        }
    }
//    Device->memoryPool->Report();
    if (ret != RET_OK) {
        DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
        m_Result |= RET_ERR;
    }
    for (int i = 0; i < TEST_MAX_TIMES; i++) {
        if (data[i] != NULL) {
            ret = Device->memoryPool->Free(data[i]);
            if (ret != RET_OK) {
                DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
                m_Result |= RET_ERR;
            }
        }
    }
    return NULL;
}

static void* pthread_new_run7(void *arg) {
    S32 ret = RET_OK;
    void *data[128] = {0};
    for (int i = 0; i < TEST_MAX_TIMES; i++) {
        data[i] = Device->memoryPool->Malloc(50, MARK("MT_TEST_MEMORY"));
        if (data[i] == NULL) {
            DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!![%d]", i);
            m_Result |= RET_ERR;
        }
    }
//    Device->memoryPool->Report();
    if (ret != RET_OK) {
        DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
        m_Result |= RET_ERR;
    }
    for (int i = 0; i < TEST_MAX_TIMES; i++) {
        if (data[i] != NULL) {
            ret = Device->memoryPool->Free(data[i]);
            if (ret != RET_OK) {
                DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
                m_Result |= RET_ERR;
            }
        }
    }
    return NULL;
}

static void* pthread_new_run8(void *arg) {
    S32 ret = RET_OK;
    void *data[128] = {0};
    for (int i = 0; i < TEST_MAX_TIMES; i++) {
        data[i] = Device->memoryPool->Malloc(1024 * 1024 * 8, MARK("MT_TEST_MEMORY"));
        if (data[i] == NULL) {
            DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!![%d]", i);
            m_Result |= RET_ERR;
        }
    }
//    Device->memoryPool->Report();
    if (ret != RET_OK) {
        DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
        m_Result |= RET_ERR;
    }
    for (int i = 0; i < TEST_MAX_TIMES; i++) {
        if (data[i] != NULL) {
            ret = Device->memoryPool->Free(data[i]);
            if (ret != RET_OK) {
                DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
                m_Result |= RET_ERR;
            }
        }
    }
    return NULL;
}

static void* pthread_new_run9(void *arg) {
    S32 ret = RET_OK;
    void *data[128] = {0};
    for (int i = 0; i < TEST_MAX_TIMES; i++) {
        data[i] = Device->memoryPool->Malloc(1024 * 1024 * 9, MARK("MT_TEST_MEMORY"));
        if (data[i] == NULL) {
            DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!![%d]", i);
            m_Result |= RET_ERR;
        }
    }
//    Device->memoryPool->Report();
    if (ret != RET_OK) {
        DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
        m_Result |= RET_ERR;
    }
    for (int i = 0; i < TEST_MAX_TIMES; i++) {
        if (data[i] != NULL) {
            ret = Device->memoryPool->Free(data[i]);
            if (ret != RET_OK) {
                DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
                m_Result |= RET_ERR;
            }
        }
    }
    return NULL;
}

static void* pthread_new_run10(void *arg) {
    S32 ret = RET_OK;
    void *data[128] = {0};
    for (int i = 0; i < TEST_MAX_TIMES; i++) {
        data[i] = Device->memoryPool->Malloc(1024 * 1024 * 10, MARK("MT_TEST_MEMORY"));
        if (data[i] == NULL) {
            DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!![%d]", i);
            m_Result |= RET_ERR;
        }
    }
//    Device->memoryPool->Report();
    if (ret != RET_OK) {
        DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
        m_Result |= RET_ERR;
    }
    for (int i = 0; i < TEST_MAX_TIMES; i++) {
        if (data[i] != NULL) {
            ret = Device->memoryPool->Free(data[i]);
            if (ret != RET_OK) {
                DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
                m_Result |= RET_ERR;
            }
        }
    }
    return NULL;
}

static void* pthread_new_run11(void *arg) {
    S32 ret = RET_OK;
    void *data[128] = {0};
    for (int i = 0; i < TEST_MAX_TIMES; i++) {
        data[i] = Device->memoryPool->Malloc((1024 * 1024 * 10) + 1, MARK("MT_TEST_MEMORY"));
        if (data[i] == NULL) {
            DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!![%d]", i);
            m_Result |= RET_ERR;
        }
    }
//    Device->memoryPool->Report();
    if (ret != RET_OK) {
        DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
        m_Result |= RET_ERR;
    }
    for (int i = 0; i < TEST_MAX_TIMES; i++) {
        if (data[i] != NULL) {
            ret = Device->memoryPool->Free(data[i]);
            if (ret != RET_OK) {
                DEV_LOGE("MEMORY POOL TEST ERR!!!!!!!!!!!!!!!!!");
                m_Result |= RET_ERR;
            }
        }
    }
    return NULL;
}

static void* pthread_new_run12(void *arg) {
    void *data = NULL;
    data = Device->memoryPool->Malloc(512, MARK("MT_TEST_MEMORY"));
    Device->memoryPool->Report();
    Device->memoryPool->Free(data);
    return NULL;
}

static void* pthread_new_run13(void *arg) {
    U8 *data = NULL;
    data = (U8*)Device->memoryPool->Malloc(10, MARK("MT_TEST_MEMORY"));
    data[11] = 0;
    Device->memoryPool->Free(data);
    return NULL;
}

static void* pthread_new_run14(void *arg) {
    U8 *data1 = NULL;
    U8 *data2 = NULL;
    data1 = (U8*)Device->memoryPool->Malloc(10, MARK("align TEST_10"));
    data2 = (U8*)Device->memoryPool->Malloc(3 * 1024 * 1024, MARK("align TEST_3M"));
    Device->memoryPool->Report();
    Device->memoryPool->Free(data1);
    Device->memoryPool->Free(data2);
    return NULL;
}

static void* pthread_new_run15(void *arg) {
    U8 *data1 = (U8*)Device->memoryPool->Malloc(512, MARK("align TEST_512_1"));
    U8 *data2 = (U8*)Device->memoryPool->Malloc(512, MARK("align TEST_512_1"));
    U8 *data3 = (U8*)Device->memoryPool->Malloc(512, MARK("align TEST_512_1"));
    U8 *data4 = (U8*)Device->memoryPool->Malloc(49 * 1024 * 1024, MARK("align TEST_49M"));
    U8 *data5 = (U8*)Device->memoryPool->Malloc(49 * 1024 * 1024, MARK("align TEST_49M"));
    U8 *data6 = (U8*)Device->memoryPool->Malloc(50 * 1024 * 1024, MARK("align TEST_50M"));
//    Device->memoryPool->Report();
    Device->memoryPool->Free(data1);
    Device->memoryPool->Free(data2);
    Device->memoryPool->Free(data3);
    Device->memoryPool->Free(data4);
    Device->memoryPool->Free(data5);
    Device->memoryPool->Free(data6);
    Device->memoryPool->Report();
    return NULL;
}

S32 test_dev_memoryPool(void) {
    m_Result = RET_OK;
    memset((void*)m_data, 0, sizeof(m_data));
    m_Result |= Device->pthread->Create(&m_pthread_Id1, pthread_new_run1, NULL, MARK("m_pthread_Id1"));
    m_Result |= Device->pthread->Create(&m_pthread_Id2, pthread_new_run2, NULL, MARK("m_pthread_Id2"));
    m_Result |= Device->pthread->Create(&m_pthread_Id3, pthread_new_run3, NULL, MARK("m_pthread_Id3"));
    m_Result |= Device->pthread->Create(&m_pthread_Id4, pthread_new_run4, NULL, MARK("m_pthread_Id4"));
    m_Result |= Device->pthread->Create(&m_pthread_Id5, pthread_new_run5, NULL, MARK("m_pthread_Id5"));
    m_Result |= Device->pthread->Create(&m_pthread_Id6, pthread_new_run6, NULL, MARK("m_pthread_Id6"));
    m_Result |= Device->pthread->Create(&m_pthread_Id7, pthread_new_run7, NULL, MARK("m_pthread_Id7"));
    m_Result |= Device->pthread->Create(&m_pthread_Id8, pthread_new_run8, NULL, MARK("m_pthread_Id8"));
    m_Result |= Device->pthread->Create(&m_pthread_Id9, pthread_new_run9, NULL, MARK("m_pthread_Id9"));
    m_Result |= Device->pthread->Create(&m_pthread_Id10, pthread_new_run10, NULL, MARK("m_pthread_Id10"));
    m_Result |= Device->pthread->Create(&m_pthread_Id11, pthread_new_run11, NULL, MARK("m_pthread_Id11"));
    m_Result |= Device->pthread->Create(&m_pthread_Id12, pthread_new_run12, NULL, MARK("m_pthread_Id12"));
    m_Result |= Device->pthread->Create(&m_pthread_Id13, pthread_new_run13, NULL, MARK("overflow_TEST"));
    m_Result |= Device->pthread->Create(&m_pthread_Id14, pthread_new_run14, NULL, MARK("align TEST"));
    m_Result |= Device->pthread->Create(&m_pthread_Id15, pthread_new_run15, NULL, MARK("free TEST"));
    m_Result |= Device->pthread->Join(m_pthread_Id1, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id2, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id3, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id4, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id5, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id6, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id7, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id8, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id9, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id10, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id11, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id12, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id13, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id14, NULL);
    m_Result |= Device->pthread->Join(m_pthread_Id15, NULL);

    Device->memoryPool->Report();
    m_Result |= Device->pthread->Destroy(&m_pthread_Id1);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id2);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id3);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id4);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id5);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id6);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id7);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id8);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id9);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id10);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id11);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id12);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id13);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id14);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id15);
    return m_Result;
}
