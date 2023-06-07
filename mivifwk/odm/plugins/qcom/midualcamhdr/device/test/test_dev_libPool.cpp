/**
 * @file       test_dev_libPool.cpp
 * @brief
 * @details
 * @author
 * @date       2021.07.03
 * @version    0.1
 * @note
 * @warning
 * @par        History:
 *
 */
#include "../device.h"

/**
 * Source code for com.xxx.xxx.xxx.so
 * Dev_libPool_define.h
 */
/***************************************************************************************************************/
typedef S64 (*DEV_INIT_ENTRY)(void);
typedef S64 (*DEV_PROCESS_ENTRY)(void);
typedef S64 (*DEV_DEINIT_ENTRY)(void);

typedef struct __DEV_LIB_ENTRY_OPS {
    U32 size;
    DEV_INIT_ENTRY Init;
    DEV_PROCESS_ENTRY Process;
    DEV_DEINIT_ENTRY Deinit;
} DEV_LIB_ENTRY_OPS;
/***************************************************************************************************************/

/**
 * Source code for com.xxx.xxx.xxx.so
 * com.xxx.xxx.xxx.cpp
 */
/***************************************************************************************************************/
#if 0
#include "Dev_libPool_define.h"
#endif

S64 DevLibXXX_Init(void) {
    DEV_LOGI("Lib TEST XXX INIT OK!!!");
    return 1;
}

S64 DevLibXXX_Process(void) {
    DEV_LOGI("Lib TEST XXX PROCESS OK!!!");
    return 1;
}

S64 DevLibXXX_Deinit(void) {
    DEV_LOGI("Lib TEST XXX DEINIT OK!!!");
    return 1;
}

DEV_PUBLIC_ENTRY void Entry(void *arg) {
    if (NULL != arg) {
        DEV_LIB_ENTRY_OPS *entryOps = (DEV_LIB_ENTRY_OPS*)arg;
        entryOps->size = sizeof(DEV_LIB_ENTRY_OPS);
        entryOps->Init = DevLibXXX_Init;
        entryOps->Process = DevLibXXX_Process;
        entryOps->Deinit = DevLibXXX_Deinit;
    }
}
/***************************************************************************************************************/


/**
 * Source code for framework
 * Dev_framework.cpp
 */
/***************************************************************************************************************/
#if 0
#include "Dev_libPool_define.h"
#endif


#if defined(WINDOWS_OS)
#define DEVICE_TEST_LIB_DIR    "resources"
#define DEVICE_TEST_LIB_FILE   "com.xiaomi.plugin.capbokeh.so"
#elif defined(LINUX_OS)
#define DEVICE_TEST_LIB_DIR    "resources"
#define DEVICE_TEST_LIB_FILE   "com.xiaomi.plugin.test.so"
#elif defined(ANDROID_OS)
#define DEVICE_TEST_LIB_DIR    "/vendor/lib64/camera/plugins"
#define DEVICE_TEST_LIB_FILE   "com.xiaomi.plugin.capbokeh.so"
#endif

static S32 m_Result = 0;
static S64 m_mutex_Id;
static S64 m_pthread_Id1;
static S64 m_pthread_Id2;
static S64 m_pthread_Id3;
static S64 m_pthread_Id4;
static DEV_LIBPOOL_LIST libList = {0};
static S32 libListOpsNum = 0;
static DEV_LIB_ENTRY_OPS libListOps[DEV_LIBPOOL_LIB_OPEN_MAX] = { {0}};

static void* pthread_new_run1(void *arg) {
    m_Result |= Device->libPool->OpenDir(DEVICE_TEST_LIB_DIR, &libList, MARK("libList"));
    for (int i = 0; i < libList.num; i++) {
        DEV_LOGI("LOAD LIB [%s][%p]", libList.name[i], libList.entry[i]);
        if (libList.entry[i] != NULL) {
            libList.entry[i](&libListOps[i]);
            if (libListOps[i].size != sizeof(DEV_LIB_ENTRY_OPS)) {
                m_Result |= RET_ERR;
                memset((void*)&libListOps[i], 0, sizeof(DEV_LIB_ENTRY_OPS));
                DEV_LOGE("LIB ENTRY OPS SIZE ERR");
            }
        }
    }
    libListOpsNum = libList.num;
    return NULL;
}

static void* pthread_new_run2(void *arg) {
    for (int i = 0; i < libListOpsNum; i++) {
        S32 ret = 0;
        ret += libListOps[i].Init();
        ret += libListOps[i].Process();
        ret += libListOps[i].Deinit();
        if (ret != 3) {
            DEV_LOGE("LIB POOL TEST ERR!!!!!!!!!!!!!!!!!");
            m_Result |= RET_ERR;
        }
    }
    return NULL;
}

static void* pthread_new_run3(void *arg) {
    m_Result |= Device->libPool->Report();
    m_Result |= Device->libPool->CloseDir(DEVICE_TEST_LIB_DIR);
    return NULL;
}

static void* pthread_new_run4(void *arg) {
    char dir[] = {DEVICE_TEST_LIB_DIR};
    char libName[] = {DEVICE_TEST_LIB_FILE};
    S32 ret = 0;
    DEV_LIBPOOL_ENTRY entry;
    DEV_LIB_ENTRY_OPS libListOps;
    m_Result |= Device->libPool->Open(dir, libName, &entry, MARK("entry"));
    if (entry != NULL) {
//        DEV_LOGI("LOAD LIB [%s][%p]", libName, entry);
        entry(&libListOps);
        if (libListOps.size != sizeof(DEV_LIB_ENTRY_OPS)) {
            m_Result |= RET_ERR;
            memset((void*)&libListOps, 0, sizeof(DEV_LIB_ENTRY_OPS));
            DEV_LOGE("LIB ENTRY OPS SIZE ERR");
        }
    } else {
        m_Result |= RET_ERR;
        return NULL;
    }
//    m_Result |= Device->libPool->Report();

    ret += libListOps.Init();
    ret += libListOps.Process();
    ret += libListOps.Deinit();
    if (ret != 3) {
        DEV_LOGE("LIB POOL TEST ERR!!!!!!!!!!!!!!!!!");
        m_Result |= RET_ERR;
    }
    m_Result |= Device->libPool->Close(dir, libName);

//    m_Result |= Device->libPool->Report();
    return NULL;
}

S32 test_dev_libPool(void) {
    m_Result = RET_OK;
    m_Result |= Device->mutex->Create(&m_mutex_Id, MARK("m_mutex_Id"));
    m_Result |= Device->pthread->Create(&m_pthread_Id1, pthread_new_run1, NULL, MARK("pthread_new_run1"));
    m_Result |= Device->pthread->Join(m_pthread_Id1, NULL);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id1);

    m_Result |= Device->pthread->Create(&m_pthread_Id2, pthread_new_run2, NULL, MARK("pthread_new_run2"));
    m_Result |= Device->pthread->Join(m_pthread_Id2, NULL);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id2);

    m_Result |= Device->pthread->Create(&m_pthread_Id3, pthread_new_run3, NULL, MARK("pthread_new_run3"));
    m_Result |= Device->pthread->Join(m_pthread_Id3, NULL);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id3);

    m_Result |= Device->pthread->Create(&m_pthread_Id4, pthread_new_run4, NULL, MARK("pthread_new_run4"));
    m_Result |= Device->pthread->Join(m_pthread_Id4, NULL);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id4);

    m_Result |= Device->mutex->Destroy(&m_mutex_Id);
    return m_Result;
}
/***************************************************************************************************************/
