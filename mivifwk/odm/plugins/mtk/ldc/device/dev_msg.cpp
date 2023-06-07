/**
 * @file		dev_msg.c
 * @brief		消息队列模块 FIFO模式
 * @details		根据目标芯片的内存适当调整 MSG_QUEUE_MSG_MAX 最大消息的存储量，防止内存溢出
 * @author
 * @date        2019.07.03
 * @version		0.1
 * @note
 * @warning
 * @par			History:
 *
 */

#include "dev_msg.h"

#include "dev_log.h"
#include "dev_type.h"
#include "stdlib.h"
#include "string.h"

#define MSG_LIST_MAX_NUM (256) // 最大消息队列数量
#define MSG_DATA_MAX_NUM (256) // 最大消息数量

//#define CONFIG_PTHREAD_SUPPORT       (1)
//#undef CONFIG_PTHREAD_SUPPORT

#ifdef CONFIG_PTHREAD_SUPPORT
#include <pthread.h>
static pthread_mutex_t m_mutex;
#endif

#pragma pack(1)
/**
 * @brief		消息节点
 * @details
 * @note
 * @par
 */
typedef struct _MESSAGE
{
    S64 data_length;
    U8 *data;
    struct _MESSAGE *next;
    struct _MESSAGE *previous;
} Message;

/**
 * @brief		消息队列
 * @details
 * @note
 * @par
 */
typedef struct _CHANNEL
{
    S64 msg_Id;
    S64 type;
    S64 limit_max;
    S64 message_count;
    Message *first_messages;
    Message *last_message;
} Channel;
#pragma pack()

static volatile Channel *channel_list[MSG_LIST_MAX_NUM];
static S64 init_f = FALSE;

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
S64 DevMsg_Init(void)
{
    if (init_f == TRUE) {
        return RET_OK;
    }
    memset(channel_list, 0, sizeof(channel_list));
#ifdef CONFIG_PTHREAD_SUPPORT
    S32 ret = pthread_mutex_init(&m_mutex, NULL);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "Mutex Create  err");
#endif
    init_f = TRUE;
    return RET_OK;
}

/**
 * @brief		创建消息队列
 * @details
 * @param[out]   msgId  消息ID
 * @return
 * @return
 * @see
 * @note
 * @par
 */
S64 DevMsg_Create(Enum_MSG_TYPE type, S64 msg_num_max)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Msg Init err");
#ifdef CONFIG_PTHREAD_SUPPORT
    S64 ret = pthread_mutex_lock(&m_mutex);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "mutex Lock err");
#endif
    S64 id = 0;
    volatile Channel *channel = NULL;
    for (id = 1; id < MSG_LIST_MAX_NUM; id++) {
        if (channel_list[id] == NULL) {
            break;
        }
    }
    DEV_IF_LOGE_GOTO((id >= MSG_LIST_MAX_NUM), exit, "Msg Create Insufficient quantity");
    channel = (Channel *)malloc(sizeof(Channel));
    DEV_IF_LOGE_GOTO((channel == NULL), exit, "Msg channel malloc err");
    if (msg_num_max == 0) {
        msg_num_max = MSG_DATA_MAX_NUM;
    }
    channel->msg_Id = id;
    channel->message_count = 0;
    channel->first_messages = NULL;
    channel->last_message = NULL;
    channel->type = type;
    channel->limit_max = msg_num_max;
    channel_list[id] = channel;
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_unlock(&m_mutex);
#endif
    return channel->msg_Id;
exit:
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_unlock(&m_mutex);
#endif
    return 0;
}

/**
 * @brief		查看当前地址是否被使用
 * @details		在MSG_TYPE_INDEX 模式下需要写的地址是否安全
 * @param[in]   msgId  消息ID
 * @param[in]	data	消息地址
 * @param[in]	data_length	消息内容长度
 * @return
 * @return
 * @see
 * @note
 * @par
 */
S64 DevMsg_Try(S64 id, void *data, S64 length)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Msg Init err");
    DEV_IF_LOGE_RETURN_RET(((id <= 0) || (id >= MSG_LIST_MAX_NUM)), RET_ERR, "Msg Id err");
#ifdef CONFIG_PTHREAD_SUPPORT
    S64 ret = pthread_mutex_lock(&m_mutex);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "mutex Lock err");
#endif
    volatile Channel *channel = channel_list[id];
    Message *message = NULL;
    S64 i = 0;
    S64 ret_ok = 0;
    long addr = (long)data;
    DEV_IF_LOGE_GOTO((channel == NULL), exit, "Msg channel err");
    DEV_IF_LOGE_GOTO((length <= 0), exit, "Msg length err");
    if (channel->message_count >=
        channel->limit_max) { //超过存储的最大数量了，不在保存新的消息了
                              //		info("msg FULL %ld\n", msgId);
        goto exit;
    }
    if ((channel->type & MSG_TYPE_INDEX) == MSG_TYPE_INDEX) {
        message = channel->first_messages;
        for (i = 0; i < channel->message_count; i++) {
            if ((U8 *)(addr + length) <= (message->data) ||
                ((U8 *)addr > (message->data + message->data_length))) {
            } else {
                ret_ok++; //数据不安全
            }
            message = message->next;
        }
        if (ret_ok != 0) {
            DEV_LOGE("addr is used");
            goto exit;
        }
    } else { // MSG_TYPE_DATA	// malloc到数据本身认为安全
    }
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_unlock(&m_mutex);
#endif
    return RET_OK;
exit:
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_unlock(&m_mutex);
#endif
    return RET_ERR;
}

/**
 * @brief		向消息队列中发送消息
 * @details
 * @param[in]   msgId  消息ID
 * @param[in]	data	消息内容
 * @param[in]	data_length	消息内容长度
 * @return
 * @return
 * @see
 * @note
 * @par
 */
S64 DevMsg_Write(S64 id, void *data, S64 length)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Msg Init err");
    DEV_IF_LOGE_RETURN_RET(((id <= 0) || (id >= MSG_LIST_MAX_NUM)), 0, "Msg Id err");
#ifdef CONFIG_PTHREAD_SUPPORT
    S64 ret = pthread_mutex_lock(&m_mutex);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "mutex Lock err");
#endif
    volatile Channel *channel = channel_list[id];
    Message *message = NULL;
    DEV_IF_LOGE_GOTO((channel == NULL), exit, "Msg channel err");
    DEV_IF_LOGE_GOTO((length <= 0), exit, "Msg length err");
    if (channel->message_count > channel->limit_max) { //超过存储的最大数量了，不在保存新的消息了
        if ((channel->type & MSG_TYPE_ABANDON_LAST) == MSG_TYPE_ABANDON_LAST) {
            //			info("MSG FULL ABANDON %ld\n", msgId);
            goto exit;
        }
    }
    message = (Message *)malloc(sizeof(Message));
    DEV_IF_LOGE_GOTO((message == NULL), exit, "Msg data err");
    if ((channel->type & MSG_TYPE_INDEX) == MSG_TYPE_INDEX) {
        message->data = (U8 *)data;
    } else { // MSG_TYPE_DATA
        message->data = (U8 *)malloc(length);
        DEV_IF_LOGE_GOTO((message == NULL), exit, "Msg data err");
        if (message->data == NULL) {
            DEV_LOGE("data malloc IS NULL");
            free(message);
            message = NULL;
            goto exit;
        }
        memcpy(message->data, data, length);
    }
    message->data_length = length;
    if (channel->first_messages == NULL) { //第一个消息
        channel->first_messages = message;
        channel->last_message = message;
        channel->first_messages->next = message;
        channel->last_message->next = message;
        channel->first_messages->previous = message;
        channel->last_message->previous = message;
    } else {
        message->previous = channel->last_message; //保存新节点的前节点地址
        message->next = channel->first_messages;   //循环队列节点
        channel->last_message->next = message;     //增加新的节点连接
        channel->last_message = message;           //新节点内容
    }
    channel->message_count++;
    if (channel->message_count > channel->limit_max) { //超过存储的最大数量了，放弃最老的数据
        if ((channel->type & MSG_TYPE_ABANDON_FIRST) == MSG_TYPE_ABANDON_FIRST) {
            message = channel->first_messages;
            channel->first_messages = message->next;
            channel->first_messages->previous = message->previous;
            if ((channel->type & MSG_TYPE_INDEX) == MSG_TYPE_INDEX) {
            } else { // MSG_TYPE_DATA
                free(message->data);
                message->data = NULL;
            }
            channel->message_count--;
            free(message);
            message = NULL;
            //			info("MSG FULL ABANDON FIRST %ld\n", msgId);
            goto exit;
        }
    }
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_unlock(&m_mutex);
#endif
    return length;
exit:
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_unlock(&m_mutex);
#endif
    return 0;
}

/**
 * @brief		从消息队列中获取消息
 * @details
 * @param[in]   msgId  消息ID
 * @param[out]	data	消息内容
 * @param[out]	data_length	消息内容长度
 * @return
 * @return
 * @see
 * @note
 * @par
 */
S64 DevMsg_Read(S64 id, void *data, S64 length)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Msg Init err");
    DEV_IF_LOGE_RETURN_RET(((id <= 0) || (id >= MSG_LIST_MAX_NUM)), 0, "Msg Id err");
#ifdef CONFIG_PTHREAD_SUPPORT
    S64 ret = pthread_mutex_lock(&m_mutex);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "mutex Lock err");
#endif
    volatile Channel *channel = channel_list[id];
    S64 ret_length = 0;
    Message *message = NULL;
    DEV_IF_LOGE_GOTO((channel == NULL), exit, "Msg channel err");
    DEV_IF_LOGE_GOTO((channel->first_messages == NULL), exit, "Msg data err");
    if (channel->message_count == 0) {
        goto exit;
    }
    if ((channel->type & MSG_TYPE_STACK) == MSG_TYPE_STACK) {
        //提取最后入队列的消息
        message = channel->last_message;
        DEV_IF_LOGE_GOTO((!message->data_length), exit, "Msg length err");
        if (channel->message_count == 1) { //最后一个了，后面的节点应该是NULL了
            channel->first_messages = NULL;
            channel->last_message = NULL;
        } else {
            channel->last_message = message->previous;
        }
    } else { // MSG_TYPE_FIFO
        //提取最先入队列的消息
        message = channel->first_messages;
        DEV_IF_LOGE_GOTO((!message->data_length), exit, "Msg length err");
        if (channel->message_count == 1) { //最后一个了，后面的节点应该是NULL了
            channel->first_messages = NULL;
            channel->last_message = NULL;
        } else {
            channel->first_messages = message->next;
        }
    }
    if ((channel->type & MSG_TYPE_INDEX) == MSG_TYPE_INDEX) {
        memcpy(data, &message->data, sizeof(U8 *));
    } else { // MSG_TYPE_DATA
        DEV_IF_LOGE_GOTO((message == NULL), exit, "Msg data err");
        DEV_IF_LOGE_GOTO((data == NULL), exit, "Msg data err");
        memcpy(data, message->data, message->data_length);
        free(message->data);
        message->data = NULL;
    }
    ret_length = message->data_length;
    channel->message_count--;
    free(message);
    message = NULL;
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_unlock(&m_mutex);
#endif
    return ret_length;
exit:
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_unlock(&m_mutex);
#endif
    return 0;
}

/**
 * @brief		从消息队列中获取消息的长度
 * @details
 * @param[in]   msgId  消息ID
 * @param[out]
 * @param[out]
 * @return		消息内容长度
 * @return
 * @see
 * @note
 * @par
 */
S64 DevMsg_Length(S64 id)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Msg Init err");
    DEV_IF_LOGE_RETURN_RET(((id <= 0) || (id >= MSG_LIST_MAX_NUM)), 0, "Msg Id err");
#ifdef CONFIG_PTHREAD_SUPPORT
    S64 ret = pthread_mutex_lock(&m_mutex);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), 0, "mutex Lock err");
#endif
    volatile Channel *channel = channel_list[id];
    Message *message = NULL;
    DEV_IF_LOGE_GOTO((channel == NULL), exit, "Msg channel err");
    DEV_IF_LOGE_GOTO((channel->first_messages == NULL), exit, "Msg data err");
    if (channel->message_count == 0) {
        goto exit;
    }
    if ((channel->type & MSG_TYPE_STACK) == MSG_TYPE_STACK) {
        //提取最后入队列的消息
        message = channel->last_message;
        DEV_IF_LOGE_GOTO((!message->data_length), exit, "Msg length err");
    } else { // MSG_TYPE_FIFO
        //提取最先入队列的消息
        message = channel->first_messages;
        DEV_IF_LOGE_GOTO((!message->data_length), exit, "Msg length err");
    }
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_unlock(&m_mutex);
#endif
    return message->data_length;
    ;
exit:
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_unlock(&m_mutex);
#endif
    return 0;
}

/**
 * @brief		判断消息队列是否为空
 * @details
 * @param[in]   msgId  消息ID
 * @param[in]
 * @return  	TRUE 空
 * @return  	FALSE 非空
 * @see
 * @note
 * @par
 */
S64 DevMsg_Empty(S64 id)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), TRUE, "Msg Init err");
    DEV_IF_LOGE_RETURN_RET(((id <= 0) || (id >= MSG_LIST_MAX_NUM)), TRUE, "Msg Id err");
    volatile Channel *channel = channel_list[id];
    DEV_IF_LOGE_RETURN_RET((channel == NULL), TRUE, "Msg list err");
    return channel->message_count ? FALSE : TRUE;
}

/**
 * @brief		判断消息队列是否为满
 * @details
 * @param[in]   msgId  消息ID
 * @param[in]
 * @return  	TRUE 满
 * @return  	FALSE 非满
 * @see
 * @note
 * @par
 */
S64 DevMsg_Full(S64 id)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), TRUE, "Msg Init err");
    DEV_IF_LOGE_RETURN_RET(((id <= 0) || (id >= MSG_LIST_MAX_NUM)), TRUE, "Msg Id err");
    volatile Channel *channel = channel_list[id];
    DEV_IF_LOGE_RETURN_RET((channel == NULL), TRUE, "Msg list err");
    return channel->message_count >= channel->limit_max ? TRUE : FALSE;
}

/**
 * @brief		获取消息队列中消息的数量
 * @details
 * @param[in]   msgId  消息ID
 * @param[in]
 * @return  	无
 * @return  	无
 * @see
 * @note
 * @par
 */
S64 DevMsg_Count(S64 id)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Msg Init err");
    DEV_IF_LOGE_RETURN_RET(((id <= 0) || (id >= MSG_LIST_MAX_NUM)), 0, "Msg Id err");
    volatile Channel *channel = channel_list[id];
    DEV_IF_LOGE_RETURN_RET((channel == NULL), 0, "Msg list err");
    return channel->message_count;
}

/**
 * @brief		获取消息队列中消息可用的数量
 * @details
 * @param[in]   msgId  消息ID
 * @param[in]
 * @return  	无
 * @return  	无
 * @see
 * @note
 * @par
 */
S64 DevMsg_Available(S64 id)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), 0, "Msg Init err");
    DEV_IF_LOGE_RETURN_RET(((id <= 0) || (id >= MSG_LIST_MAX_NUM)), 0, "Msg Id err");
    volatile Channel *channel = channel_list[id];
    DEV_IF_LOGE_RETURN_RET((channel == NULL), 0, "Msg list err");
    return channel->limit_max - channel->message_count;
}

/**
 * @brief		清楚消息队列中消息
 * @details
 * @param[in]   msgId  消息ID
 * @param[in]
 * @return  	无
 * @return  	无
 * @see
 * @note
 * @par
 */
S64 DevMsg_Clean(S64 id)
{
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "Msg Init err");
    DEV_IF_LOGE_RETURN_RET(((id <= 0) || (id >= MSG_LIST_MAX_NUM)), RET_ERR, "Msg Id err");
#ifdef CONFIG_PTHREAD_SUPPORT
    S64 ret = pthread_mutex_lock(&m_mutex);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "mutex Lock err");
#endif
    volatile Channel *channel = channel_list[id];
    Message *message = channel->first_messages;
    DEV_IF_LOGE_GOTO((channel == NULL), exit, "Msg channel err");
    //    DEV_IF_LOGE_GOTO((message == NULL), exit, "Msg data err");
    if (message != NULL) { //没有任何数据被存储。
        for (S64 i = 0; i < channel->message_count; i++) {
            if ((channel->type & MSG_TYPE_INDEX) == MSG_TYPE_INDEX) {
                message->data = NULL;
            } else { // MSG_TYPE_DATA
                free(message->data);
                message->data = NULL;
            }
            message->data_length = 0;
            message = message->next;
        }
    }
    channel->message_count = 0;
    channel->first_messages = NULL;
    channel->last_message = NULL;
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_unlock(&m_mutex);
#endif
    return RET_OK;
exit:
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_unlock(&m_mutex);
#endif
    return RET_ERR;
}

/**
 * @brief		向消息队列去初始化
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
S64 DevMsg_Destroy(S64 *id)
{
    if (init_f != TRUE) {
        return RET_OK;
    }
    DEV_IF_LOGE_RETURN_RET((id == NULL), RET_ERR, "Msg id err");
    DEV_IF_LOGE_RETURN_RET(((*id <= 0) || (*id >= MSG_LIST_MAX_NUM)), RET_ERR, "Msg Id err");
#ifdef CONFIG_PTHREAD_SUPPORT
    S64 ret = pthread_mutex_lock(&m_mutex);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "mutex Lock err");
#endif
    volatile Channel *channel = channel_list[*id];
    if (channel == NULL) {
        *id = 0;
#ifdef CONFIG_PTHREAD_SUPPORT
        pthread_mutex_unlock(&m_mutex);
#endif
        return RET_OK;
    }
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_unlock(&m_mutex);
#endif
    DevMsg_Clean(*id);
#ifdef CONFIG_PTHREAD_SUPPORT
    ret = pthread_mutex_lock(&m_mutex);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "mutex Lock err");
#endif
    free((void *)channel);
    channel = NULL;
    channel_list[*id] = NULL;
    *id = 0;
#ifdef CONFIG_PTHREAD_SUPPORT
    pthread_mutex_unlock(&m_mutex);
#endif
    return RET_OK;
}

/**
 * @brief		消息队列模块去初始化
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
S64 DevMsg_Uninit(void)
{
    if (init_f != TRUE) {
        return RET_OK;
    }
    for (S64 id = 1; id < MSG_LIST_MAX_NUM; id++) {
        S64 idp = id;
        DevMsg_Destroy(&idp);
    }
    memset(channel_list, 0, sizeof(channel_list));
#ifdef CONFIG_PTHREAD_SUPPORT
    S64 ret = pthread_mutex_destroy(&m_mutex);
    DEV_IF_LOGE((ret != RET_OK), "mutex Destroy");
#endif
    init_f = FALSE;
    return RET_OK;
}

const Dev_Msg m_dev_msg = {
    .Init = DevMsg_Init,
    .Uninit = DevMsg_Uninit,
    .Create = DevMsg_Create,
    .Destroy = DevMsg_Destroy,
    .Write = DevMsg_Write,
    .Read = DevMsg_Read,
    .Try = DevMsg_Try,
    .Empty = DevMsg_Empty,
    .Full = DevMsg_Full,
    .Count = DevMsg_Count,
    .Available = DevMsg_Available,
    .Length = DevMsg_Length,
    .Clean = DevMsg_Clean,
};
