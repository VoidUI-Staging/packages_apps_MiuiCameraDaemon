/**
 * @file		config.h
 * @brief
 * @details
 * @author
 * @date        2019.07.03
 * @version		0.1
 * @note
 * @warning
 * @par
 *
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#define PROJECT_NAME    "BASS"
#define PROJECT_VERSION 00000001
#define PROJECT_RELEASE FALSE

#if (PROJECT_RELEASE == TRUE)
#define DEBUG_TEST_CODE        (FALSE)
#define CONFIG_PTHREAD_SUPPORT (TRUE)

#else
#define DEBUG_TEST_CODE        (TRUE)
#define CONFIG_PTHREAD_SUPPORT (TRUE)
#endif

#endif // __CONFIG_H__
