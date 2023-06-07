/*===========================================================================
   Copyright (c) 2018,2020 by Tetras Technologies, Incorporated.
   All Rights Reserved.
============================================================================*/

/*===========================================================================

                           EDIT HISTORY FOR FILE

This section contains comments describing changes made to this file.
Notice that changes are listed in reverse chronological order.
============================================================================*/

#ifndef STM_UTILS_H_
#define STM_UTILS_H_ 1

#ifdef _MSC_VER
#ifdef SDK_EXPORTS
#define PORTRAIT_REPAIR_API_ __declspec(dllexport)
#else
#define PORTRAIT_REPAIR_API_ __declspec(dllimport)
#endif
#define DEPRECATED_ATTR_ __declspec(deprecated)
#else /* _MSC_VER */
#ifdef SDK_EXPORTS
#define PORTRAIT_REPAIR_API_ __attribute__((visibility("default")))
#else
#define PORTRAIT_REPAIR_API_
#endif
#define DEPRECATED_ATTR_ __attribute__((deprecated))
#endif

#ifdef __cplusplus
#define PORTRAIT_REPAIR_API extern "C" PORTRAIT_REPAIR_API_
#else
#define PORTRAIT_REPAIR_API PORTRAIT_REPAIR_API_
#endif

#define PORTRAIT_REPAIR_DEPRECATED_API PORTRAIT_REPAIR_API DEPRECATED_ATTR_

#endif // STM_UTILS_H_
