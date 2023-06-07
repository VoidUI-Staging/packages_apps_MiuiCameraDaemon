/**
 * @file        dev_pthreadPool.h
 * @brief
 * @details
 * @author
 * @date        2021.07.03
 * @version     0.1
 * @note
 * @warning
 * @par         History:
 *
 */

#ifndef __DEV_PTHREADPOOL_H__
#define __DEV_PTHREADPOOL_H__

#define PTHREAD_POOL_MAX_WOKER          (6)//TODO 使用Setting控制数量

typedef void* (*DEV_PTHREAD_POOL_P_FUNCTION)(void *arg1,void *arg2);

typedef struct __Dev_PthreadPool Dev_PthreadPool;
struct __Dev_PthreadPool {
    S64 (*Init)(void);
    S64 (*Deinit)(void);
    S64 (*Post)(DEV_PTHREAD_POOL_P_FUNCTION, void *arg1, void *arg2, MARK_TAG tagName);
    S64 (*Report)(void);
    S64 (*Flush)(void);
};

extern const Dev_PthreadPool m_dev_pthreadPool;

#endif //__DEV_PTHREADPOOL_H__
