/**
 * @file        settings.h
 * @brief
 * @details
 * @author
 * @date        2021.06.16
 * @version     0.1
 * @note
 * @warning
 * @par         History:
 *
 */
#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#include "dev_settings.h"

#if defined(WINDOWS_OS)
#define SETTINGS_FILE_C1    "resources\\camxoverridesettings.txt"
#elif defined(LINUX_OS)
#define SETTINGS_FILE_C1    "resources/camxoverridesettings.txt"
#elif defined(ANDROID_OS)
#define SETTINGS_FILE_C1    "/odm/etc/camera/camxoverridesettings.txt"
#endif

ADD_SETTINGS_START
    ADD_SETTINGS(char,SDK_PROJECT_NAME, "BOKEH_CAPTURE_HDR");
    ADD_SETTINGS(char,SDK_PROJECT_VERSION, "0.1");
    ADD_SETTINGS(bool,SDK_PROJECT_RELEASE, false);
    ADD_SETTINGS(int, SDK_PROJECT_TEST, 0x8992);
    ADD_SETTINGS(bool,SDK_PTHREAD_SUPPORT, TRUE);
    /************************************************************/
    ADD_SETTINGS(int,SDK_LOG_MASK, 0xFF);
    ADD_SETTINGS(int,isFakeSatSupported, 0);
    ADD_SETTINGS(float,luxIndexThreashouldForFlicker, 93.2);
    ADD_SETTINGS(char,multiCameraLogicalXMLFile, "test.xml");
    ADD_SETTINGS(bool,HDRSupportedFlashHdr,false);
ADD_SETTINGS_END

#endif    //__SETTINGS_H__
