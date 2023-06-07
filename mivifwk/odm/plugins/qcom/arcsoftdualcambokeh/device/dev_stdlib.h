/**
 * @file        Dev_stdlib.h
 * @brief       this file provides some Standard library APIs
 * @details
 * @author      LZL
 * @date        2016.05.17
 * @version     0.1
 * @note
 * @warning
 * @par         History:
 *
 */

#ifndef __DEV_STDLIB_H__
#define __DEV_STDLIB_H__
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "dev_type.h"

//extern s32 (*sprintf)(char *, const char *, ...);
//extern s32 (*snprintf)(char *, u32, const char *, ...);
//extern s32 (*sscanf)(const char*, const char*, ...);

int Dev_xtoa(const U8 *in, char *out, int len);
U8 Dev_xstoi(U8* str, U8* val);
void Dev_atox(const char * s, U8 * hex, U32 * len);
char* Dev_strtok_r(char *s, const char *delim, char **save_ptr);
#define Dev_atoi        atoi
#define Dev_atof        atof
#define Dev_strtol      strtol
#define Dev_memset      memset
#define Dev_memcpy      memcpy
#define Dev_memcmp      memcmp
#define Dev_memmove     memmove
#define Dev_strcpy      strcpy
#define Dev_strncpy     strncpy
#define Dev_strcat      strcat
#define Dev_strncat     strncat
#define Dev_strcmp      strcmp
#define Dev_strncmp     strncmp
#define Dev_strchr      strchr
#define Dev_strlen      strlen
#define Dev_strstr      strstr
#define Dev_toupper     toupper
#define Dev_tolower     tolower
#define Dev_isdigit     isdigit
#define Dev_sprintf     sprintf
#define Dev_snprintf    snprintf
#define Dev_sscanf      sscanf
#define Dev_vsprintf    vsprintf
#define Dev_printf      printf
#define Dev_malloc      malloc
#define Dev_free        free

#endif  // __DEV_STDLIB_H__
