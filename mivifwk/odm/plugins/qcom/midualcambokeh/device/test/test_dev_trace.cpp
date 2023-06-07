/**
 * @file        test_dev_trace.cpp
 * @brief
 * @details
 * @author
 * @date        2021.06.03
 * @version     0.1
 * @note
 * @warning
 * @par         History:
 *
 *./record_android_trace -o trace_file.perfetto-trace -t 10s -b 512mb sched freq idle am wm gfx view binder_driver hal dalvik camera input res memory --app com.android.camera
 */
#include "../device.h"

static S32 m_Result = 0;

#define TEST_MAX_TIMES      5

static S64 m_pthread_Id;
static S64 g_traceTestId_1 = 0;
static S64 g_traceTestId_2 = 0;

static void* pthread_new_run(void *arg) {
    m_Result |= Device->trace->Begin(g_traceTestId_1, MARK("MY_TEST trace TEST1-->"));
    m_Result |= Device->system->SleepMs(150);
    m_Result |= Device->trace->End(g_traceTestId_1, MARK("MY_TEST trace TEST1-->"));

    for (int w = 0; w < TEST_MAX_TIMES; w++) {
        m_Result |= Device->trace->AsyncBegin(g_traceTestId_1,MARK("MY_TEST trace TEST2-->"),8);
        m_Result |= Device->system->SleepMs(1);
        m_Result |= Device->trace->AsyncEnd(g_traceTestId_1,MARK("MY_TEST trace TEST2-->"),8);
        m_Result |= Device->trace->Message(g_traceTestId_1,MARK("Message MY_TEST trace TEST3-->"));
        m_Result |= Device->trace->AsyncBegin(g_traceTestId_2,MARK("MY_TEST trace TEST3-->"),8);
        m_Result |= Device->system->SleepMs(12);
        m_Result |= Device->trace->AsyncEnd(g_traceTestId_1,MARK("MY_TEST trace TEST3-->"),8);
        m_Result |= Device->trace->Message(g_traceTestId_1,MARK("MY_TEST trace Message-->"));
    }
    m_Result |= Device->trace->Message(g_traceTestId_1,MARK("MY_TEST trace TEST4-->"));
    m_Result |= Device->system->SleepMs(1);
    m_Result |= Device->pthread->Exit(0);
    return NULL;
}

S32 test_dev_trace(void) {
    m_Result = 0;
    m_Result |= Device->trace->Create(&g_traceTestId_1, MARK("g_traceTestId_1"));
    m_Result |= Device->trace->Create(&g_traceTestId_2, MARK("g_traceTestId_2"));
//    m_Result |= Device->trace->Report();
    m_Result |= Device->pthread->Create(&m_pthread_Id, pthread_new_run, NULL, MARK("m_pthread_Id"));
    m_Result |= Device->pthread->Join(m_pthread_Id, NULL);
    m_Result |= Device->trace->Destroy(&g_traceTestId_1);
    m_Result |= Device->trace->Destroy(&g_traceTestId_2);
    m_Result |= Device->pthread->Destroy(&m_pthread_Id);
    return m_Result;
}
