/**
 * @file        dev_libPool.h
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

#ifndef __DEV_LIBPOOL_H__
#define __DEV_LIBPOOL_H__

#define DEV_LIBPOOL_LIB_OPEN_MAX          (64)

typedef void (*DEV_LIBPOOL_ENTRY)(void *arg);

typedef struct __DevLibPool_List {
    S32 num;
    char name[DEV_LIBPOOL_LIB_OPEN_MAX][FILE_NAME_LEN_MAX];
    DEV_LIBPOOL_ENTRY entry[DEV_LIBPOOL_LIB_OPEN_MAX];
} DEV_LIBPOOL_LIST;

typedef struct __Dev_LibPool Dev_LibPool;
struct __Dev_LibPool {
    S64 (*Init)(void);
    S64 (*Deinit)(void);
    S64 (*OpenDir)(const char *dir, DEV_LIBPOOL_LIST *libList, MARK_TAG tagName);
    S64 (*CloseDir)(const char *dir);
    S64 (*Open)(const char *dir, const char *name, DEV_LIBPOOL_ENTRY *entry, MARK_TAG tagName);
    S64 (*Close)(const char *dir, const char *name);
    S64 (*Report)(void);
};

extern const Dev_LibPool m_dev_libPool;

#endif //__DEV_LIBPOOL_H__
