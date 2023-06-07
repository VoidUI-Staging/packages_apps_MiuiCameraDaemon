/**
 * @file		dev_system.cpp
 * @brief
 * @details
 * @author
 * @date        2019.07.03
 * @version		0.1
 * @note
 * @warning
 * @par
 *
 */

#include "dev_system.h"

#include "dev_type.h"
#include "unistd.h"

S64 DevSystem_SleepMs(S64 ms)
{
    return usleep(ms * 1000);
}

S64 DevSystem_SleepUs(S64 us)
{
    return usleep(us);
}

S64 DevSystem_Reboot(S64 ms)
{
    usleep(ms * 1000);
    // TODO
    return RET_ERR;
}

const Dev_System m_dev_system = {
    .SleepMs = DevSystem_SleepMs,
    .SleepUs = DevSystem_SleepUs,
    .Reboot = DevSystem_Reboot,
};
