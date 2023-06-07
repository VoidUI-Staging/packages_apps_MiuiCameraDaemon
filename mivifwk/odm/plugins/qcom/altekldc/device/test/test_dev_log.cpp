/**
 * @file       test_dev_log.c
 * @brief
 * @details
 * @author
 * @date       2021.06.21
 * @version    0.1
 * @note
 * @warning
 * @par        History:
 *
 */

#include "../device.h"

#undef LOG_TAG
#define LOG_TAG  "MY_TEST"

#undef LOG_GROUP
#define LOG_GROUP GROUP_TEST

static S32 m_Result = RET_OK;

S32 test_dev_log(void) {
//    Device->log->Level(DEV_LOG_LEVEL_INFO);
//    Device->log->Group(GROUP_CORE|GROUP_PLUGIN|GROUP_META);
//    Device->log->OfflineEnable(TRUE);
    DEV_LOGV("LOG TEST[DEV_LOGV]");
    DEV_LOGI("LOG TEST[DEV_LOGI]");
    DEV_LOGE("LOG TEST[DEV_LOGE]");
    m_Result = RET_OK;
    U8 p[10] = {0xFF, 0x55, 0x33};
    DEV_LOGI("LOG TEST[DEV_LOGI]");
    DEV_LOG_HEX("LOG TEST[DEV_LOG_HEX]", p, (int )sizeof(p));
    DEV_IF_LOGE((p!=NULL), "LOG TEST[DEV_IF_LOGE]");
    DEV_LOGE("LOG TEST[DEV_LOGE]");
    DEV_IF_LOGE_RETURN_RET((p!=NULL), m_Result, "LOG TEST[DEV_IF_LOGE_RETURN_RET]");
    return m_Result;
}
