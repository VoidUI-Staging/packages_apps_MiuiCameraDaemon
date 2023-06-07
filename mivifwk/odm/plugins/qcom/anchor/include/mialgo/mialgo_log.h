/************************************************************************************

Filename  : mialgo_log.h
Content   :
Created   : Nov. 21, 2019
Author    : guwei (guwei@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/

#ifndef MIALGO_LOG_H__
#define MIALGO_LOG_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_basic.h"
#include "mialgo_type.h"

/**
* @brief Data structure of log output.
*
* for windows and ubuntu.
*   print can only be output to stdout.
*
* for android.
*   print can be output to stdout or logcat.
*   and output to logcat by default.
*   MIALGO_LOG_OUTPUT_STDOUT set output to stdout
*/
typedef enum
{
    MIALGO_LOG_OUTPUT_INVALID = 0,               /*!< invalid output */
    MIALGO_LOG_OUTPUT_STDOUT,                    /*!< standard output */
    MIALGO_LOG_OUTPUT_LOGCAT,                    /*!< LogCat of output */
    MIALGO_LOG_OUTPUT_NUM,                       /*!< number of output */
} MialgoLogOutput;

/**
* @brief Mialgo print function, direct use of MialgoPrint is not recommended.
* @param[in] tag                  tag of log
* @param[in] set_level            log level, mainly including  MIALGO_LOG_LV_ERROR, MIALGO_LOG_LV_INFO,  MIALGO_LOG_LV_DEBUG
* @param[in] format               format for printing
*
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
extern MI_S32 MialgoPrint(const MI_CHAR *tag, MialgoLogLv set_level, const MI_CHAR *format, ...);

/**
* @brief Mialgo for printing, direct use of MialgoPrint is not recommended .
*/
#define MIALGO_LOG(tag, level, format, ...)                                         \
    do {                                                                            \
        MialgoPrint(tag, level, format, ##__VA_ARGS__);                            \
    } while (0)

/**
* @brief Error printing.
*/
#define MIALGO_LOGE(tag, format, ...)    MIALGO_LOG(tag, MIALGO_LOG_LV_ERROR,       \
                                                    "[%s %d] " format, \
                                                    __FUNCTION__, __LINE__, ##__VA_ARGS__)

/**
* @brief Information printing.
*/
#define MIALGO_LOGI(tag, format, ...)    MIALGO_LOG(tag, MIALGO_LOG_LV_INFO,        \
                                                    format, ##__VA_ARGS__)

/**
* @brief Debug printing.
*/
#define MIALGO_LOGD(tag, format, ...)    MIALGO_LOG(tag, MIALGO_LOG_LV_DEBUG,       \
                                                    format, ##__VA_ARGS__)

/**
* @brief Mialgo Sets log level function.
* @param[in] level           MialgoLogLv type, level to set, @see MialgoLogLv.
* If level is MIALGO_LOG_LV_INVALID, set it to MIALGO_LOG_LV_ERROR, if level is MIALGO_LOG_LV_NUM, set it to MIALGO_LOG_LV_DEBUG.
* Other types remain the same
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoSetLogLevel(MialgoLogLv level);

/**
* @brief Mialgo Sets log tag function.
* @param[in] tag            const MI_CHAR type, tag to set.
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
* if set tag, only the tag set can be printed
*
*/
MI_S32 MialgoSetLogTag(const MI_CHAR *tag);

/**
* @brief Mialgo Adds log tag function.
* @param[in] tag            const MI_CHAR type, tag to add.
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h

* if set tag, all the tags will be printed
*/
MI_S32 MialgoAddLogTag(const MI_CHAR *tag);

/**
* @brief Mialgo Deletes log tag function.
* @param[in] tag            const MI_CHAR type, tag to delete.
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoDelLogTag(const MI_CHAR *tag);

/**
* @brief Mialgo Sets log output type function.
* @param[in] output            MialgoLogOutput type, output type to set, @see MialgoLogOutput.
* 
* * @code
    MI_S32 ret = MIALGO_OK;
    if ((ret = MialgoSetLogOutput(MIALGO_LOG_OUTPUT_STDOUT)) != MIALGO_OK)
        {
            return ret;
        }
* @endcode
* 
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*/
MI_S32 MialgoSetLogOutput(MialgoLogOutput output);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_LOG_H__

