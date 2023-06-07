/**
 * @file        dev_memoryPool.h
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

#ifndef __DEV_MEMORYPOOL_H__
#define __DEV_MEMORYPOOL_H__

typedef struct __Dev_MemoryPool Dev_MemoryPool;
struct __Dev_MemoryPool {
    S64 (*Init)(void);
    S64 (*Deinit)(void);
    void* (*Malloc)(U32 size, MARK_TAG tagName);
    S64 (*Free)(void *addr);
    S64 (*Report)(void);
};

extern const Dev_MemoryPool m_dev_memoryPool;

#endif //__DEV_MEMORYPOOL_H__
