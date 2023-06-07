/**
 * @file     morpho_error.h
 * @brief    Error code definitions
 * @version  2.0.0
 * @date     2017-02-01
 *
 * Copyright (C) 2006 Morpho, Inc.
 */

#ifndef MORPHO_ERROR_H
#define MORPHO_ERROR_H

/* Error code */
#define MORPHO_OK         ((int)0x00000000) /**< Successful */
#define MORPHO_INPROGRESS ((int)0x00010000) /**< Now in progress */
#define MORPHO_CANCELED   ((int)0x00020000) /**< Canceled */
#define MORPHO_SUSPENDED  ((int)0x00080000) /**< Suspended */

#define MORPHO_ERROR_GENERAL_ERROR ((int)0x80000000) /**< General error */
#define MORPHO_ERROR_PARAM         ((int)0x80000001) /**< Invalid argument */
#define MORPHO_ERROR_STATE         ((int)0x80000002) /**< Invalid internal state or function call order */
#define MORPHO_ERROR_MALLOC        ((int)0x80000004) /**< Memory allocation error */
#define MORPHO_ERROR_IO            ((int)0x80000008) /**< Input/output error */
#define MORPHO_ERROR_UNSUPPORTED   ((int)0x80000010) /**< Unsupported function */
#define MORPHO_ERROR_NOTFOUND      ((int)0x80000020) /**< Is not found to be searched */
#define MORPHO_ERROR_INTERNAL      ((int)0x80000040) /**< Internal error */
#define MORPHO_ERROR_TIMEDOUT      ((int)0x80000080) /**< Timed out */
#define MORPHO_ERROR_UNKNOWN       ((int)0xC0000000) /**< Error other than the above */

#endif /* #ifndef MORPHO_ERROR_H */
