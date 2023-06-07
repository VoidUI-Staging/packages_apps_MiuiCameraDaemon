/**
 * @file        dev_trace.h
 * @brief
 * @details
 * @author
 * @date        2022.10.27
 * @version     0.1
 * @note
 * @warning
 * @par         History:
 *
 */

#ifndef __DEV_TRACE_H__
#define __DEV_TRACE_H__

typedef struct __Dev_Trace Dev_Trace;
struct __Dev_Trace {
    S64 (*Init)(void);
    S64 (*Deinit)(void);
    S64 (*Create)(S64 *pId, MARK_TAG tagName);
    S64 (*Destroy)(S64 *pId);
    S64 (*Begin)(S64 id, MARK_TAG tag);
    S64 (*End)(S64 id, MARK_TAG tag);
    S64 (*Message)(S64 id, MARK_TAG message);
    S64 (*AsyncBegin)(S64 id, MARK_TAG tag, S32 cookie);
    S64 (*AsyncEnd)(S64 id, MARK_TAG tag, S32 cookie);
    S64 (*Report)(void);
};

extern const Dev_Trace m_dev_trace;

#endif // __DEV_TRACE_H__
