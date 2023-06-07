////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021. Xiaomi Technology Co.
// All Rights Reserved.
// Confidential and Proprietary - Xiaomi Technology Co.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "MiCallStackUtils.h"

#include <utils/CallStack.h>
#include <utils/Printer.h>
#include <utils/ProcessCallStack.h>

#include "MiDebugUtils.h"

namespace midebug {

class miviLogPrinter final : public android::Printer
{
public:
    miviLogPrinter(const char *prefix = nullptr, int logLevel = ANDROID_LOG_WARN)
        : mPrefix(prefix ?: ""), mLogLevel(logLevel)
    {
    }
    virtual void printLine(const char *string) override
    {
        MLOG_PRI(mLogLevel, Mia2LogGroupCallStack, "%s: %s", mPrefix, string);
    }
    bool isEnabled() { return (gMiCamDebugMask > 0) && (gMiCamLogGroup & Mia2LogGroupCallStack); }

private:
    const char *mPrefix;
    int mLogLevel;
};

void dumpThreadCallStack(pid_t tid, int32_t ignoreDepth, const char *prefix, int logLevel)
{
    miviLogPrinter printer(prefix, logLevel);
    if (!printer.isEnabled()) {
        return;
    }

    android::CallStack stack;
    stack.update(ignoreDepth, tid);
    stack.print(printer);
}

void dumpCurProcessCallStack(const char *prefix, int logLevel)
{
    miviLogPrinter printer(prefix, logLevel);
    if (!printer.isEnabled()) {
        return;
    }

    android::ProcessCallStack stack;
    stack.update();
    stack.print(printer);
}

} // namespace midebug
