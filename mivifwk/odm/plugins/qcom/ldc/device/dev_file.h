/**
 * @file        dev_file.h
 * @brief
 * @details
 * @author
 * @date        2019.07.03
 * @version     0.1
 * @note
 * @warning
 * @par
 *
 */

#ifndef __DEV_FILE_H__
#define __DEV_FILE_H__

typedef struct __Dev_File Dev_File;
struct __Dev_File {
    S64 (*Open)(S64 *pFd, const char *name, MARK_TAG tagName);
    S64 (*Read)(S64 fd, void *buf, S64 size, S64 offset);
    S64 (*Write)(S64 fd, void *buf, S64 size, S64 offset);
    S64 (*Close)(S64 *pFd);
    S64 (*Delete)(const char* name);
    S64 (*GetSize)(const char* name);
    S64 (*WriteOnce)(const char* name, void *buf, S64 size, S64 offset);
    S64 (*ReadOnce)(const char* name, void *buf, S64 size, S64 offset);
};

extern const Dev_File m_dev_file;

#endif // __DEV_FILE_H__
