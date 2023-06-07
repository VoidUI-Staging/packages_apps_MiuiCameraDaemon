/**
 * @file       dev_msg.c
 * @brief      消息队列模块 FIFO模式
 * @details    根据目标芯片的内存适当调整 MSG_QUEUE_MSG_MAX 最大消息的存储量，防止内存溢出
 * @author
 * @date       2019.07.03
 * @version    0.1
 * @note
 * @warning
 * @par        History:
 *
 */

#include "string.h"
#include "device.h"
#include "dev_msg.h"

#define MSG_LIST_MAX_NUM        (256)   // 最大消息队列数量
#define MSG_DATA_MAX_NUM        (256)   // 最大消息数量

static S64 m_mutex_Id;

#pragma pack(1)
/**
 * @brief        消息节点
 * @details
 * @note
 * @par
 */
typedef struct __Message_Node {
    S64 size;
    U8 *data;
    S64 writeTimestamp;
    struct __Message_Node *next;
    struct __Message_Node *previous;
} Message_Node;

/**
 * @brief        消息队列
 * @details
 * @note
 * @par
 */
typedef struct __Message {
    S64 *pId;
    char __fileName[FILE_NAME_LEN_MAX];
    char __tagName[FILE_NAME_LEN_MAX];
    U32 __fileLine;
    S64 mutex_Id;
    S64 type;
    S64 limit_max;
    S64 message_count;
    Message_Node *first_messages;
    Message_Node *last_message;
} Message;
#pragma pack()

static volatile Message *m_message_p_list[MSG_LIST_MAX_NUM];
static S64 init_f = FALSE;

static const char* DevMsg_NoDirFileName(const char *pFilePath) {
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
 * @brief        消息队列模块初始化
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
S64 DevMsg_Init(void) {
    if (init_f == TRUE) {
        return RET_OK;
    }
    memset(m_message_p_list, 0, sizeof(m_message_p_list));
    S32 ret = Device->mutex->Create(&m_mutex_Id, MARK("m_mutex_Id"));
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "Mutex Create  err");
    init_f = TRUE;
    return RET_OK;
}

/**
 * @brief        创建消息队列
 * @details
 * @param[out]   msgId  消息ID
 * @return
 * @return
 * @see
 * @note
 * @par
 */
S64 DevMsg_Create(S64 *pId, Enum_MSG_TYPE type, S64 msg_num_max, MARK_TAG tagName) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Msg Init err");
    S64 ret = RET_ERR;
    ret = Device->mutex->Lock(m_mutex_Id);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "mutex Lock err");
    S64 m_id = 0;
    S64 id = 0;
    char msgTagName[FILE_NAME_LEN_MAX + 32] = {0};
    volatile Message *message = NULL;
    for (id = 1; id < MSG_LIST_MAX_NUM; id++) {
        if (m_message_p_list[id] != NULL) {
            if (m_message_p_list[id]->pId != NULL) {
                if (m_message_p_list[id]->pId == pId) {
                    m_id = id;
                    DEV_IF_LOGW_GOTO(m_id != 0, exit, "double Create %ld", m_id);
                    DEV_LOGE("not Destroy err=%ld", id);
                }
            }
        }
    }
    id = 0;
    for (id = 1; id < MSG_LIST_MAX_NUM; id++) {
        if (m_message_p_list[id] == NULL) {
            break;
        }
    }
    DEV_IF_LOGE_GOTO((id >= MSG_LIST_MAX_NUM), exit, "Msg Create Insufficient quantity");
    message = (Message*) Dev_malloc(sizeof(Message));
    DEV_IF_LOGE_GOTO((message == NULL), exit, "Msg Message Dev_malloc err");
    if (msg_num_max == 0) {
        msg_num_max = MSG_DATA_MAX_NUM;
    }
    message->message_count = 0;
    message->first_messages = NULL;
    message->last_message = NULL;
    message->type = type;
    message->limit_max = msg_num_max;
    Dev_snprintf(msgTagName, FILE_NAME_LEN_MAX + 32, "msgCreate_%s", tagName);
    Device->mutex->Create((S64*)&message->mutex_Id, __fileName, __fileLine, msgTagName);
    m_message_p_list[id] = message;
    m_id = id;
    *pId = m_id;
    m_message_p_list[id]->pId = pId;
    Dev_snprintf((char*) message->__fileName, FILE_NAME_LEN_MAX, "%s", DevMsg_NoDirFileName(__fileName));
    Dev_snprintf((char*) message->__tagName, FILE_NAME_LEN_MAX, "%s", tagName);
    message->__fileLine = __fileLine;
    ret = RET_OK;
    exit:
    Device->mutex->Unlock(m_mutex_Id);
    return ret;
}

/**
 * @brief       查看当前地址是否被使用
 * @details     在MSG_TYPE_INDEX 模式下需要写的地址是否安全
 * @param[in]   msgId  消息ID
 * @param[in]   data    消息地址
 * @param[in]   size    消息内容长度
 * @return
 * @return
 * @see
 * @note
 * @par
 */
S64 DevMsg_Try(S64 id, void *data, S64 size) {
    S64 ret = RET_OK;
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Msg Init err");
    DEV_IF_LOGE_RETURN_RET(((id <= 0)||(id>=MSG_LIST_MAX_NUM)), RET_ERR, "Msg Id err");
    volatile Message *message = m_message_p_list[id];
    Message_Node *messageNode = NULL;
    S64 i = 0;
    S64 ret_ok = 0;
    U8 *addr = (U8*) data;
    DEV_IF_LOGE_RETURN_RET((message == NULL), RET_ERR, "Msg Message err");
    ret = Device->mutex->Lock(message->mutex_Id);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "mutex Lock err");
    DEV_IF_LOGE_GOTO((size <= 0), exit, "Msg size err");
    if (message->message_count >= message->limit_max) { //超过存储的最大数量了，不在保存新的消息了
//        info("msg FULL %ld\n", msgId);
        goto exit;
    }
    if ((message->type & MSG_TYPE_INDEX) == MSG_TYPE_INDEX) {
        messageNode = message->first_messages;
        for (i = 0; i < message->message_count; i++) {
            if ((U8*) (addr + size) <= (messageNode->data) || ((U8*) addr > (messageNode->data + messageNode->size))) {
            } else {
                ret_ok++; //数据不安全
            }
            messageNode = messageNode->next;
        }
        if (ret_ok != 0) {
            DEV_LOGE("addr is used");
            goto exit;
        }
    } else { //MSG_TYPE_DATA    // malloc到数据本身认为安全
    }
    Device->mutex->Unlock(message->mutex_Id);
    return RET_OK;
    exit:
    Device->mutex->Unlock(message->mutex_Id);
    return RET_ERR;
}

/**
 * @brief       向消息队列中发送消息
 * @details
 * @param[in]   msgId  消息ID
 * @param[in]   data    消息内容
 * @param[in]   size    消息内容长度
 * @return
 * @return
 * @see
 * @note
 * @par
 */
S64 DevMsg_Write(S64 id, void *data, S64 size) {
    S64 ret = RET_OK;
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Msg Init err");
    DEV_IF_LOGE_RETURN_RET(((id <= 0)||(id>=MSG_LIST_MAX_NUM)), 0, "Msg Id err");
    volatile Message *message = m_message_p_list[id];
    Message_Node *messageNode = NULL;
    DEV_IF_LOGE_RETURN_RET((message == NULL), 0, "Msg Message err");
    ret = Device->mutex->Lock(message->mutex_Id);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "mutex Lock err");
    DEV_IF_LOGE_GOTO((size <= 0), exit, "Msg size err");
    if (message->message_count > message->limit_max) { //超过存储的最大数量了，不在保存新的消息了
        if ((message->type & MSG_TYPE_ABANDON_LAST) == MSG_TYPE_ABANDON_LAST) {
//            info("MSG FULL ABANDON %ld\n", msgId);
            goto exit;
        }
    }
    messageNode = (Message_Node*) Dev_malloc(sizeof(Message_Node));
    DEV_IF_LOGE_GOTO((messageNode == NULL), exit, "Msg data err");
    if ((message->type & MSG_TYPE_INDEX) == MSG_TYPE_INDEX) {
        messageNode->data = (U8*) data;
    } else { //MSG_TYPE_DATA
        messageNode->data = (U8*) Dev_malloc(size);
        DEV_IF_LOGE_GOTO((messageNode == NULL), exit, "Msg data err");
        if (messageNode->data == NULL) {
            DEV_LOGE("data malloc IS NULL");
            Dev_free(messageNode);
            messageNode = NULL;
            goto exit;
        }
        memcpy(messageNode->data, data, size);
    }
    messageNode->size = size;
    messageNode->writeTimestamp = Device->time->GetTimestampMs();
    if (message->first_messages == NULL) { //第一个消息
        message->first_messages = messageNode;
        message->last_message = messageNode;
        message->first_messages->next = messageNode;
        message->last_message->next = messageNode;
        message->first_messages->previous = messageNode;
        message->last_message->previous = messageNode;
    } else {
        messageNode->previous = message->last_message; //保存新节点的前节点地址
        messageNode->next = message->first_messages; //循环队列节点
        message->last_message->next = messageNode; //增加新的节点连接
        message->last_message = messageNode; //新节点内容
    }
    message->message_count++;
    if (message->message_count > message->limit_max) { //超过存储的最大数量了，放弃最老的数据
        if ((message->type & MSG_TYPE_ABANDON_FIRST) == MSG_TYPE_ABANDON_FIRST) {
            messageNode = message->first_messages;
            message->first_messages = messageNode->next;
            message->first_messages->previous = messageNode->previous;
            if ((message->type & MSG_TYPE_INDEX) == MSG_TYPE_INDEX) {
            } else { //MSG_TYPE_DATA
                Dev_free(messageNode->data);
                messageNode->data = NULL;
            }
            message->message_count--;
            Dev_free(messageNode);
            messageNode = NULL;
//            info("MSG FULL ABANDON FIRST %ld\n", msgId);
            goto exit;
        }
    }
    Device->mutex->Unlock(message->mutex_Id);
    return size;
    exit:
    Device->mutex->Unlock(message->mutex_Id);
    return 0;
}

/**
 * @brief       从消息队列中获取消息
 * @details
 * @param[in]   msgId  消息ID
 * @param[out]  data    消息内容
 * @param[out]  size    消息内容长度
 * @return
 * @return
 * @see
 * @note
 * @par
 */
S64 DevMsg_Read(S64 id, void *data, S64 size) {
    S64 data_size = 0;
    S64 ret = RET_OK;
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Msg Init err");
    DEV_IF_LOGE_RETURN_RET(((id <= 0)||(id>=MSG_LIST_MAX_NUM)), 0, "Msg Id err");
    volatile Message *message = m_message_p_list[id];
    Message_Node *messageNode = NULL;
    DEV_IF_LOGE_RETURN_RET((message == NULL), 0, "Msg Message err");
    ret = Device->mutex->Lock(message->mutex_Id);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "mutex Lock err");
    DEV_IF_LOGE_GOTO((message->first_messages == NULL), exit, "Msg data err");
    if (message->message_count == 0) {
        goto exit;
    }
    if ((message->type & MSG_TYPE_STACK) == MSG_TYPE_STACK) {
        //提取最后入队列的消息
        messageNode = message->last_message;
        DEV_IF_LOGE_GOTO((!messageNode->size), exit, "Msg size err");
        if (message->message_count == 1) { //最后一个了，后面的节点应该是NULL了
            message->first_messages = NULL;
            message->last_message = NULL;
        } else {
            message->last_message = messageNode->previous;
        }
    } else { //MSG_TYPE_FIFO
        //提取最先入队列的消息
        messageNode = message->first_messages;
        DEV_IF_LOGE_GOTO((!messageNode->size), exit, "Msg size err");
        if (message->message_count == 1) { //最后一个了，后面的节点应该是NULL了
            message->first_messages = NULL;
            message->last_message = NULL;
        } else {
            message->first_messages = messageNode->next;
        }
    }
    if ((message->type & MSG_TYPE_INDEX) == MSG_TYPE_INDEX) {
        memcpy(data, &messageNode->data, sizeof(U8*));
    } else { //MSG_TYPE_DATA
        DEV_IF_LOGE_GOTO((messageNode == NULL), exit, "Msg data err");
        DEV_IF_LOGE_GOTO((data == NULL), exit, "Msg data err");
        memcpy(data, messageNode->data, messageNode->size);
        Dev_free(messageNode->data);
        messageNode->data = NULL;
    }
    data_size = messageNode->size;
    message->message_count--;
    Dev_free(messageNode);
    messageNode = NULL;
    exit:
    Device->mutex->Unlock(message->mutex_Id);
    return data_size;
}

/**
 * @brief       从消息队列中获取消息的长度
 * @details
 * @param[in]   msgId  消息ID
 * @param[out]
 * @param[out]
 * @return      消息内容长度
 * @return
 * @see
 * @note
 * @par
 */
S64 DevMsg_Size(S64 id) {
    S64 size = 0;
    S64 ret = RET_OK;
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), size, "Msg Init err");
    DEV_IF_LOGE_RETURN_RET(((id <= 0)||(id>=MSG_LIST_MAX_NUM)), size, "Msg Id err");
    volatile Message *message = m_message_p_list[id];
    Message_Node *messageNode = NULL;
    DEV_IF_LOGE_RETURN_RET((message == NULL), size, "Msg Message err");
    ret = Device->mutex->Lock(message->mutex_Id);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "mutex Lock err");
    DEV_IF_LOGE_GOTO((message->first_messages == NULL), exit, "Msg data err");
    if (message->message_count == 0) {
        goto exit;
    }
    if ((message->type & MSG_TYPE_STACK) == MSG_TYPE_STACK) {
        //提取最后入队列的消息
        messageNode = message->last_message;
        DEV_IF_LOGE_GOTO((!messageNode->size), exit, "Msg size err");
    } else { //MSG_TYPE_FIFO
        //提取最先入队列的消息
        messageNode = message->first_messages;
        DEV_IF_LOGE_GOTO((!messageNode->size), exit, "Msg size err");
    }
    size = messageNode->size;
    exit:
    Device->mutex->Unlock(message->mutex_Id);
    return size;
}

/**
 * @brief       判断消息队列是否为空
 * @details
 * @param[in]   msgId  消息ID
 * @param[in]
 * @return      TRUE 空
 * @return      FALSE 非空
 * @see
 * @note
 * @par
 */
S64 DevMsg_Empty(S64 id) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), TRUE, "Msg Init err");
    DEV_IF_LOGE_RETURN_RET(((id <= 0)||(id>=MSG_LIST_MAX_NUM)), TRUE, "Msg Id err");
    volatile Message *message = m_message_p_list[id];
    DEV_IF_LOGE_RETURN_RET((message == NULL), TRUE, "Msg list err");
    return message->message_count ? FALSE : TRUE;
}

/**
 * @brief       判断消息队列是否为满
 * @details
 * @param[in]   msgId  消息ID
 * @param[in]
 * @return      TRUE 满
 * @return      FALSE 非满
 * @see
 * @note
 * @par
 */
S64 DevMsg_Full(S64 id) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), TRUE, "Msg Init err");
    DEV_IF_LOGE_RETURN_RET(((id <= 0)||(id>=MSG_LIST_MAX_NUM)), TRUE, "Msg Id err");
    volatile Message *message = m_message_p_list[id];
    DEV_IF_LOGE_RETURN_RET((message == NULL), TRUE, "Msg list err");
    return message->message_count >= message->limit_max ? TRUE : FALSE;
}

/**
 * @brief       获取消息队列中消息的数量
 * @details
 * @param[in]   msgId  消息ID
 * @param[in]
 * @return      无
 * @return      无
 * @see
 * @note
 * @par
 */
S64 DevMsg_Count(S64 id) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Msg Init err");
    DEV_IF_LOGE_RETURN_RET(((id <= 0)||(id>=MSG_LIST_MAX_NUM)), 0, "Msg Id err");
    volatile Message *message = m_message_p_list[id];
    DEV_IF_LOGE_RETURN_RET((message == NULL), 0, "Msg list err");
    return message->message_count;
}

/**
 * @brief       获取消息队列中消息可用的数量
 * @details
 * @param[in]   msgId  消息ID
 * @param[in]
 * @return      无
 * @return      无
 * @see
 * @note
 * @par
 */
S64 DevMsg_Available(S64 id) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Msg Init err");
    DEV_IF_LOGE_RETURN_RET(((id <= 0)||(id>=MSG_LIST_MAX_NUM)), 0, "Msg Id err");
    volatile Message *message = m_message_p_list[id];
    DEV_IF_LOGE_RETURN_RET((message == NULL), 0, "Msg list err");
    return message->limit_max - message->message_count;
}

/**
 * @brief       清楚消息队列中消息
 * @details
 * @param[in]   msgId  消息ID
 * @param[in]
 * @return      无
 * @return      无
 * @see
 * @note
 * @par
 */
S64 DevMsg_Clean(S64 id) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Msg Init err");
    DEV_IF_LOGE_RETURN_RET(((id <= 0)||(id>=MSG_LIST_MAX_NUM)), RET_ERR, "Msg Id err");
    S64 ret = RET_OK;
    volatile Message *message = m_message_p_list[id];
    Message_Node *messageNode = message->first_messages;
    DEV_IF_LOGE_RETURN_RET((message == NULL), RET_ERR, "Msg Message err");
    ret = Device->mutex->Lock(message->mutex_Id);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "mutex Lock err");
    //    DEV_IF_LOGE_GOTO((message == NULL), exit, "Msg data err");
    if (messageNode != NULL) {        //没有任何数据被存储。
        for (S64 i = 0; i < message->message_count; i++) {
            if ((message->type & MSG_TYPE_INDEX) == MSG_TYPE_INDEX) {
                messageNode->data = NULL;
            } else { //MSG_TYPE_DATA
                Dev_free(messageNode->data);
                messageNode->data = NULL;
            }
            messageNode->size = 0;
            messageNode = messageNode->next;
        }
    }
    message->message_count = 0;
    message->first_messages = NULL;
    message->last_message = NULL;
    exit:
    Device->mutex->Unlock(message->mutex_Id);
    return ret;
}

/**
 * @brief       向消息队列去初始化
 * @details
 * @param[in]   msgId  消息ID
 * @param[in]
 * @param[in]
 * @return
 * @return
 * @see
 * @note
 * @par
 */
S64 DevMsg_Destroy(S64 *pId) {
    if (init_f != TRUE) {
        return RET_OK;
    }
    DEV_IF_LOGE_RETURN_RET((pId==NULL), RET_ERR, "Msg id err");
    DEV_IF_LOGE_RETURN_RET(((*pId <= 0)||(*pId>=MSG_LIST_MAX_NUM)), RET_ERR, "Msg Id err");
    volatile Message *message = NULL;
    S64 ret = RET_OK;
    S64 m_id = 0;
    ret = Device->mutex->Lock(m_mutex_Id);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "mutex Lock err");
    message = m_message_p_list[*pId];
    if (message == NULL) {
        *pId = 0;
        ret = RET_OK;
        goto exit;
    }
    if (m_message_p_list[*pId]->pId == NULL) {
        *pId = 0;
        ret = RET_ERR;
        goto exit;
    }
    DevMsg_Clean(*pId);
    Device->mutex->Destroy((S64*)&message->mutex_Id);
    m_id = *m_message_p_list[*pId]->pId;
    DEV_IF_LOGE_RETURN_RET(((m_id <= 0)||(m_id>=MSG_LIST_MAX_NUM)), RET_ERR, "Msg Id err");
//    *m_message_p_list[*pId]->pId = 0;
    Dev_free((void*) message);
    message = NULL;
    m_message_p_list[m_id] = NULL;
    *pId = 0;
    exit:
    Device->mutex->Unlock(m_mutex_Id);
    return ret;
}

S64 DevMsg_Report(void) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_LOGI("[-------------------------------------MSG REPORT SATRT--------------------------------------]");
    Device->mutex->Lock(m_mutex_Id);
    for (int i = 0; i < MSG_LIST_MAX_NUM; i++) {
        if (m_message_p_list[i] != NULL) {
            if (m_message_p_list[i]->pId != NULL) {
                DEV_LOGI("[---MSG  [%d][%s][%s][%d]pId[%p]TYPE[%ld]COUNT[%ld/%ld]---]"
                        , i + 1
                        , m_message_p_list[i]->__tagName
                        , m_message_p_list[i]->__fileName
                        , m_message_p_list[i]->__fileLine
                        , m_message_p_list[i]->pId
                        , m_message_p_list[i]->type
                        , m_message_p_list[i]->message_count
                        , m_message_p_list[i]->limit_max
                        );

                volatile Message *message = m_message_p_list[i];
                Message_Node *messageNode = NULL;

                if ((message->type & MSG_TYPE_STACK) == MSG_TYPE_STACK) {
                    //提取最后入队列的消息
                    messageNode = message->last_message;
                    for (int j = 0; j < m_message_p_list[i]->message_count; j++) {
                        DEV_IF_LOGE_RETURN_RET((messageNode==NULL), RET_ERR, "messageNode err");
                        DEV_LOGI("[---DATA [%d/%ld]pId[%p]data[%p]LENGTH[%ld]TIMESTAMP[%ld]KEEP[%ldMS]---]"
                                , j + 1
                                , m_message_p_list[i]->message_count
                                , m_message_p_list[i]->pId
                                , messageNode->data
                                , messageNode->size
                                , messageNode->writeTimestamp
                                , Device->time->GetTimestampMs()-messageNode->writeTimestamp
                                );
                        messageNode = messageNode->previous;
                    }
                } else { //MSG_TYPE_FIFO
                    //提取最先入队列的消息
                    messageNode = message->first_messages;
                    for (int j = 0; j < m_message_p_list[i]->message_count; j++) {
                        DEV_IF_LOGE_RETURN_RET((messageNode==NULL), RET_ERR, "messageNode err");
                        DEV_LOGI("[---DATA [%d/%ld]pId[%p]data[%p]LENGTH[%ld]TIMESTAMP[%ld]KEEP[%ldMS]---]"
                                , j + 1
                                , m_message_p_list[i]->message_count
                                , m_message_p_list[i]->pId
                                , messageNode->data
                                , messageNode->size
                                , messageNode->writeTimestamp
                                , Device->time->GetTimestampMs()-messageNode->writeTimestamp
                                );
                        messageNode = messageNode->next;
                    }
                }
            }
        }
    }
    Device->mutex->Unlock(m_mutex_Id);
    DEV_LOGI("[-------------------------------------MSG REPORT END  --------------------------------------]");
    return RET_OK;
}

/**
 * @brief        消息队列模块去初始化
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
S64 DevMsg_Deinit(void) {
    if (init_f != TRUE) {
        return RET_OK;
    }
    S32 report = 0;
    for (int i = 0; i < MSG_LIST_MAX_NUM; i++) {
        if (m_message_p_list[i] != NULL) {
            report++;
        }
    }
    if (report > 0) {
        DevMsg_Report();
    }
    for (S64 id = 1; id < MSG_LIST_MAX_NUM; id++) {
        S64 idp = id;
        DevMsg_Destroy(&idp);
    }
    init_f = FALSE;
    memset(m_message_p_list, 0, sizeof(m_message_p_list));
    S64 ret = Device->mutex->Destroy(&m_mutex_Id);
    DEV_IF_LOGE((ret != RET_OK), "mutex Destroy");
    return RET_OK;
}

const Dev_Msg m_dev_msg = {
        .Init       = DevMsg_Init,
        .Deinit     = DevMsg_Deinit,
        .Create     = DevMsg_Create,
        .Destroy    = DevMsg_Destroy,
        .Write      = DevMsg_Write,
        .Read       = DevMsg_Read,
        .Try        = DevMsg_Try,
        .Empty      = DevMsg_Empty,
        .Full       = DevMsg_Full,
        .Count      = DevMsg_Count,
        .Available  = DevMsg_Available,
        .Size       = DevMsg_Size,
        .Clean      = DevMsg_Clean,
        .Report     = DevMsg_Report,
};
