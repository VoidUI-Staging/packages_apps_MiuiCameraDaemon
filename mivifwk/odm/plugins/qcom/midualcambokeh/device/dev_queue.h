/**
 * @file        dev_queue.h
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

#ifndef __DEV_QUEUE_H__
#define __DEV_QUEUE_H__

typedef struct __Dev_Queue Dev_Queue;
struct __Dev_Queue {
    S64 (*Init)(void);
    S64 (*Deinit)(void);
    S64 (*Create)(S64 *pId, S64 size, MARK_TAG tagName);
    S64 (*Destroy)(S64 *pId);
    S64 (*Write8Bit)(S64 id, U8 data);
    S64 (*Write16Bit)(S64 id, U16 data);
    S64 (*Write32Bit)(S64 id, U32 data);
    S64 (*Write64Bit)(S64 id, U64 data);
    S64 (*Write)(S64 id, U8 *data, S64 size);
    S64 (*Read8Bit)(S64 id, U8* data);
    S64 (*Read16Bit)(S64 id, U16* data);
    S64 (*Read32Bit)(S64 id, U32* data);
    S64 (*Read64Bit)(S64 id, U64* data);
    S64 (*Read)(S64 id, U8* data, S64 size);
    S64 (*Empty)(S64 id);
    S64 (*Count)(S64 id);
    S64 (*Available)(S64 id);
    S64 (*ContigAvailable)(S64 id);
    S64 (*Full)(S64 id);
    S64 (*Clean)(S64 id);
    S64 (*GetWritePtr)(S64 id, void ** ptr, S64 size);
    S64 (*MarkWriteDone)(S64 id);
    S64 (*GetReadPtr)(S64 id, void ** ptr, S64 size);
    S64 (*MarkReadDone)(S64 id);
    S64 (*Report)(void);
};

extern const Dev_Queue m_dev_queue;
#endif //__DEV_QUEUE_H__

