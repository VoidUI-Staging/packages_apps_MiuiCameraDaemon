#pragma once

#include "OfflineDebugDataManager.h"

#if defined(__GNUC__)
#define VISIBILITY_PUBLIC __attribute__((visibility("default")))
#else
#define VISIBILITY_PUBLIC __declspec(dllexport)
#endif // defined(__GNUC__)

#ifdef __cplusplus
extern "C" {
#endif

VISIBILITY_PUBLIC void *GetDebugDataBufferAndAddReference(size_t oriSize);

VISIBILITY_PUBLIC uint32_t SubDebugDataReference(void *pDebugData);

VISIBILITY_PUBLIC void SetSessionStatus(bool status);

VISIBILITY_PUBLIC void SetVTStatus(bool status);

#ifdef __cplusplus
}
#endif
