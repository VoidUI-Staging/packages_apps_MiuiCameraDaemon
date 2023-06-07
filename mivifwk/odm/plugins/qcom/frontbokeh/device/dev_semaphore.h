/**
 * @file        dev_semaphore.h
 * @brief
 * @details
 * @author
 * @date        2019.07.03
 * @version     0.1
 * @note
 * @warning
 * @par         History:
 *
 */

#ifndef __DEV_SEMAPHORE_H__
#define __DEV_SEMAPHORE_H__

typedef struct __Dev_Semaphore Dev_Semaphore;
struct __Dev_Semaphore {
    S64 (*Init)(void);
    S64 (*Deinit)(void);
    S64 (*Create)(S64 *pId, MARK_TAG tagName);
    S64 (*Destroy)(S64 *pId);
    S64 (*TimedWait)(S64 id, U64 ms);
    S64 (*Wait)(S64 id);
    S64 (*Post)(S64 id);
    S64 (*Report)(void);
};

extern const Dev_Semaphore m_dev_semaphore;

#endif //__DEV_SEMAPHORE_H__

