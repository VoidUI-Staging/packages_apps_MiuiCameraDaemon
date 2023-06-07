/*----------------------------------------------------------------------------------------------
*
* This file is ArcSoft's property. It contains ArcSoft's trade secret, proprietary and
* confidential information.
*
* The information and code contained in this file is only for authorized ArcSoft employees
* to design, create, modify, or review.
*
* DO NOT DISTRIBUTE, DO NOT DUPLICATE OR TRANSMIT IN ANY FORM WITHOUT PROPER AUTHORIZATION.
*
* If you are not an intended recipient of this file, you must not copy, distribute, modify,
* or take any action in reliance on it.
*
* If you have received this file in error, please immediately notify ArcSoft and
* permanently delete the original and any copy of any file and any printout thereof.
*
*-------------------------------------------------------------------------------------------------*/

#ifndef __ARCSOFT_DEBUG_LOGS_H__
#define __ARCSOFT_DEBUG_LOGS_H__

#ifndef ARCSOFT_LOG_TAG
#define ARCSOFT_LOG_TAG "ARCSOFT_DEBUG"
#endif

#define FILE_FUNC_LINE __FILE__, __func__, __LINE__

typedef enum arc_dbg_e
{
    ARC_ITG_ERR_FATAL,
    ARC_ITG_ERR_ERROR,
    ARC_ITG_ERR_WARN,
    ARC_ITG_ERR_INFO,
    ARC_ITG_ERR_DEBUG,
    ARC_ITG_ERR_VERBOSE,
    ARC_ITG_ERR_MAX
} arc_dbg_lvl_t;

void itg_err_msg(arc_dbg_lvl_t lvl, const char *filepath, const char *funcname, long lineno, const char *fmt, ...);

void config_arcsoft_log(int level);

#define ARC_LOGF(...)                                                \
    do                                                               \
    {                                                                \
        itg_err_msg(ARC_ITG_ERR_FATAL, FILE_FUNC_LINE, __VA_ARGS__); \
        exit(EXIT_FAILURE);                                          \
    } while (0)

#if 1
#define ARC_LOGE(...) itg_err_msg(ARC_ITG_ERR_ERROR, FILE_FUNC_LINE, __VA_ARGS__)
#define ARC_LOGW(...) itg_err_msg(ARC_ITG_ERR_WARN, FILE_FUNC_LINE, __VA_ARGS__)
#define ARC_LOGI(...) itg_err_msg(ARC_ITG_ERR_INFO, FILE_FUNC_LINE, __VA_ARGS__)
#define ARC_LOGD(...) itg_err_msg(ARC_ITG_ERR_DEBUG, FILE_FUNC_LINE, __VA_ARGS__)
#define ARC_LOGV(...) itg_err_msg(ARC_ITG_ERR_VERBOSE, FILE_FUNC_LINE, __VA_ARGS__)
#else
#define ARC_LOGE(...)
#define ARC_LOGW(...)
#define ARC_LOGI(...)
#define ARC_LOGD(...)
#define ARC_LOGV(...)
#endif

#define UNUSED_PARAM(param) (void)param

///> check the condition
#define IFNULL_GOTO(ptr,lable)           if((ptr) == NULL) { ARC_LOGE("[ARC_DBG] NULL POINTER!", __func__, __LINE__); goto lable; }
#define IFNULL_THEN_GOTO(ptr,then,lable) if((ptr) == NULL) { (then); ARC_LOGE("[ARC_DBG] NULL POINTER!", __func__, __LINE__); goto lable; }
#define IFNULL_RETURN(ptr)               if((ptr) == NULL) { ARC_LOGE("[ARC_DBG] NULL POINTER!", __func__, __LINE__); return; }
#define IFNULL_RETURN_ERR(ptr,errCode)   if((ptr) == NULL) { ARC_LOGE("[ARC_DBG] errCode = %d, NULL POINTER!", __func__, __LINE__, (int)(errCode));  return (errCode); }

#define IFERR_GOTO(res,lable)                   if((res)) { ARC_LOGE("[ARC_DBG] res: %d!", __func__, __LINE__,(int) res); goto lable; }
#define IFERR_RETURN(res)                       if((res)) { ARC_LOGE("[ARC_DBG] res: %d", __func__, __LINE__,(int) res);return (res); }
#define IFERR_RETURN_WITH_ERRCODE(res,errcode)  if((res)) { ARC_LOGE("[ARC_DBG] res: %d", __func__, __LINE__,(int) res);return (errcode); }

#define SAFE_DEL(ptr)   if(ptr) { delete ptr; ptr = NULL; }
#define SAFE_FREE(ptr)  if(ptr) { free(ptr); ptr = NULL; }

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
static long long now_ms(void)
{
    struct timeval res;
    gettimeofday(&res, NULL);
    return (long long)res.tv_sec * 1000 + (long long)res.tv_usec / 1e3;
}

#endif //__ARCSOFT_DEBUG_LOGS_H__
