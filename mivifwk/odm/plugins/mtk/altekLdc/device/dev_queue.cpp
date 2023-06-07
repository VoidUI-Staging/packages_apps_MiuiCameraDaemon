/**
 * @file		dev_queue.cpp
 * @brief		预分配循环FIFO
 * @details
 * @author
 * @date        2019.07.03
 * @version		0.1
 * @note
 * @warning		多线程同时写入 DevQueue_GetWritePtr 不安全
 * @par			History:
 *
 */

#include "dev_queue.h"

#include "dev_log.h"
#include "dev_type.h"
#include "stdlib.h"
#include "string.h"

//#define CONFIG_PTHREAD_SUPPORT       (1)
//#undef CONFIG_PTHREAD_SUPPORT

#ifdef CONFIG_PTHREAD_SUPPORT
#include <pthread.h>
static pthread_mutex_t m_mutex;
#endif

#define MSG_QUEUE_MAX_NUM (256) // 最大消息队列数量

#pragma pack(1)
/**
 * @brief		消息队列
 * @details
 * @note
 * @par
 */
typedef struct __Queue_list_node
{
    U8 *memory;
    S64 size;
    S64 unreadSize;
    S64 writeIndex;
    S64 activeWriteSize;
    S64 readIndex;
    S64 activeReadSize;
} Queue_list;
#pragma pack()

static volatile Queue_list *m_queue_list[MSG_QUEUE_MAX_NUM];
static S64 init_f = FALSE;

#ifndef MIN
#define MIN(a, b)               \
    ({                          \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        _a < _b ? _a : _b;      \
    })
#endif
#ifndef MAX
#define MAX(a, b)               \
    ({                          \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        _a > _b ? _a : _b;      \
    })
#endif

/**
 * @brief		消息队列模块初始化
 * @details
 * @param[in]
 * @param[in]
 * @param[in]
 * @return
 * @return
 * @see
 * @note
 * @par
 */
S64 DevQueue_Init(void)
{
    if (init_f == TRUE) {
        return RET_OK;
    }
#ifdef CONFIG_PTHREAD_SUPPORT
    S32 ret = pthread_mutex_init(&m_mutex, NULL);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "Mutex Create  err");
#endif
    memset((void *)m_queue_list, 0, sizeof(m_queue_list));
    init_f = TRUE;
    return RET_OK;
}

/**
 * @brief		创建消息队列
 * @details
 * @param[out]   id  消息ID
 * @return
 * @return
 * @see
 * @note
 * @par
 */
S64 DevQueue_Create(S64 size)
{
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_lock(&m_mutex);
#endif
    S64 m_id = 0;
    S64 id = 0;
    volatile Queue_list *p_Queue = NULL;
    DEV_IF_LOGE_GOTO((init_f != TRUE), exit, "Queue Init err");
    for (id = 1; id < MSG_QUEUE_MAX_NUM; id++) {
        if (m_queue_list[id] == NULL) {
            break;
        }
    }
    DEV_IF_LOGE_GOTO((id >= MSG_QUEUE_MAX_NUM), exit, "Queue Insufficient quantity");
    p_Queue = (Queue_list *)malloc(sizeof(Queue_list));
    DEV_IF_LOGE_GOTO((p_Queue == NULL), exit, "Queue malloc err");
    p_Queue->size = size;
    p_Queue->unreadSize = 0;
    p_Queue->writeIndex = 0;
    p_Queue->activeWriteSize = 0;
    p_Queue->readIndex = 0;
    p_Queue->activeReadSize = 0;
    p_Queue->memory = (U8 *)malloc(p_Queue->size);
    if (p_Queue->memory == NULL) {
        DEV_LOGE("Queue Malloc err");
        if (p_Queue != NULL) {
            free((void *)p_Queue);
        }
        goto exit;
    }
    m_queue_list[id] = p_Queue;
    m_id = id;
exit:
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_unlock(&m_mutex);
#endif
    DEV_IF_LOGE((m_id == 0), "Create ID err");
    return m_id;
}

/**
 * @brief		判断消息队列是否为空
 * @details
 * @param[in]   id  消息ID
 * @param[in]
 * @return  	TRUE 空
 * @return  	FALSE 非空
 * @see
 * @note
 * @par
 */
S64 DevQueue_Empty(S64 id)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), TRUE, "Queue Init err");
    volatile Queue_list *p_Queue = m_queue_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), TRUE, "Queue list err");
    if (p_Queue->unreadSize == 0) {
        return TRUE;
    }
    return FALSE;
}

/**
 * @brief		判断消息队列是否为满
 * @details
 * @param[in]   id  消息ID
 * @param[in]
 * @return  	TRUE 满
 * @return  	FALSE 非满
 * @see
 * @note
 * @par
 */
S64 DevQueue_Full(S64 id)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), TRUE, "Queue Init err");
    volatile Queue_list *p_Queue = m_queue_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), TRUE, "Queue list err");
    if (p_Queue->unreadSize == p_Queue->size) {
        return TRUE;
    }
    return FALSE;
}

S64 DevQueue_GetWritePtr(S64 id, void **ptr, S64 length)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Queue Init err");
    volatile Queue_list *p_Queue = m_queue_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), 0, "Queue list err");
    S64 returnedArea = 0;
    if (ptr == NULL) {
        return 0;
    }
    if (p_Queue->activeWriteSize != 0) {
        return 0;
    }
    // check how much total size we have
    returnedArea = MIN(p_Queue->size - p_Queue->unreadSize, (S64)length);
    // check how much data we have untill the end of FIFO
    returnedArea = MIN(p_Queue->size - p_Queue->writeIndex, returnedArea);
    // get out fast if no data avilable
    if (p_Queue->memory == NULL) {
        return 0;
    }
    if (returnedArea == 0) {
        *ptr = NULL;
    } else {
        p_Queue->activeWriteSize = returnedArea;
        *ptr = &p_Queue->memory[p_Queue->writeIndex];
    }
    return returnedArea;
}

S64 DevQueue_MarkWriteDone(S64 id)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Queue Init err");
    volatile Queue_list *p_Queue = m_queue_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), RET_ERR, "Queue list err");
    if (p_Queue->activeWriteSize == 0) {
        return RET_CONTINUE;
    }
    p_Queue->writeIndex += p_Queue->activeWriteSize;
    if (p_Queue->writeIndex == p_Queue->size) {
        p_Queue->writeIndex = 0;
    }
    p_Queue->unreadSize += p_Queue->activeWriteSize;
    p_Queue->activeWriteSize = 0;
    return RET_OK;
}

S64 DevQueue_GetReadPtr(S64 id, void **ptr, S64 length)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Queue Init err");
    volatile Queue_list *p_Queue = m_queue_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), 0, "Queue list err");
    S64 returnedArea = 0;
    if (ptr == NULL) {
        return 0;
    }
    if (p_Queue->activeReadSize != 0) {
        return 0;
    }
    // we are limited either by the read size, or by the data available
    returnedArea = MIN(p_Queue->unreadSize, (S64)length);
    // and limited to provide a contiguous array
    returnedArea = MIN(p_Queue->size - p_Queue->readIndex, returnedArea);
    if (p_Queue->memory == NULL) {
        return 0;
    }
    if (returnedArea == 0) {
        *ptr = NULL;
    } else {
        p_Queue->activeReadSize = returnedArea;
        *ptr = &p_Queue->memory[p_Queue->readIndex];
    }
    return returnedArea;
}

S64 DevQueue_MarkReadDone(S64 id)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Queue Init err");
    volatile Queue_list *p_Queue = m_queue_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), RET_ERR, "Queue list err");
    if (p_Queue->activeReadSize == 0) {
        return RET_CONTINUE;
    }
    p_Queue->readIndex += p_Queue->activeReadSize;
    if (p_Queue->readIndex == p_Queue->size) {
        p_Queue->readIndex = 0;
    }
    p_Queue->unreadSize -= p_Queue->activeReadSize;
    p_Queue->activeReadSize = 0;
    return RET_OK;
}

#define DevQueue_PushNBit(p_Queue, data)                                      \
    {                                                                         \
        if (p_Queue->size - p_Queue->unreadSize < (S64)sizeof(data)) {        \
            return RET_ERR;                                                   \
        }                                                                     \
        *((__typeof__(&data))(&p_Queue->memory[p_Queue->writeIndex])) = data; \
        p_Queue->unreadSize += (S64)sizeof(data);                             \
        p_Queue->writeIndex += (S64)sizeof(data);                             \
        if (p_Queue->writeIndex == p_Queue->size) {                           \
            p_Queue->writeIndex = 0;                                          \
        }                                                                     \
        return RET_OK;                                                        \
    }

#define DevQueue_PopNBit(p_Queue, data)                                      \
    {                                                                        \
        if (p_Queue->unreadSize < (S64)sizeof(data)) {                       \
            return RET_ERR;                                                  \
        }                                                                    \
        data = *((__typeof__(&data))(&p_Queue->memory[p_Queue->readIndex])); \
        p_Queue->unreadSize -= (S64)sizeof(data);                            \
        p_Queue->readIndex += (S64)sizeof(data);                             \
        if (p_Queue->readIndex == p_Queue->size) {                           \
            p_Queue->readIndex = 0;                                          \
        }                                                                    \
        return RET_OK;                                                       \
    }

/**
 * @brief		向消息队列中发送消息
 * @details
 * @param[in]   id  消息ID
 * @param[in]	data	消息内容
 * @param[in]	data_length	消息内容长度
 * @return
 * @return
 * @see
 * @note
 * @par
 */
S64 DevQueue_Write8Bit(S64 id, U8 data)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Queue Init err");
    volatile Queue_list *p_Queue = m_queue_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), RET_ERR, "Queue list err");
    DevQueue_PushNBit(p_Queue, data);
    return RET_OK;
}

S64 DevQueue_Write16Bit(S64 id, U16 data)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Queue Init err");
    volatile Queue_list *p_Queue = m_queue_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), RET_ERR, "Queue list err");
    DevQueue_PushNBit(p_Queue, data);
    return RET_OK;
}

S64 DevQueue_Write32Bit(S64 id, U32 data)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Queue Init err");
    volatile Queue_list *p_Queue = m_queue_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), RET_ERR, "Queue list err");
    DevQueue_PushNBit(p_Queue, data);
    return RET_OK;
}

S64 DevQueue_Write64Bit(S64 id, U64 data)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Queue Init err");
    volatile Queue_list *p_Queue = m_queue_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), RET_ERR, "Queue list err");
    DevQueue_PushNBit(p_Queue, data);
    return RET_OK;
}

S64 DevQueue_Write(S64 id, U8 *data, S64 length)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Queue Init err");
    DEV_IF_LOGE_RETURN_RET((data == NULL), RET_ERR_ARG, "Queue arg err");
    volatile Queue_list *p_Queue = m_queue_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), 0, "Queue list err");
    DEV_IF_LOGE_RETURN_RET((p_Queue->memory == NULL), 0, "Queue data err");
    if (p_Queue->size - p_Queue->unreadSize < (S64)length) {
        return 0;
    }
    memcpy((void *)(&p_Queue->memory[p_Queue->writeIndex]), (void *)data, length); // XXX 验证
    p_Queue->unreadSize += (S64)length;
    p_Queue->writeIndex += (S64)length;
    if (p_Queue->writeIndex == p_Queue->size) {
        p_Queue->writeIndex = 0;
    }
    return length;
}

/**
 * @brief		从消息队列中获取消息
 * @details
 * @param[in]   id  消息ID
 * @param[out]	data	消息内容
 * @param[out]	data_length	消息内容长度
 * @return
 * @return
 * @see
 * @note
 * @par
 */
S64 DevQueue_Read8Bit(S64 id, U8 *data)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Queue Init err");
    DEV_IF_LOGE_RETURN_RET((data == NULL), RET_ERR_ARG, "Queue arg err");
    volatile Queue_list *p_Queue = m_queue_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), RET_ERR, "Queue list err");
    DevQueue_PopNBit(p_Queue, *data);
    return RET_OK;
}

S64 DevQueue_Read16Bit(S64 id, U16 *data)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Queue Init err");
    DEV_IF_LOGE_RETURN_RET((data == NULL), RET_ERR_ARG, "Queue arg err");
    volatile Queue_list *p_Queue = m_queue_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), RET_ERR, "Queue list err");
    DevQueue_PopNBit(p_Queue, *data);
    return RET_OK;
}

S64 DevQueue_Read32Bit(S64 id, U32 *data)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Queue Init err");
    DEV_IF_LOGE_RETURN_RET((data == NULL), RET_ERR_ARG, "Queue arg err");
    volatile Queue_list *p_Queue = m_queue_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), RET_ERR, "Queue list err");
    DevQueue_PopNBit(p_Queue, *data);
    return RET_OK;
}

S64 DevQueue_Read64Bit(S64 id, U64 *data)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Queue Init err");
    DEV_IF_LOGE_RETURN_RET((data == NULL), RET_ERR_ARG, "Queue arg err");
    volatile Queue_list *p_Queue = m_queue_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), RET_ERR, "Queue list err");
    DevQueue_PopNBit(p_Queue, *data);
    return RET_OK;
}

S64 DevQueue_Read(S64 id, U8 *data, S64 length)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Queue Init err");
    DEV_IF_LOGE_RETURN_RET((data == NULL), 0, "Queue arg err");
    volatile Queue_list *p_Queue = m_queue_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), 0, "Queue list err");
    DEV_IF_LOGE_RETURN_RET((p_Queue->memory == NULL), 0, "Queue data err");
    if (p_Queue->unreadSize < (S64)length) {
        return 0;
    }
    memcpy((void *)data, (void *)(&p_Queue->memory[p_Queue->readIndex]), length); // XXX 验证
    p_Queue->unreadSize -= (S64)length;
    p_Queue->readIndex += (S64)length;
    if (p_Queue->readIndex == p_Queue->size) {
        p_Queue->readIndex = 0;
    }
    return length;
}

/**
 * @brief		获取消息队列中消息的数量
 * @details
 * @param[in]   id  消息ID
 * @param[in]
 * @return  	无
 * @return  	无
 * @see
 * @note
 * @par
 */
S64 DevQueue_Count(S64 id)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Queue Init err");
    volatile Queue_list *p_Queue = m_queue_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), 0, "Queue list err");
    return p_Queue->unreadSize;
}

/**
 * @brief		获取消息队列中消息可用的数量
 * @details
 * @param[in]   id  消息ID
 * @param[in]
 * @return  	无
 * @return  	无
 * @see
 * @note
 * @par
 */
S64 DevQueue_Available(S64 id)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Queue Init err");
    volatile Queue_list *p_Queue = m_queue_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), 0, "Queue list err");
    if (p_Queue->size - p_Queue->unreadSize > 0) {
        return p_Queue->size - p_Queue->unreadSize;
    } else {
        return 0;
    }
}

/**
 * @brief		the number of maximum contiguous unused elements in the fifo
 * @details
 * @param[in]   id  消息ID
 * @param[in]
 * @return  	无
 * @return  	无
 * @see
 * @note
 * @par
 */
S64 DevQueue_ContigAvailable(S64 id)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Queue Init err");
    volatile Queue_list *p_Queue = m_queue_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), 0, "Queue list err");
    if (p_Queue->writeIndex > p_Queue->readIndex) {
        return MAX(p_Queue->size - p_Queue->writeIndex, p_Queue->readIndex);
    } else if (p_Queue->writeIndex < p_Queue->readIndex) {
        return p_Queue->readIndex - p_Queue->writeIndex;
    } else {
        return p_Queue->size - p_Queue->unreadSize;
    }
}

/**
 * @brief		清楚消息队列中消息
 * @details
 * @param[in]   id  消息ID
 * @param[in]
 * @return  	无
 * @return  	无
 * @see
 * @note
 * @par
 */
S64 DevQueue_Clean(S64 id)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Queue Init err");
    volatile Queue_list *p_Queue = m_queue_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), RET_ERR, "Queue list err");
    p_Queue->readIndex = 0;
    p_Queue->writeIndex = 0;
    p_Queue->unreadSize = 0;
    p_Queue->activeReadSize = 0;
    p_Queue->activeWriteSize = 0;
    memset(p_Queue->memory, 0, p_Queue->size);
    return RET_OK;
}

/**
 * @brief		向消息队列去初始化
 * @details
 * @param[in]   id  消息ID
 * @param[in]
 * @param[in]
 * @return
 * @return
 * @see
 * @note
 * @par
 */
S64 DevQueue_Destroy(S64 *id)
{
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_lock(&m_mutex);
#endif
    volatile Queue_list *p_Queue = NULL;
    if (init_f != TRUE) {
        goto exit;
    }
    DEV_IF_LOGE_GOTO((id == NULL), exit, "Queue id err");
    p_Queue = m_queue_list[*id];
    if (p_Queue == NULL) {
        *id = 0;
        goto exit;
    }
    DevQueue_Clean(*id);
    p_Queue->size = 0;
    if (p_Queue->memory != NULL) {
        free(p_Queue->memory);
        p_Queue->memory = NULL;
    }
    if (p_Queue != NULL) {
        free((void *)p_Queue);
        p_Queue = NULL;
    }
    m_queue_list[*id] = NULL;
    *id = 0;
exit:
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_unlock(&m_mutex);
#endif
    return RET_OK;
}

/**
 * @brief       消息队列模块去初始化
 * @details
 * @param[in]
 * @param[in]
 * @param[in]
 * @return
 * @return
 * @see
 * @note
 * @par
 */
S64 DevQueue_Uninit(void)
{
    if (init_f != TRUE) {
        return RET_OK;
    }
    for (S64 id = 1; id < MSG_QUEUE_MAX_NUM; id++) {
        S64 idp = id;
        DevQueue_Destroy(&idp);
    }
    memset(m_queue_list, 0, sizeof(m_queue_list));
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_destroy(&m_mutex);
#endif
    init_f = FALSE;
    return RET_OK;
}

const Dev_Queue m_dev_queue = {
    .Init = DevQueue_Init,
    .Uninit = DevQueue_Uninit,
    .Create = DevQueue_Create,
    .Destroy = DevQueue_Destroy,
    .Write8Bit = DevQueue_Write8Bit,
    .Write16Bit = DevQueue_Write16Bit,
    .Write32Bit = DevQueue_Write32Bit,
    .Write64Bit = DevQueue_Write64Bit,
    .Write = DevQueue_Write,
    .Read8Bit = DevQueue_Read8Bit,
    .Read16Bit = DevQueue_Read16Bit,
    .Read32Bit = DevQueue_Read32Bit,
    .Read64Bit = DevQueue_Read64Bit,
    .Read = DevQueue_Read,
    .Empty = DevQueue_Empty,
    .Count = DevQueue_Count,
    .Available = DevQueue_Available,
    .ContigAvailable = DevQueue_ContigAvailable,
    .Full = DevQueue_Full,
    .Clean = DevQueue_Clean,
    .GetWritePtr = DevQueue_GetWritePtr,
    .MarkWriteDone = DevQueue_MarkWriteDone,
    .GetReadPtr = DevQueue_GetReadPtr,
    .MarkReadDone = DevQueue_MarkReadDone,
};
