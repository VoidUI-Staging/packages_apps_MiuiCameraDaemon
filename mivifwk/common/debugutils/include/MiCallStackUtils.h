////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021. Xiaomi Technology Co.
// All Rights Reserved.
// Confidential and Proprietary - Xiaomi Technology Co.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _MICALLSTACKUTILS_H
#define _MICALLSTACKUTILS_H

#include <sys/types.h>
#include <unistd.h>

namespace midebug {

void dumpThreadCallStack(pid_t tid, int32_t ignoreDepth, const char *prefix, int logLevel);
void dumpCurProcessCallStack(const char *prefix, int logLevel);
} // namespace midebug

#endif
