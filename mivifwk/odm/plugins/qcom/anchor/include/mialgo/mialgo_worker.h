/************************************************************************************

Filename  : mialgo_worker.h
Content   :
Created   : Jan. 13, 2019
Author    : guwei (guwei@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/
#ifndef MIALGO_WORKER_H__
#define MIALGO_WORKER_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#ifndef WIN32
#include "mialgo_type.h"
#include "mialgo_errorno.h"
#include "mialgo_basic.h"
#include "mialgo_cpu.h"

#include <sys/syscall.h>
#include <unistd.h>
#include <stdint.h>

#define MIALGO_MAX_WORKER_NUM               (32)            /*!< max worker thread num */
#define MIALGO_MAX_TASK_NUM                 (1024)          /*!< max task list num */

/**
* @brief task token, used to synchronize tasks.
*/
typedef struct MialgoTaskToken MialgoTaskToken;

/**
* @brief worker pool.
* A typical producer-consumer pattern is implemented.
* User use @see MialgoWorkerPoolAddTask to add a task to the worker pool.
* Inside the thread pool is a set of threads that process tasks.
*/
typedef struct MialgoWorkerPool MialgoWorkerPool;

/**
* @brief task callback function.
*/
typedef MI_S32 (*MialgoTaskCallback)(MI_VOID *);

/**
* @brief worker task.
*
* Description of callback
*   @see MialgoTaskCallback, user-defined callback function.
*
* Description of param
*   User-defined parameters.
*
* Description of token
*   @see MialgoTaskToken, use to synchronize tasks.
*/
typedef struct
{
    MialgoTaskCallback callback;        /*!< @see MialgoTaskCallback, user-defined callback function */
    MI_VOID *param;                     /*!< User-defined parameters */
    MialgoTaskToken *token;             /*!< @see MialgoTaskToken, use to synchronize tasks. */
} MialgoWorkerTask;

/**
* @brief Init task token.
*
* @return
*        <em>MialgoTaskToken *</em> @see MialgoTaskToken, return NULL mean an error.
*/
MialgoTaskToken* MialgoInitTaskToken();

/**
* @brief Init task token.
* @param[in]  token             @see MialgoTaskToken.
*
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoUnitTaskToken(MialgoTaskToken *token);

/**
* @brief Synchronize tasks.
* @param[in]  token             @see MialgoTaskToken.
*
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h.
*
* Synchronize tasks
*   we use @MialgoWorkerPoolAddTask to add a task to worker pool, the worker pool asynchronous process task.
*   we can do other things, like perform a GPU calculation.
*   Finally we want to make sure that the task given to the thread pool is done
*   MialgoTaskTokenSyncWait is uesed to Synchronize tasks.
*   MialgoTaskTokenSyncWait will block until the token num is 0.
*/
MI_S32 MialgoTaskTokenSyncWait(MialgoTaskToken *token);

/**
* @brief token num atoms minus one.
* @param[in]  token             @see MialgoTaskToken.
*
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h.
*
* Synchronize tasks
*   MialgoTaskTokenAtomicDec must be called at user-defined callback function.
*   when call MialgoTaskTokenAtomicDec, token num atoms minus one.
*   when the token num is 0, will wakeup MialgoTaskTokenSyncWait
*/
MI_S32 MialgoTaskTokenAtomicDec(MialgoTaskToken *token);

/**
* @brief Create a new worker pool.
* @param[in]  worker_num            woker thread num, worker_num must <= @see worker_num.
* @param[in]  task_size             task list size, task_size must <= @see MIALGO_MAX_TASK_NUM.
* @param[in]  affinity              woker thread affinity, @see MIALGO_CPU_BIG @see MIALGO_CPU_LITTLE.
* @param[in]  debug                 debug mode, will print run info.
*
* @return
*        <em>MialgoWorkerPool *</em> @see MialgoWorkerPool, return NULL mean an error.
*
* affinity
*   MIALGO_CPU_BIG      : worker thread run at big cpus
*   MIALGO_CPU_LITTLE   : worker thread run at littel cpus
*   0                   : worker thread run all cpus
*/
MialgoWorkerPool* MialgoInitWorkerPool(MI_S32 worker_num, MI_S32 task_size, MI_S32 affinity, MI_S32 debug);

/**
* @brief Unit worker pool.
* @param[in]  worker_pool               @see MialgoWorkerPool.
*
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoUnitWorkerPool(MialgoWorkerPool *worker_pool);

/**
* @brief Set worker pool debug.
* @param[in]  worker_pool               @see MialgoWorkerPool.
* @param[in]  debug                     debug mode, will print run info.
*
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoWorkerPoolSetDebug(MialgoWorkerPool *worker_pool, MI_S32 debug);

/**
* @brief Add a new task to worker pool.
* @param[in]  worker_pool               @see MialgoWorkerPool.
* @param[in]  task                      @see MialgoWorkerTask.
*
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h.
*
* Description of token
*   When add a task, the num of the task's token will be plus one.
*/
MI_S32 MialgoWorkerPoolAddTask(MialgoWorkerPool *worker_pool, MialgoWorkerTask *task);

#endif // WIN32

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_WORKER_H__
