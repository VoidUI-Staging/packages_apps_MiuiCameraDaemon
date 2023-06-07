/**
 * @file        dev_queue.cpp
 * @brief       预分配循环FIFO
 * @details
 * @author
 * @date        2021.07.03
 * @version     0.1
 * @note
 * @warning
 * @par         History:
 *
 */

#include "device.h"
#include "dev_queue.h"
#include <iostream>
#include <atomic>

#define MSG_QUEUE_MAX_NUM   (256)    // 最大消息队列数量

/**
 * @brief        消息队列
 * @details
 * @note
 * @par
 */
typedef struct __Queue_node {
    S64 *pId;
    char __fileName[FILE_NAME_LEN_MAX];
    char __tagName[FILE_NAME_LEN_MAX];
    U32 __fileLine;
    S64 createTimestamp;
    U8 *data;
    S64 size;
    std::atomic<int> atomic_unreadSize;
    S64 writeIndex;
    S64 activeWriteSize;
    S64 readIndex;
    S64 activeReadSize;
} Queue_node;

static volatile Queue_node *m_queue_p_list[MSG_QUEUE_MAX_NUM];
static S64 init_f = FALSE;
static S64 m_mutex_Id;

#ifndef MIN
#define MIN(a,b)                                \
    ({ __typeof__ (a) _a = (a);                 \
        __typeof__ (b) _b = (b);                \
        _a < _b ? _a : _b; })
#endif
#ifndef MAX
#define MAX(a,b)                                \
    ({ __typeof__ (a) _a = (a);                 \
        __typeof__ (b) _b = (b);                \
        _a > _b ? _a : _b; })
#endif

static const char* DevQueue_NoDirFileName(const char *pFilePath) {
    if (pFilePath == NULL) {
        return NULL;
    }
    const char *pFileName = strrchr(pFilePath, '/');
    if (NULL != pFileName) {
        pFileName += 1;
    } else {
        pFileName = pFilePath;
    }
    return pFileName;
}

/**
 * @brief        创建消息队列
 * @details
 * @param[out]   id  消息ID
 * @return
 * @return
 * @see
 * @note
 * @par
 */
S64 DevQueue_Create(S64 *pId, S64 size, MARK_TAG tagName) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Queue Init err");
    Device->mutex->Lock(m_mutex_Id);
    S64 ret = RET_ERR;
    S64 m_id = 0;
    S64 id = 0;
    volatile Queue_node *p_Queue = NULL;
    for (id = 1; id < MSG_QUEUE_MAX_NUM; id++) {
        if (m_queue_p_list[id] != NULL) {
            if (m_queue_p_list[id]->pId != NULL) {
                if (m_queue_p_list[id]->pId == pId) {
                    if (m_queue_p_list[id]->data != NULL) {
                        m_id = id;
                        DEV_IF_LOGW_GOTO(m_id != 0, exit, "double Create %ld", m_id);
                    }
                    DEV_LOGE("not Destroy err=%ld", id);
                }
            }
        }
    }
    id = 0;
    for (id = 1; id < MSG_QUEUE_MAX_NUM; id++) {
        if (m_queue_p_list[id] == NULL) {
            break;
        }
    }
    DEV_IF_LOGE_GOTO((id >= MSG_QUEUE_MAX_NUM), exit, "Queue Insufficient quantity");
    p_Queue = (Queue_node*) Dev_malloc(sizeof(Queue_node));
    DEV_IF_LOGE_GOTO((p_Queue == NULL), exit, "Queue malloc err");
    p_Queue->size = size;
    std::atomic_init(&p_Queue->atomic_unreadSize,0);
    p_Queue->writeIndex = 0;
    p_Queue->activeWriteSize = 0;
    p_Queue->readIndex = 0;
    p_Queue->activeReadSize = 0;
    p_Queue->data = (U8*) Dev_malloc(p_Queue->size);
    if (p_Queue->data == NULL) {
        DEV_LOGE("Queue Malloc err");
        if (p_Queue != NULL) {
            Dev_free((void*)p_Queue);
        }
        goto exit;
    }
    m_queue_p_list[id] = p_Queue;
    m_id = id;
    *pId = m_id;
    m_queue_p_list[id]->pId = pId;
    Dev_snprintf((char*)m_queue_p_list[id]->__fileName, FILE_NAME_LEN_MAX, "%s", DevQueue_NoDirFileName(__fileName));
    Dev_snprintf((char*)m_queue_p_list[id]->__tagName, FILE_NAME_LEN_MAX, "%s", tagName);
    m_queue_p_list[id]->__fileLine = __fileLine;
    m_queue_p_list[id]->createTimestamp = Device->time->GetTimestampMs();
    ret = RET_OK;
    exit:
    Device->mutex->Unlock(m_mutex_Id);
    DEV_IF_LOGE_RETURN_RET((m_id == 0), RET_ERR, "Create ID err");
    return ret;
}

/**
 * @brief       判断消息队列是否为空
 * @details
 * @param[in]   id  消息ID
 * @param[in]
 * @return      TRUE 空
 * @return      FALSE 非空
 * @see
 * @note
 * @par
 */
S64 DevQueue_Empty(S64 id) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), TRUE, "Queue Init err");
    volatile Queue_node *p_Queue = m_queue_p_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), TRUE, "Queue list err");
    if (p_Queue->atomic_unreadSize.load() == 0) {
        return TRUE;
    }
    return FALSE;
}

/**
 * @brief       判断消息队列是否为满
 * @details
 * @param[in]   id  消息ID
 * @param[in]
 * @return      TRUE 满
 * @return      FALSE 非满
 * @see
 * @note
 * @par
 */
S64 DevQueue_Full(S64 id) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), TRUE, "Queue Init err");
    volatile Queue_node *p_Queue = m_queue_p_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), TRUE, "Queue list err");
    if (p_Queue->atomic_unreadSize.load() == p_Queue->size) {
        return TRUE;
    }
    return FALSE;
}

S64 DevQueue_GetWritePtr(S64 id, void **ptr, S64 size) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Queue Init err");
    volatile Queue_node *p_Queue = m_queue_p_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), 0, "Queue list err");
    S64 returnedArea = 0;
    if (ptr == NULL) {
        return 0;
    }
    if (p_Queue->activeWriteSize != 0) {
        return 0;
    }
    // check how much total size we have
    returnedArea = MIN(p_Queue->size - p_Queue->atomic_unreadSize.load(), (S64 ) size);
    // check how much data we have untill the end of FIFO
    returnedArea = MIN(p_Queue->size - p_Queue->writeIndex, returnedArea);
    // get out fast if no data avilable
    if (p_Queue->data == NULL) {
        return 0;
    }
    if (returnedArea == 0) {
        *ptr = NULL;
    } else {
        p_Queue->activeWriteSize = returnedArea;
        *ptr = &p_Queue->data[p_Queue->writeIndex];
    }
    return returnedArea;
}

S64 DevQueue_MarkWriteDone(S64 id) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Queue Init err");
    volatile Queue_node *p_Queue = m_queue_p_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), RET_ERR, "Queue list err");
    if (p_Queue->activeWriteSize == 0) {
        return RET_CONTINUE;
    }
    p_Queue->writeIndex += p_Queue->activeWriteSize;
    if (p_Queue->writeIndex == p_Queue->size) {
        p_Queue->writeIndex = 0;
    }
    p_Queue->atomic_unreadSize.fetch_add(p_Queue->activeWriteSize);
    p_Queue->activeWriteSize = 0;
    return RET_OK;
}

S64 DevQueue_GetReadPtr(S64 id, void **ptr, S64 size) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Queue Init err");
    volatile Queue_node *p_Queue = m_queue_p_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), 0, "Queue list err");
    S64 returnedArea = 0;
    if (ptr == NULL) {
        return 0;
    }
    if (p_Queue->activeReadSize != 0) {
        return 0;
    }
    // we are limited either by the read size, or by the data available
    returnedArea = MIN(p_Queue->atomic_unreadSize.load(), (S64 ) size);
    // and limited to provide a contiguous array
    returnedArea = MIN(p_Queue->size - p_Queue->readIndex, returnedArea);
    if (p_Queue->data == NULL) {
        return 0;
    }
    if (returnedArea == 0) {
        *ptr = NULL;
    } else {
        p_Queue->activeReadSize = returnedArea;
        *ptr = &p_Queue->data[p_Queue->readIndex];
    }
    return returnedArea;
}

S64 DevQueue_MarkReadDone(S64 id) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Queue Init err");
    volatile Queue_node *p_Queue = m_queue_p_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), RET_ERR, "Queue list err");
    if (p_Queue->activeReadSize == 0) {
        return RET_CONTINUE;
    }
    p_Queue->readIndex += p_Queue->activeReadSize;
    if (p_Queue->readIndex == p_Queue->size) {
        p_Queue->readIndex = 0;
    }
    p_Queue->atomic_unreadSize.fetch_sub(p_Queue->activeReadSize);
    p_Queue->activeReadSize = 0;
    return RET_OK;
}

#define DevQueue_PushNBit(p_Queue, __data) {                            \
    if(p_Queue->size - p_Queue->atomic_unreadSize.load() < (S64)sizeof(__data)){  \
        return RET_ERR;                                                 \
    }                                                                   \
    *((__typeof__(&__data))(&p_Queue->data[p_Queue->writeIndex])) = __data;       \
    p_Queue->atomic_unreadSize.fetch_add((S64)sizeof(__data));          \
    p_Queue->writeIndex += (S64)sizeof(__data);                         \
    if(p_Queue->writeIndex == p_Queue->size){                           \
        p_Queue->writeIndex = 0;                                        \
    }                                                                   \
    return RET_OK;                                                      \
}

#define DevQueue_PopNBit(p_Queue, __data) {                             \
    if(p_Queue->atomic_unreadSize.load() < (S64)sizeof(__data)){        \
        return RET_ERR;                                                 \
    }                                                                   \
    __data = *((__typeof__(&__data))(&p_Queue->data[p_Queue->readIndex]));       \
    p_Queue->atomic_unreadSize.fetch_sub((S64)sizeof(__data));          \
    p_Queue->readIndex  += (S64)sizeof(__data);                         \
    if(p_Queue->readIndex == p_Queue->size){                            \
        p_Queue->readIndex = 0;                                         \
    }                                                                   \
    return RET_OK;                                                      \
}

/**
 * @brief       向消息队列中发送消息
 * @details
 * @param[in]   id  消息ID
 * @param[in]   data    消息内容
 * @param[in]   size    消息内容长度
 * @return
 * @return
 * @see
 * @note
 * @par
 */
S64 DevQueue_Write8Bit(S64 id, U8 data) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Queue Init err");
    volatile Queue_node *p_Queue = m_queue_p_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), RET_ERR, "Queue list err");
    DevQueue_PushNBit(p_Queue, data);
    return RET_OK;
}

S64 DevQueue_Write16Bit(S64 id, U16 data) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Queue Init err");
    volatile Queue_node *p_Queue = m_queue_p_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), RET_ERR, "Queue list err");
    DevQueue_PushNBit(p_Queue, data);
    return RET_OK;
}

S64 DevQueue_Write32Bit(S64 id, U32 data) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Queue Init err");
    volatile Queue_node *p_Queue = m_queue_p_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), RET_ERR, "Queue list err");
    DevQueue_PushNBit(p_Queue, data);
    return RET_OK;
}

S64 DevQueue_Write64Bit(S64 id, U64 data) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Queue Init err");
    volatile Queue_node *p_Queue = m_queue_p_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), RET_ERR, "Queue list err");
    DevQueue_PushNBit(p_Queue, data);
    return RET_OK;
}

S64 DevQueue_Write(S64 id, U8 *data, S64 size) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Queue Init err");
    DEV_IF_LOGE_RETURN_RET((data == NULL), RET_ERR_ARG, "Queue arg err");
    volatile Queue_node *p_Queue = m_queue_p_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), 0, "Queue list err");
    DEV_IF_LOGE_RETURN_RET((p_Queue->data == NULL), 0, "Queue data err");
    DEV_IF_LOGE_RETURN_RET((p_Queue->size - p_Queue->atomic_unreadSize.load() < (S64 ) size), 0, "Queue Buffer is full");
    Dev_memcpy((void*)(&p_Queue->data[p_Queue->writeIndex]), (void*)data, size);
    p_Queue->atomic_unreadSize.fetch_add(size);
    p_Queue->writeIndex += (S64)size;
    if (p_Queue->writeIndex == p_Queue->size) {
        p_Queue->writeIndex = 0;
    }
    return size;
}

/**
 * @brief       从消息队列中获取消息
 * @details
 * @param[in]   id  消息ID
 * @param[out]  data    消息内容
 * @param[out]  size    消息内容长度
 * @return
 * @return
 * @see
 * @note
 * @par
 */
S64 DevQueue_Read8Bit(S64 id, U8 *data) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Queue Init err");
    DEV_IF_LOGE_RETURN_RET((data == NULL), RET_ERR_ARG, "Queue arg err");
    volatile Queue_node *p_Queue = m_queue_p_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), RET_ERR, "Queue list err");
    DevQueue_PopNBit(p_Queue, *data);
    return RET_OK;
}

S64 DevQueue_Read16Bit(S64 id, U16 *data) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Queue Init err");
    DEV_IF_LOGE_RETURN_RET((data == NULL), RET_ERR_ARG, "Queue arg err");
    volatile Queue_node *p_Queue = m_queue_p_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), RET_ERR, "Queue list err");
    DevQueue_PopNBit(p_Queue, *data);
    return RET_OK;
}

S64 DevQueue_Read32Bit(S64 id, U32 *data) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Queue Init err");
    DEV_IF_LOGE_RETURN_RET((data == NULL), RET_ERR_ARG, "Queue arg err");
    volatile Queue_node *p_Queue = m_queue_p_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), RET_ERR, "Queue list err");
    DevQueue_PopNBit(p_Queue, *data);
    return RET_OK;
}

S64 DevQueue_Read64Bit(S64 id, U64 *data) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Queue Init err");
    DEV_IF_LOGE_RETURN_RET((data == NULL), RET_ERR_ARG, "Queue arg err");
    volatile Queue_node *p_Queue = m_queue_p_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), RET_ERR, "Queue list err");
    DevQueue_PopNBit(p_Queue, *data);
    return RET_OK;
}

S64 DevQueue_Read(S64 id, U8 *data, S64 size) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Queue Init err");
    DEV_IF_LOGE_RETURN_RET((data == NULL), 0, "Queue arg err");
    volatile Queue_node *p_Queue = m_queue_p_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), 0, "Queue list err");
    DEV_IF_LOGE_RETURN_RET((p_Queue->data == NULL), 0, "Queue data err");
    DEV_IF_LOGE_RETURN_RET((p_Queue->atomic_unreadSize.load() == 0), 0, "Queue Buffer is Empty");
    S64 readSize = size;
    if (p_Queue->atomic_unreadSize.load() < (S64)size) {
        DEV_LOGE("The buffer is not enough to read size[%ld:%ld]", readSize, size);
        readSize = p_Queue->atomic_unreadSize.load();
    }
    Dev_memcpy((void*)data, (void*)(&p_Queue->data[p_Queue->readIndex]), readSize);
    p_Queue->atomic_unreadSize.fetch_sub(readSize);
    p_Queue->readIndex += (S64)readSize;
    if (p_Queue->readIndex == p_Queue->size) {
        p_Queue->readIndex = 0;
    }
    return readSize;
}

/**
 * @brief       获取消息队列中消息的数量
 * @details
 * @param[in]   id  消息ID
 * @param[in]
 * @return      无
 * @return      无
 * @see
 * @note
 * @par
 */
S64 DevQueue_Count(S64 id) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Queue Init err");
    volatile Queue_node *p_Queue = m_queue_p_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), 0, "Queue list err");
    return p_Queue->atomic_unreadSize.load();
}

/**
 * @brief       获取消息队列中消息可用的数量
 * @details
 * @param[in]   id  消息ID
 * @param[in]
 * @return      无
 * @return      无
 * @see
 * @note
 * @par
 */
S64 DevQueue_Available(S64 id) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Queue Init err");
    volatile Queue_node *p_Queue = m_queue_p_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), 0, "Queue list err");
    if (p_Queue->size - p_Queue->atomic_unreadSize.load() > 0) {
        return p_Queue->size - p_Queue->atomic_unreadSize.load();
    } else {
        return 0;
    }
}

/**
 * @brief       the number of maximum contiguous unused elements in the fifo
 * @details
 * @param[in]   id  消息ID
 * @param[in]
 * @return      无
 * @return      无
 * @see
 * @note
 * @par
 */
S64 DevQueue_ContigAvailable(S64 id) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Queue Init err");
    volatile Queue_node *p_Queue = m_queue_p_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), 0, "Queue list err");
    if (p_Queue->writeIndex > p_Queue->readIndex) {
        return MAX(p_Queue->size - p_Queue->writeIndex, p_Queue->readIndex);
    } else if (p_Queue->writeIndex < p_Queue->readIndex) {
        return p_Queue->readIndex - p_Queue->writeIndex;
    } else {
        return p_Queue->size - p_Queue->atomic_unreadSize.load();
    }
}

/**
 * @brief       清楚消息队列中消息
 * @details
 * @param[in]   id  消息ID
 * @param[in]
 * @return      无
 * @return      无
 * @see
 * @note
 * @par
 */
S64 DevQueue_Clean(S64 id) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Queue Init err");
    volatile Queue_node *p_Queue = m_queue_p_list[id];
    DEV_IF_LOGE_RETURN_RET((p_Queue == NULL), RET_ERR, "Queue list err");
    p_Queue->atomic_unreadSize.store(0);
    p_Queue->readIndex = 0;
    p_Queue->writeIndex = 0;
    p_Queue->activeReadSize = 0;
    p_Queue->activeWriteSize = 0;
    Dev_memset(p_Queue->data, 0, p_Queue->size);
    return RET_OK;
}

/**
 * @brief       向消息队列去初始化
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
S64 DevQueue_Destroy(S64 *pId) {
    if (init_f != TRUE) {
        return RET_OK;
    }
    DEV_IF_LOGE_RETURN_RET((pId==NULL), RET_ERR, "Msg id err");
    DEV_IF_LOGE_RETURN_RET(((*pId <= 0)||(*pId>=MSG_QUEUE_MAX_NUM)), RET_ERR, "Msg Id err");
    Device->mutex->Lock(m_mutex_Id);
    S64 ret = RET_OK;
    S64 m_id = 0;
    volatile Queue_node *p_Queue = NULL;
    p_Queue = m_queue_p_list[*pId];
    if (p_Queue == NULL) {
        *pId = 0;
        ret = RET_ERR;
        goto exit;
    }
    if (m_queue_p_list[*pId]->pId == NULL) {
        *pId = 0;
        ret = RET_ERR;
        goto exit;
    }
    DevQueue_Clean(*pId);
    p_Queue->size = 0;
    m_id = *m_queue_p_list[*pId]->pId;
    DEV_IF_LOGE_RETURN_RET(((m_id <= 0)||(m_id>=MSG_QUEUE_MAX_NUM)), RET_ERR, "Msg Id err");
//    *m_queue_p_list[*pId]->pId = 0;
    if (p_Queue->data != NULL) {
        Dev_free(p_Queue->data);
        p_Queue->data = NULL;
    }
    if (p_Queue != NULL) {
        Dev_free((void*)p_Queue);
        p_Queue = NULL;
    }
    m_queue_p_list[m_id] = NULL;
    *pId = 0;
    exit:
    Device->mutex->Unlock(m_mutex_Id);
    return ret;
}

S64 DevQueue_Report(void) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_LOGI("[-------------------------------------QUEUE REPORT SATRT--------------------------------------]");
    Device->mutex->Lock(m_mutex_Id);
    for (int i = 0; i < MSG_QUEUE_MAX_NUM; i++) {
        if (m_queue_p_list[i] != NULL) {
            if ((m_queue_p_list[i]->pId != NULL) && (m_queue_p_list[i]->data != NULL)) {
                DEV_LOGI("[---QUEUE [%d][%s][%s][%d]pId[%p]TIMESTAMP[%ld]KEEP[%ldMS]DATA[%p]SIZE[%ld/%d/%ld]---]"
                        , i + 1
                        , m_queue_p_list[i]->__tagName
                        , m_queue_p_list[i]->__fileName
                        , m_queue_p_list[i]->__fileLine
                        , m_queue_p_list[i]->pId
                        , m_queue_p_list[i]->createTimestamp
                        , ((Device->time->GetTimestampMs()-m_queue_p_list[i]->createTimestamp)>0?(Device->time->GetTimestampMs()-m_queue_p_list[i]->createTimestamp):0)
                        , m_queue_p_list[i]->data
                        , (((S64 )(m_queue_p_list[i]->data) - m_queue_p_list[i]->readIndex) > 0 ?((S64 )(m_queue_p_list[i]->data) - m_queue_p_list[i]->readIndex) : 0)
                        , m_queue_p_list[i]->atomic_unreadSize.load()
                        , m_queue_p_list[i]->size
                        );
            }
        }
    }
    Device->mutex->Unlock(m_mutex_Id);
    DEV_LOGI("[-------------------------------------QUEUE REPORT END  --------------------------------------]");
    return RET_OK;
}

/**
 * @brief      消息队列模块初始化
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
S64 DevQueue_Init(void) {
    if (init_f == TRUE) {
        return RET_OK;
    }
    S32 ret = Device->mutex->Create(&m_mutex_Id, MARK("m_mutex_Id"));
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "Mutex Create  err");
    Dev_memset((void*)m_queue_p_list, 0, sizeof(m_queue_p_list));
    init_f = TRUE;
    return RET_OK;
}

/**
 * @brief      消息队列模块去初始化
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
S64 DevQueue_Deinit(void) {
    if (init_f != TRUE) {
        return RET_OK;
    }
    S32 report = 0;
    for (int i = 0; i < MSG_QUEUE_MAX_NUM; i++) {
        if (m_queue_p_list[i] != NULL) {
            if ((m_queue_p_list[i]->pId != NULL) && (m_queue_p_list[i]->data != NULL)) {
                report++;
            }
        }
    }
    if (report > 0) {
        DevQueue_Report();
    }

    for (S64 id = 1; id < MSG_QUEUE_MAX_NUM; id++) {
        S64 idp = id;
        DevQueue_Destroy(&idp);
    }
    init_f = FALSE;
    Dev_memset(m_queue_p_list, 0, sizeof(m_queue_p_list));
    Device->mutex->Destroy(&m_mutex_Id);
    return RET_OK;
}

const Dev_Queue m_dev_queue = {
        .Init           = DevQueue_Init,
        .Deinit         = DevQueue_Deinit,
        .Create         = DevQueue_Create,
        .Destroy        = DevQueue_Destroy,
        .Write8Bit      = DevQueue_Write8Bit,
        .Write16Bit     = DevQueue_Write16Bit,
        .Write32Bit     = DevQueue_Write32Bit,
        .Write64Bit     = DevQueue_Write64Bit,
        .Write          = DevQueue_Write,
        .Read8Bit       = DevQueue_Read8Bit,
        .Read16Bit      = DevQueue_Read16Bit,
        .Read32Bit      = DevQueue_Read32Bit,
        .Read64Bit      = DevQueue_Read64Bit,
        .Read           = DevQueue_Read,
        .Empty          = DevQueue_Empty,
        .Count          = DevQueue_Count,
        .Available      = DevQueue_Available,
        .ContigAvailable= DevQueue_ContigAvailable,
        .Full           = DevQueue_Full,
        .Clean          = DevQueue_Clean,
        .GetWritePtr    = DevQueue_GetWritePtr,
        .MarkWriteDone  = DevQueue_MarkWriteDone,
        .GetReadPtr     = DevQueue_GetReadPtr,
        .MarkReadDone   = DevQueue_MarkReadDone,
        .Report         = DevQueue_Report,
};

