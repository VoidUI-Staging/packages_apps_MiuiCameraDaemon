/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef __MICAMUTILS__
#define __MICAMUTILS__

#include <string>

namespace midebug {

#if defined(__GNUC__)
#define MICAM_VISIBILITY_PUBLIC __attribute__((visibility("default")))
#define MICAM_VISIBILITY_LOCAL  __attribute__((visibility("hidden")))
#else
#define MICAM_VISIBILITY_PUBLIC __declspec(dllexport)
#define MICAM_VISIBILITY_LOCAL
#endif // defined(__GNUC__)

#define MICAM_UNREFERENCED_PARAM(param) (void)param

#define MICAM_ASSERT_EX(type, fmt, ...) void(0)

#define MICAM_ASSERT(condition)                                  \
    do {                                                         \
        if (!(condition)) {                                      \
            MICAM_ASSERT_EX(MiCamAssertConditional, #condition); \
        }                                                        \
    } while ((void)0, 0)

#define MICAM_ASSERT_MESSAGE(condition, fmt, ...)                          \
    do {                                                                   \
        if (!(condition)) {                                                \
            MICAM_ASSERT_EX(MiCamAssertConditional, (fmt), ##__VA_ARGS__); \
        }                                                                  \
    } while ((void)0, 0)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Utils
///
/// @brief General utility class implementation
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class MICAM_VISIBILITY_PUBLIC MCamxUtils
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// SNPrintF
    ///
    /// @brief Write formatted data from variable argument list to sized buffer
    ///
    /// @param pDst     Destination buffer
    /// @param sizeDst  Size of the destination buffer
    /// @param pFormat  Format string, printf style
    /// @param ...      Parameters required by format
    ///
    /// @return Number of characters written
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static MICAM_VISIBILITY_PUBLIC inline int SNPrintF(char *dst, size_t sizeDst,
                                                       const char *format, ...)
    {
        int numCharWritten;
        va_list args;

        va_start(args, format);
        numCharWritten = VSNPrintF(dst, sizeDst, format, args);
        va_end(args);

        return numCharWritten;
    }

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// VSNPrintF
    ///
    /// @brief  Write formatted data from variable argument list to sized buffer
    ///
    /// @param  pDst     Destination buffer
    /// @param  sizeDst  Size of the destination buffer
    /// @param  pFormat  Format string, printf style
    /// @param  argptr   Parameters required by format
    ///
    /// @return Number of characters written
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    static int MICAM_VISIBILITY_PUBLIC inline VSNPrintF(char *dst, size_t sizeDst,
                                                        const char *format, va_list argPtr)
    {
        int numCharWritten = 0;
        numCharWritten = vsnprintf(dst, sizeDst, format, argPtr);
        MICAM_ASSERT(numCharWritten >= 0);
        if ((numCharWritten >= static_cast<int>(sizeDst)) && (sizeDst > 0)) {
            // Message length exceeds the buffer limit size
            MICAM_ASSERT(0);
            numCharWritten = -1;
        }

        return numCharWritten;
    }
};

} // namespace midebug

#endif //__MICAMUTILS__
