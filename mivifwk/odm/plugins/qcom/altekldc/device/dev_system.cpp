/**
 * @file        dev_system.c
 * @brief
 * @details
 * @author
 * @date        2016.05.17
 * @version     0.1
 * @note
 * @warning
 * @par         History:
 *
 */

#include "dev_type.h"
#include "dev_log.h"
#include "unistd.h"
#include "signal.h"
#include "dev_system.h"

#if defined(WINDOWS_OS)
#include <windows.h>
#include <signal.h>
void usleep(int waitTime) {
    HANDLE timer;
    LARGE_INTEGER ft;
    ft.QuadPart = -(10 * (__int64)waitTime);
    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
}
#endif

S64 DevSystem_SleepMs(S64 ms) {
    usleep(ms * 1000);
    return RET_OK;
}

S64 DevSystem_SleepUs(S64 us) {
    usleep(us);
    return RET_OK;
}

S64 DevSystem_ReBoot(void) {
    DEV_LOGI("SYSTEM REBOOT");
    DevSystem_SleepUs(1000);
    return raise(SIGABRT);//XXX
    return RET_ERR;
}

S64 DevSystem_Abort() {
#if defined(WINDOWS_OS)
    abort();
#else
    return raise(SIGABRT);
#endif
}

S32 DevSystem_Handler(void) {
    return RET_OK;
}

S32 DevSystem_Init(void) {
    return RET_OK;
}

S32 DevSystem_Deinit(void) {
    return RET_OK;
}

const Dev_System m_dev_system={
        .Init       =DevSystem_Init,
        .Deinit     =DevSystem_Deinit,
        .Handler    =DevSystem_Handler,
        .SleepMs    =DevSystem_SleepMs,
        .SleepUs    =DevSystem_SleepUs,
        .ReBoot     =DevSystem_ReBoot,
        .Abort      =DevSystem_Abort,
};
