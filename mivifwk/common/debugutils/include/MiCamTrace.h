////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2021. Xiaomi Technology Co.
// All Rights Reserved.
// Confidential and Proprietary - Xiaomi Technology Co.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  MiCamTrace.h
/// @brief Trace related definitions and prototypes
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _MICAMTRACE_H
#define _MICAMTRACE_H

#undef ATRACE_TAG
#define ATRACE_TAG ATRACE_TAG_ALWAYS

#define MICAM_TRACE_ENABLED 1

#include <cutils/trace.h>
#include <stdarg.h>

#include "MiCamUtils.h"

namespace midebug {

typedef uint32_t MiCamTraceGroup;
static const MiCamTraceGroup MialgoTraceProfile = (1 << 0);
static const MiCamTraceGroup MialgoTraceMemProfile = (1 << 1);
static const MiCamTraceGroup MialgoTraceCapture = (1 << 2);

// @brief Trace enablement structure
struct MiCamTraceInfo
{
    MiCamTraceGroup groupsEnable; // < Tracing groups enable bits
    bool traceErrorLogEnable;     // < Enable tracing for error logs
};

extern MiCamTraceInfo g_traceInfo;

#if MICAM_TRACE_ENABLED

class ScopedTrace
{
public:
    inline ScopedTrace(uint64_t tag, const char *name) : mTag(tag) { atrace_begin(mTag, name); }
    inline ~ScopedTrace() { atrace_end(mTag); }

private:
    uint64_t mTag;
};

// ATRACE_NAME traces from its location until the end of its enclosing scope.
#define _PASTE(x, y) x##y
#define PASTE(x, y)  _PASTE(x, y)

#define MICAM_TRACE_SCOPED_F(group, ...)                                                           \
    if (midebug::g_traceInfo.groupsEnable & group) {                                               \
        char traceNameRandomString[midebug::MaxTraceStringLength];                                 \
        MCamxUtils::SNPrintF(traceNameRandomString, sizeof(traceNameRandomString), ##__VA_ARGS__); \
        ::midebug::ScopedTrace PASTE(___tracer, __LINE__)(ATRACE_TAG, traceNameRandomString);      \
    }

// Event UID
static const uint32_t MaxTraceStringLength = 512; ///< midebug internal trace message length limit

// The local name is just some random name that hopefully wont ever collide with some real local
#define MICAM_TRACE_SYNC_BEGIN_F(group, ...)                                                       \
    if (midebug::g_traceInfo.groupsEnable & group) {                                               \
        char traceNameRandomString[midebug::MaxTraceStringLength];                                 \
        MCamxUtils::SNPrintF(traceNameRandomString, sizeof(traceNameRandomString), ##__VA_ARGS__); \
        ATRACE_BEGIN(traceNameRandomString);                                                       \
    }

#define MICAM_TRACE_ASYNC_BEGIN_F(group, id, ...)                                                  \
    if (midebug::g_traceInfo.groupsEnable & group) {                                               \
        char traceNameRandomString[midebug::MaxTraceStringLength];                                 \
        MCamxUtils::SNPrintF(traceNameRandomString, sizeof(traceNameRandomString), ##__VA_ARGS__); \
        ATRACE_ASYNC_BEGIN(traceNameRandomString, static_cast<uint32_t>(id));                      \
    }

#define MICAM_TRACE_ASYNC_END_F(group, id, ...)                                                    \
    if (midebug::g_traceInfo.groupsEnable & group) {                                               \
        char traceNameRandomString[midebug::MaxTraceStringLength];                                 \
        MCamxUtils::SNPrintF(traceNameRandomString, sizeof(traceNameRandomString), ##__VA_ARGS__); \
        ATRACE_ASYNC_END(traceNameRandomString, static_cast<uint32_t>(id));                        \
    }

#define MICAM_TRACE_INT32_F(group, value, ...)                                                     \
    if (midebug::g_traceInfo.groupsEnable & group) {                                               \
        char traceNameRandomString[midebug::MaxTraceStringLength];                                 \
        MCamxUtils::SNPrintF(traceNameRandomString, sizeof(traceNameRandomString), ##__VA_ARGS__); \
        ATRACE_INT(traceNameRandomString, value);                                                  \
    }

#define MICAM_TRACE_INT64_F(group, value, ...)                                                     \
    if (midebug::g_traceInfo.groupsEnable & group) {                                               \
        char traceNameRandomString[midebug::MaxTraceStringLength];                                 \
        MCamxUtils::SNPrintF(traceNameRandomString, sizeof(traceNameRandomString), ##__VA_ARGS__); \
        ATRACE_INT64(traceNameRandomString, value);                                                \
    }

#define MICAM_TRACE_MESSAGE_F(group, ...)                                                          \
    if (midebug::g_traceInfo.groupsEnable & group) {                                               \
        char traceNameRandomString[midebug::MaxTraceStringLength];                                 \
        MCamxUtils::SNPrintF(traceNameRandomString, sizeof(traceNameRandomString), ##__VA_ARGS__); \
        ATRACE_BEGIN(traceNameRandomString);                                                       \
        ATRACE_END();                                                                              \
    }

#define MICAM_TRACE_SYNC_BEGIN(group, pName)         \
    if (midebug::g_traceInfo.groupsEnable & group) { \
        ATRACE_BEGIN(pName);                         \
    }

#define MICAM_TRACE_SYNC_END(group)                  \
    if (midebug::g_traceInfo.groupsEnable & group) { \
        ATRACE_END();                                \
    }

#define MICAM_TRACE_MESSAGE(group, pString)          \
    if (midebug::g_traceInfo.groupsEnable & group) { \
        ATRACE_BEGIN(pString);                       \
        ATRACE_END();                                \
    }

#define MICAM_IS_TRACE_ENABLED(group) (midebug::g_traceInfo.groupsEnable & group)

#else

#define MICAM_TRACE_SCOPED_F(...)
#define MICAM_TRACE_SYNC_BEGIN_F(...)
#define MICAM_TRACE_ASYNC_BEGIN_F(...)
#define MICAM_TRACE_ASYNC_END_F(...)
#define MICAM_TRACE_INT32_F(...)
#define MICAM_TRACE_INT64_F(...)
#define MICAM_TRACE_MESSAGE_F(...)
#define MICAM_TRACE_SYNC_BEGIN(...)
#define MICAM_TRACE_SYNC_END(...)
#define MICAM_TRACE_MESSAGE(...)
#define MICAM_IS_TRACE_ENABLED(...)

#endif // MICAM_TRACES_ENABLED

} // namespace midebug

#endif /* _MICAMTRACE_H*/
