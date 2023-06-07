/** @file
 * @brief  Basic data type definition
 * @author Masaki HIRAGA (garhyla@morphoinc.com)
 * @date   2008-09-06
 *
 * Copyright (C) 2008 Morpho, Inc.
 */

#ifndef MORPHO_PRIMITIVE_H
#define MORPHO_PRIMITIVE_H

#ifdef __cplusplus
extern "C" {
#endif

typedef signed char morpho_Int8;          /**<  8 bits singed   integer */
typedef unsigned char morpho_Uint8;       /**<  8 bits unsinged integer */
typedef signed short morpho_Int16;        /**< 16 bits singed   integer */
typedef unsigned short morpho_Uint16;     /**< 16 bits unsinged integer */
typedef signed int morpho_Int32;          /**< 32 bits singed   integer */
typedef unsigned int morpho_Uint32;       /**< 32 bits unsinged integer */
typedef signed long long morpho_Int64;    /**< 64 bits singed   integer */
typedef unsigned long long morpho_Uint64; /**< 64 bits unsinged integer */
typedef float morpho_Float32;
typedef double morpho_Float64;

#ifdef __IPHONE__
typedef unsigned short morpho_Char;
#define MORPHO_LCHAR(S) L##S
#define MORPHO_LSTR(S)  L##S

#elif defined(__SYMBIAN32__)
typedef unsigned short morpho_Char;
#define MORPHO_LCHAR(S) L##S
#define MORPHO_LSTR(S)  L##S

#elif defined(__BREW_SYS__)
typedef char morpho_Char;
#define MORPHO_LCHAR(S) S
#define MORPHO_LSTR(S)  S

#elif defined(_WIN32)
typedef char morpho_Char;
#define MORPHO_LCHAR(S) S
#define MORPHO_LSTR(S)  S

#else
typedef char morpho_Char;
#define MORPHO_LCHAR(S) S
#define MORPHO_LSTR(S)  S
#endif

typedef morpho_Uint32 morpho_Bool;

#define MORPHO_TRUE  (1)
#define MORPHO_FALSE (0)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* #ifndef MORPHO_PRIMITIVE_H */
