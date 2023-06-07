/**
 * @file        dev_verified.h
 * @brief
 * @details
 * @author
 * @date        2017.07.17
 * @version     0.1
 * @note
 * @warning
 * @par         History:
 *
 */

#ifndef __DEV_VERIFIED_H__
#define __DEV_VERIFIED_H__

typedef struct __Dev_Verified Dev_Verified;
struct __Dev_Verified {
    U16 (*Crc16)(U8 *buf, U32 size, U16 p_crc);
    U32 (*Crc32)(U8 *buf, U32 size, U32 p_crc);
};

extern const Dev_Verified m_dev_verified;

#endif //__DEV_VERIFIED_H__

