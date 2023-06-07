/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly
 * prohibited.
 */
/* MediaTek Inc. (C) 2021. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY
 * ACKNOWLEDGES THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY
 * THIRD PARTY ALL PROPER LICENSES CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK
 * SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO
 * RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN
 * FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER
 * WILL BE, AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT
 * ISSUE, OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER
 * TO MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 *
 * The following software/firmware and/or related documentation ("MediaTek
 * Software") have been modified by MediaTek Inc. All revisions are subject to
 * any receiver's applicable license agreements with MediaTek Inc.
 */

#ifndef MIZONETYPES_H_
#define MIZONETYPES_H_

#include <cutils/native_handle.h>
#include <mtkcam-halif/device/1.x/types.h>
#include <mtkcam-halif/utils/metadata/1.x/IMetadata.h>
#include <system/camera_metadata.h>
#include <system/graphics-base-v1.0.h>
#include <system/graphics-base-v1.1.h>
#include <system/graphics-base-v1.2.h>

#include <cmath>
#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "ImageBufferInfo.h"
#include "MiMetadata.h"
//*****************************************************************************
//*
//*****************************************************************************

using NSCam::IMetadata;

namespace mizone {

static const int NO_STREAM = -1;

enum class StreamType : uint32_t {
    OUTPUT = 0,
    INPUT = 1,
};

enum StreamRotation : uint32_t {
    ROTATION_0 = 0,
    ROTATION_90 = 1,
    ROTATION_180 = 2,
    ROTATION_270 = 3,
};

enum class StreamConfigurationMode : uint32_t {
    NORMAL_MODE = 0,
    CONSTRAINED_HIGH_SPEED_MODE = 1,
    VENDOR_MODE_0 = 0x8000,
    VENDOR_MODE_1,
    VENDOR_MODE_2,
    VENDOR_MODE_3,
    VENDOR_MODE_4,
    VENDOR_MODE_5,
    VENDOR_MODE_6,
    VENDOR_MODE_7,
};

enum EImageFormat {
    ////HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED
    eImgFmt_IMPLEMENTATION_DEFINED = 34,

    ////RAW 16-bit 1 plane
    eImgFmt_RAW16 = 32,

    ////RAW 1 plane
    eImgFmt_RAW_OPAQUE = 36,

    ////RAW 10-bit 1 plane
    eImgFmt_RAW10 = 37,

    // This format is used to carry task-specific data which does not have a
    // standard image structure. The details of the format are left to the two
    // endpoints.

    // Buffers of this format must have a height of 1, and width equal to their
    // size in bytes.

    // @note Equals to AOSP `HAL_PIXEL_FORMAT_BLOB`

    eImgFmt_BLOB = 33,

    ////HAL_PIXEL_FORMAT_YCrCb_420_888
    eImgFmt_YCBCR_420_888 = 35,

    ////HAL_PIXEL_FORMAT_RGBA_8888 (32-bit; LSB:R, MSB:A), 1 plane
    eImgFmt_RGBA8888 = 1,

    ////HAL_PIXEL_FORMAT_RGBX_8888 (32-bit; LSB:R, MSB:X), 1 plane
    eImgFmt_RGBX8888 = 2,

    ////HAL_PIXEL_FORMAT_RGB_888   (24-bit), 1 plane (RGB)
    eImgFmt_RGB888 = 3,

    ////HAL_PIXEL_FORMAT_RGB_565   (16-bit), 1 plane
    eImgFmt_RGB565 = 4,

    ////HAL_PIXEL_FORMAT_BGRA_8888 (32-bit; LSB:B, MSB:A), 1 plane
    eImgFmt_BGRA8888 = 5,

    ////HAL_PIXEL_FORMAT_YCbCr_422_I 422 format, 1 plane (YUYV)
    eImgFmt_YUY2 = 20,

    ////HAL_PIXEL_FORMAT_YCbCr_422_SP 422 format, 2 plane (Y),(UV)
    eImgFmt_NV16 = 16,

    ////HAL_PIXEL_FORMAT_YCrCb_420_SP 420 format, 2 plane (Y),(VU)
    eImgFmt_NV21 = 17,

    //*HAL_PIXEL_FORMAT_NV12 420 format, 2 plane (Y),(UV)
    eImgFmt_NV12 = 0x00001000,

    ////HAL_PIXEL_FORMAT_YV12 420 format, 3 plane (Y),(V),(U)
    eImgFmt_YV12 = 842094169,

    ////HAL_PIXEL_FORMAT_Y8 8-bit Y plane
    eImgFmt_Y8 = 538982489,

    ////@deprecated Replace it with eImgFmt_Y8
    eImgFmt_Y800 = eImgFmt_Y8,

    ////HAL_PIXEL_FORMAT_Y16 16-bit Y plane
    eImgFmt_Y16 = 540422489,

    ////HAL_PIXEL_FORMAT_CAMERA_OPAQUE Opaque format, RAW10 + Metadata
    eImgFmt_CAMERA_OPAQUE = 0x00000111,

    ////HAL_PIXEL_FORMAT_YCBCR_P010 420 format, 16bit, 2 plane (Y),(UV) = P010
    eImgFmt_YUV_P010 = 54,

    //
    //  0x2000 - 0x2FFF
    //
    //  This range is reserved for pixel formats that are specific to the HAL
    //  implementation.
    //
    ////Unknown
    eImgFmt_UNKNOWN = 0x0000,

    ////Mediatek definition start hint enumeration, no used
    eImgFmt_VENDOR_DEFINED_START = 0x2000,

    ////Indicate to YUV start enumeration hint enumeration, no used
    eImgFmt_YUV_START = eImgFmt_VENDOR_DEFINED_START,

    ////Standard 422 format, 1 plane (YVYU)
    eImgFmt_YVYU = eImgFmt_YUV_START,

    ////Standard 422 format, 1 plane (UYVY)
    eImgFmt_UYVY,

    ////Standard 422 format, 1 plane (VYUY)
    eImgFmt_VYUY,

    ////Standard 422 format, 2 plane (Y),(VU)
    eImgFmt_NV61,

    ////420 format block mode, 2 plane (Y),(UV)
    eImgFmt_NV12_BLK,

    ////420 format block mode, 2 plane (Y),(VU)
    eImgFmt_NV21_BLK,

    ////422 format, 3 plane (Y),(V),(U)
    eImgFmt_YV16,

    ////420 format, 3 plane (Y),(U),(V)
    eImgFmt_I420,

    ////422 format, 3 plane (Y),(U),(V)
    eImgFmt_I422,

    ////422 format, 10 bits data stored in 16 bits, 1 plane (YUYV) = Y210
    eImgFmt_YUYV_Y210,

    ////422 format, 10 bits data stored in 16 bits, 1 plane (YVYU)
    eImgFmt_YVYU_Y210,

    ////422 format, 10 bits data stored in 16 bits, 1 plane (UYVY)
    eImgFmt_UYVY_Y210,

    ////422 format, 10 bits data stored in 16 bits, 1 plane (VYUY)
    eImgFmt_VYUY_Y210,

    ////422 format, 10 bits data stored in 16 bits, 2 plane (Y),(UV) = P210
    eImgFmt_YUV_P210,

    ////422 format, 10 bits data stored in 16 bits, 2 plane (Y),(VU)
    eImgFmt_YVU_P210,

    ////422 format, 10 bits data stored in 16 bits, 3 plane (Y),(U),(V)
    eImgFmt_YUV_P210_3PLANE,

    ////420 format, 10 bits data stored in 16 bits, 2 plane (Y),(VU)
    eImgFmt_YVU_P010,

    ////420 format, 10 bits data stored in 16 bits, 3 plane (Y),(U),(V)
    eImgFmt_YUV_P010_3PLANE,

    //*
    // Mediatek packed 10-bit 422 YUV, 1 plane. Each channel was sampled by 10-bit
    // and packed together. E.g.:
    // @code
    //          <-- 10-bit --><-- 10-bit --><-- 10-bit --><-- 10-bit --><-->
    //  line 0: [     Y      ][     U      ][     Y      ][     V      ][  ]
    //  line 1: [     Y      ][     U      ][     Y      ][     V      ][  ]
    //          <---------------------- stride ---------------------------->
    //@endcode
    // The data accuracy is 10-bit, caller could easily truncate 2 lower bits
    // for standard 8-bit bayer (or using normalization).

    eImgFmt_MTK_YUYV_Y210,

    //*
    // Mediatek packed 10-bit 422 YUV, 1 plane. Different YUV arrangement
    // between `eImgFmt_MTK_YUYV_Y210`.
    // @sa eImgFmt_MTK_YUYV_Y210

    eImgFmt_MTK_YVYU_Y210,

    ////@copydoc eImgFmt_MTK_YVYU_Y210
    eImgFmt_MTK_UYVY_Y210,

    ////@copydoc eImgFmt_MTK_YVYU_Y210
    eImgFmt_MTK_VYUY_Y210,

    //
    // Mediatek packed 10-bit 422 YUV, 2 planes. Each channel was sampled by
    // 10-bit and packed together. E.g.:
    // @code
    //  Y plane:
    //          <-- 10-bit --><-- 10-bit --><-- 10-bit --><-- 10-bit --><-->
    //  line 0: [     Y      ][     Y      ][     Y      ][     Y      ][  ]
    //  line 1: [     Y      ][     Y      ][     Y      ][     Y      ][  ]
    //          <---------------------- stride ---------------------------->

    //  UV plane:
    //          <-- 10-bit --><-- 10-bit --><-- 10-bit --><-- 10-bit --><-->
    //  line 0: [     U      ][     V      ][     U      ][     V      ][  ]
    //  line 1: [     U      ][     V      ][     U      ][     V      ][  ]
    //          <---------------------- stride ---------------------------->
    // @endcode
    // The data accuracy is 10-bit, caller could easily truncate 2 lower bits
    // for standard 8-bit bayer (or using normalization).

    eImgFmt_MTK_YUV_P210,

    //*
    // Mediatek packed 10-bit 422 YUV, 2 plane. Different YUV arrangement
    // between `eImgFmt_MTK_YUV_P210`. (Y), (VU)
    // @sa eImgFmt_MTK_YUV_P210

    eImgFmt_MTK_YVU_P210,

    //*
    // Mediatek packed 10-bit 422 YUV, 3 planes. Each channel was sampled by
    // 10-bit and packed together. E.g.:
    // @code
    //  Y plane:
    //          <-- 10-bit --><-- 10-bit --><-- 10-bit --><-- 10-bit --><-->
    //  line 0: [     Y      ][     Y      ][     Y      ][     Y      ][  ]
    //  line 1: [     Y      ][     Y      ][     Y      ][     Y      ][  ]
    //          <---------------------- stride ---------------------------->

    //  U plane:
    //          <-- 10-bit --><-- 10-bit --><-- 10-bit --><-- 10-bit --><-->
    //  line 0: [     U      ][     U      ][     U      ][     U      ][  ]
    //  line 1: [     U      ][     U      ][     U      ][     U      ][  ]
    //          <---------------------- stride ---------------------------->

    //  V plane:
    //          <-- 10-bit --><-- 10-bit --><-- 10-bit --><-- 10-bit --><-->
    //  line 0: [     V      ][     V      ][     V      ][     V      ][  ]
    //  line 1: [     V      ][     V      ][     V      ][     V      ][  ]
    //          <---------------------- stride ---------------------------->
    // @endcode
    // The data accuracy is 10-bit, caller could easily truncate 2 lower bits
    // for standard 8-bit bayer (or using normalization).

    eImgFmt_MTK_YUV_P210_3PLANE,

    //*
    // Mediatek packed 10-bit 420 YUV, 2-plane. (Y), (UV).
    // @sa eImgFmt_MTK_YUV_P210

    eImgFmt_MTK_YUV_P010,

    //*
    // Mediatek packed 10-bit 420 YUV, 2-plane. (Y), (VU).
    // @sa eImgFmt_MTK_YUV_P210

    eImgFmt_MTK_YVU_P010,

    //*
    // Mediatek packed 10-bit 420 YUV, 3-plane. (Y), (U), (V).
    // @sa eImgFmt_MTK_YUV_P210_3PLANE

    eImgFmt_MTK_YUV_P010_3PLANE,

    //*
    // Standard YUV 420 format, 12 bits data stored in 16 bits,
    // 2 plane (Y), (UV)

    eImgFmt_YUV_P012,

    //*
    // Standard YUV 420 format, 12 bits data stored in 16 bits,
    // 2 plane (Y), (VU)

    eImgFmt_YVU_P012,

    //*
    // Mediatek packed 12-bit 420 YUV, 2-plane. (Y), (UV).
    // @sa eImgFmt_MTK_YUV_P210

    eImgFmt_MTK_YUV_P012,

    //*
    // Mediatek packed 12-bit 420 YUV, 2-plane. (Y), (VU).
    // @sa eImgFmt_MTK_YUV_P210

    eImgFmt_MTK_YVU_P012,

    ////RGB format start hint enumeration, no used
    eImgFmt_RGB_START = 0x2100, // please add RGB format after this enum

    ////ARGB (32-bit; LSB:A, MSB:B), 1 plane
    eImgFmt_ARGB8888 = eImgFmt_RGB_START,

    ////@deprecated Replace it with eImgFmt_ARGB8888
    eImgFmt_ARGB888 = eImgFmt_ARGB8888,

    ////RGB 48(16x3, 48-bit; LSB:R, MSB:B), 1 plane
    eImgFmt_RGB48,

    ////RGB, unpacked 8-bit, 3 plane
    eImgFmt_RGB8_UNPAK_3PLANE,

    ////RGB, unpacked 10-bit, 3 plane
    eImgFmt_RGB10_UNPAK_3PLANE,

    ////RGB, unpacked 12-bit, 3 plane
    eImgFmt_RGB12_UNPAK_3PLANE,

    ////RGB, packed 8-bit,  3 plane
    eImgFmt_RGB8_PAK_3PLANE,

    ////RGB, packed 10-bit, 3 plane
    eImgFmt_RGB10_PAK_3PLANE,

    ////RGB, packed 12-bit, 3 plane
    eImgFmt_RGB12_PAK_3PLANE,

    ////Mediatek bayer formats start hint enumeration, no used
    eImgFmt_RAW_START = 0x2200, // add RAW format after this enum

    //*
    // Mediatek Bayer format, 8-bit. This data arrangement is the same as
    // MIPI 8-bit raw.

    eImgFmt_BAYER8 = eImgFmt_RAW_START,

    //*
    // Mediatek Bayer format, 10-bit. Each channel was sampled by 10-bit
    // and packed together. E.g.:
    // @code
    //          <-- 10-bit --><-- 10-bit --><-- 10-bit --><-- 10-bit --><-->
    //  line 0: [     R      ][     G      ][     R      ][     G      ][  ]
    //  line 1: [     G      ][     B      ][     G      ][     B      ][  ]
    //          <---------------------- stride ---------------------------->
    // @endcode
    // The data accuracy is 10-bit, caller could easily truncate 2 lower bits
    // for standard 8-bit bayer (or using normalization).

    eImgFmt_BAYER10,

    //*
    // Mediatek Bayer format, 12-bit. Each channel was sampled by 12-bit
    // and packed together. E.g.:
    // @code
    //          <-- 12-bit --><-- 12-bit --><-- 12-bit --><-- 12-bit --><-->
    //  line 0: [     R      ][     G      ][     R      ][     G      ][  ]
    //  line 1: [     G      ][     B      ][     G      ][     B      ][  ]
    //          <---------------------- stride ---------------------------->
    // @endcode
    // The data accuracy is 12-bit, caller could easily truncate 4 lower bits
    // for standard 8-bit bayer (or using normalization).

    eImgFmt_BAYER12,

    //*
    // Mediatek Bayer format, 14-bit. Each channel was sampled by 14-bit
    // and packed together. E.g.:
    // @code
    //          <-- 14-bit --><-- 14-bit --><-- 14-bit --><-- 14-bit --><-->
    //  line 0: [     R      ][     G      ][     R      ][     G      ][  ]
    //  line 1: [     G      ][     B      ][     G      ][     B      ][  ]
    //          <---------------------- stride ---------------------------->
    // @endcode
    // The data accuracy is 14-bit, caller could easily truncate 6 lower bits
    // for standard 8-bit bayer (or using normalization).

    eImgFmt_BAYER14,

    //*
    // Mediatek bayer format with double G channel data. Each channel was
    // sampled by 8-bit, 1 plane.
    // E.g.: The standard bayer order is `RG/GB`, the full-G bayer format would
    //      be `RGG/GBG`. Memory footprint is:
    // @code

    //  <- N-bit -><- N-bit -><- N-bit -><- N-bit -><- N-bit -><- N-bit -><-->
    //  [    R    ][    G    ][    G    ][    R    ][    G    ][    G    ][  ]
    //  [    G    ][    B    ][    G    ][    G    ][    B    ][    G    ][  ]
    //  <---------------------------- stride -------------------------------->

    //  where N = 8, N is data accuracy.

    // @endcode

    eImgFmt_FG_BAYER8,

    //*
    // Mediatek bayer format with double G channel data. Each channel was
    // sampled by 10-bit, 1 plane.
    // @sa eImgFmt_FG_BAYER8

    eImgFmt_FG_BAYER10,

    //*
    // Mediatek bayer format with double G channel data. Each channel was
    // sampled by 12-bit, 1 plane.
    // @sa eImgFmt_FG_BAYER8

    eImgFmt_FG_BAYER12,

    //*
    // Mediatek bayer format with double G channel data. Each channel was
    // sampled by 14-bit, 1 plane.
    // @sa eImgFmt_FG_BAYER8

    eImgFmt_FG_BAYER14,

    //*
    // Mediatek bayer format with full G channel data stored in plane 0,
    // and the channel R and B are stored weavely in plane 1.

    // @code
    //   plane 0

    //     line 0: [G][G][G][G][]
    //     line 1: [G][G][G][G][]
    //     line 2: [G][G][G][G][]
    //     line 3: [G][G][G][G][]
    //             <-- stride -->

    //   plane 1

    //     line 0: [R][B][R][B][]
    //     line 1: [R][B][R][B][]
    //             <-- stride -->
    // @endcode

    eImgFmt_FG_BAYER8_PAK_2PLANE,

    //*
    // Mediatek bayer format with full G channel data and separated into
    // 2 planes. Each channel was sampled by 10-bit.
    // @sa eImgFmt_FG_BAYER8_PAK_2PLANE

    eImgFmt_FG_BAYER10_PAK_2PLANE,

    //*
    // Mediatek bayer format with full G channel data and separated into
    // 2 planes. Each channel was sampled by 12-bit.
    // @sa eImgFmt_FG_BAYER8_PAK_2PLANE

    eImgFmt_FG_BAYER12_PAK_2PLANE,

    //*
    // Mediatek bayer format with full G channel data and stored in plane 0.
    // Each channel was sampled by 8-bit. Plane 1 stores channel R, and
    // plane 2 stores channel G.

    // @code
    //   plane 0

    //     line 0: [G][G][G][G][]
    //     line 1: [G][G][G][G][]
    //     line 2: [G][G][G][G][]
    //     line 3: [G][G][G][G][]
    //             <-- stride -->

    //   plane 1

    //     line 0: [R][R][]
    //     line 1: [R][R][]
    //             <--+--->
    //                stride

    //   plane 2

    //     line 0: [B][B][]
    //     line 1: [B][B][]
    //             <--+--->
    //                stride

    // @endcode

    eImgFmt_FG_BAYER8_PAK_3PLANE,

    //*
    // Mediatek bayer format with full G channel data and separeted into 3
    // planes. Each channel was sampled by 10-bit.
    // @sa eImgFmt_FG_BAYER8_PAK_3PLANE

    eImgFmt_FG_BAYER10_PAK_3PLANE,

    //*
    // Mediatek bayer format with full G channel data and separeted into 3
    // planes. Each channel was sampled by 12-bit.
    // @sa eImgFmt_FG_BAYER8_PAK_3PLANE

    eImgFmt_FG_BAYER12_PAK_3PLANE,

    //*
    // Mediatek bayer format with full G channel data and separeted into 3
    // planes. Each channel was sampled by 8-bit stored in 2 bytes.
    // @sa eImgFmt_FG_BAYER8_PAK_3PLANE

    eImgFmt_FG_BAYER8_UNPAK_3PLANE,

    //*
    // Mediatek bayer format with full G channel data and separeted into 3
    // planes. Each channel was sampled by 10-bit stored in 2 bytes.
    // The data accuracy is 10-bit, caller could easily truncate 2 lower bits
    // for standard 8-bit bayer (or using normalization)
    // @sa eImgFmt_FG_BAYER8_PAK_3PLANE

    eImgFmt_FG_BAYER10_UNPAK_3PLANE,

    //*
    // Mediatek bayer format with full G channel data and separeted into 3
    // planes. Each channel was sampled by 12-bit stored in 2 bytes.
    // The data accuracy is 12-bit, caller could easily truncate 4 lower bits
    // for standard 8-bit bayer (or using normalization)
    // @sa eImgFmt_FG_BAYER8_PAK_3PLANE

    eImgFmt_FG_BAYER12_UNPAK_3PLANE,

    ////Bayer format, 10-bit (MIPI)
    eImgFmt_BAYER10_MIPI,

    ////Bayer format, 8-bit data unpacked as 16-bit
    eImgFmt_BAYER8_UNPAK,

    ////Bayer format, 10-bit data unpacked as 16-bit
    eImgFmt_BAYER10_UNPAK,

    ////Bayer format, 12-bit data unpacked as 16-bit
    eImgFmt_BAYER12_UNPAK,

    ////Bayer format, 14-bit data unpacked as 16-bit
    eImgFmt_BAYER14_UNPAK,

    ////Bayer format, 15-bit data unpacked as 16-bit
    eImgFmt_BAYER15_UNPAK,

    ////Bayer format, 16-bit data
    eImgFmt_BAYER16_UNPAK,

    ////Bayer format, 22-bit data in 24-bit fields
    eImgFmt_BAYER22_PAK,

    ////Warp format, 32-bit, 1 plane (X)
    eImgFmt_WARP_1PLANE,

    ////Warp format, 32-bit, 2 plane (X), (Y)
    eImgFmt_WARP_2PLANE,

    ////Warp format, 32-bit, 3 plane (X), (Y), (Z)
    eImgFmt_WARP_3PLANE,

    ////Processed bayer format applied Mediatek lens shading, 16-bit, 0x2217
    eImgFmt_BAYER16_APPLY_LSC = eImgFmt_BAYER16_UNPAK,

    ////Blob format starts hint enumeration, no used
    eImgFmt_BLOB_START = 0x2300, // please add BLOB format after this enum

    ////JPEG format
    eImgFmt_JPEG = eImgFmt_BLOB_START,

    ////JPEG 420 format, 3 plane (Y),(U),(V)
    eImgFmt_JPG_I420,

    ////JPEG 422 format, 3 plane (Y),(U),(V)
    eImgFmt_JPG_I422,

    ////isp tuning buffer for ISP HIDL
    eImgFmt_ISP_HIDL_TUNING,

    ////isp tuning buffer from ISP HAL
    eImgFmt_ISP_TUNING,

    ////Android Q HEIC for exif stream format
    eImgFmt_JPEG_APP_SEGMENT,

    ////ISP HIDL HEIF
    eImgFmt_HEIF,

    ////This section is used for non-image-format buffer
    eImgFmt_STA_START = 0x2400, // please add STATIC format after this enum

    ////statistic format, 8-bit
    eImgFmt_STA_BYTE = eImgFmt_STA_START,

    ////statistic format, 16-bit
    eImgFmt_STA_2BYTE,

    ////statistic format, 32-bit
    eImgFmt_STA_4BYTE,

    ////statistic format, 10-bit
    eImgFmt_STA_10BIT,

    ////statistic format, 12-bit
    eImgFmt_STA_12BIT,

    ////compression data format start hint enumeration, no used
    eImgFmt_COMPRESSION_START = 0x2500, // please add compression data after this

    ////UFBC Start enumeration value
    eImgFmt_UFBC_START = eImgFmt_COMPRESSION_START,

    //*
    // Mediatek compression format for 8-bit bayer.
    // @sa struct UfbcBufferHeader

    eImgFmt_UFBC_BAYER8 = eImgFmt_UFBC_START,

    //*
    // Mediatek compression format for 10-bit bayer.
    // @sa struct UfbcBufferHeader

    eImgFmt_UFBC_BAYER10,

    //*
    // Mediatek compression format for 12-bit bayer.
    // @sa struct UfbcBufferHeader

    eImgFmt_UFBC_BAYER12,

    //*
    // Mediatek compression format for 14-bit bayer.
    // @sa struct UfbcBufferHeader

    eImgFmt_UFBC_BAYER14,

    ////UFBC YUV 2-plane start
    eImgFmt_UFBC_YUV_2P_START,

    //*
    // Mediatek compression format for 8-bit YUV 420 2 planes, (Y),(UV)
    // @sa struct UfbcBufferHeader

    eImgFmt_UFBC_NV12 = eImgFmt_UFBC_YUV_2P_START,

    //*
    // Mediatek compression format for 8-bit YUV 420 2 planes, (Y),(VU)
    // @sa struct UfbcBufferHeader

    eImgFmt_UFBC_NV21,

    //*
    // Mediatek compression format for 10-bit packed YUV 420 2 planes, (Y),(UV)
    // @sa struct UfbcBufferHeader

    eImgFmt_UFBC_YUV_P010,

    //*
    // Mediatek compression format for 10-bit packed YUV 420 2 planes, (Y),(VU)
    // @sa struct UfbcBufferHeader

    eImgFmt_UFBC_YVU_P010,

    //*
    // Mediatek compression format for 12-bit YUV 420 2 planes, (Y),(UV)
    // @sa struct UfbcBufferHeader

    eImgFmt_UFBC_YUV_P012,

    //*
    // Mediatek compression format for 12-bit YUV 420 2 planes, (Y),(VU)
    // @sa struct UfbcBufferHeader

    eImgFmt_UFBC_YVU_P012,

    ////UFBC YUV 2-plane end enumeration, no use
    eImgFmt_UFBC_YUV_2P_END,

    ////UFBC end hint enumeration, no use
    eImgFmt_UFBC_END = eImgFmt_UFBC_YUV_2P_END,

    ////ARM Frame Buffer Compression format start hint, no use
    eImgFmt_AFBC_START,

    ////AFBC 420 format, 2 plane (Y),(UV)
    eImgFmt_AFBC_NV12 = eImgFmt_AFBC_START,

    ////AFBC 420 format, 2 plane (Y),(VU)
    eImgFmt_AFBC_NV21,

    ////AFBC 420 format, 10bit, 2 plane (Y),(UV) = P010
    eImgFmt_AFBC_MTK_YUV_P010,

    ////AFBC 420 format, 10bit, 2 plane (Y),(VU)
    eImgFmt_AFBC_MTK_YVU_P010,

    ////AFBC RGBA (32-bit; LSB:R, MSB:A), 1 plane
    eImgFmt_AFBC_RGBA8888,

    ////ARM Frame Buffer Compression format end
    eImfFmt_AFBC_END,

    ////Compression data end
    eImgFmt_COMPRESSION_END = eImfFmt_AFBC_END,
};

//*
// Image Buffer Usage.

enum BufferUsageEnum {
    //*
    // Buffer is never read in software

    eBUFFER_USAGE_SW_READ_NEVER = 0x00000000U,

    //*
    // Buffer is rarely read in software

    eBUFFER_USAGE_SW_READ_RARELY = 0x00000002U,

    //*
    // Buffer is often read in software

    eBUFFER_USAGE_SW_READ_OFTEN = 0x00000003U,

    //*
    // Mask for the software read values

    eBUFFER_USAGE_SW_READ_MASK = 0x0000000FU,

    //*
    // Buffer is never written in software

    eBUFFER_USAGE_SW_WRITE_NEVER = 0x00000000U,

    //*
    // Buffer is rarely written in software

    eBUFFER_USAGE_SW_WRITE_RARELY = 0x00000020U,

    //*
    // Buffer is often written in software

    eBUFFER_USAGE_SW_WRITE_OFTEN = 0x00000030U,

    //*
    // Mask for the software write values

    eBUFFER_USAGE_SW_WRITE_MASK = 0x000000F0U,

    //*
    // Mask for the software access

    eBUFFER_USAGE_SW_MASK = eBUFFER_USAGE_SW_READ_MASK | eBUFFER_USAGE_SW_WRITE_MASK,

    //*
    // Buffer will be used as an OpenGL ES texture (read by GPU).
    // @note This flag causes gralloc buffer.

    eBUFFER_USAGE_HW_TEXTURE = 0x00000100U,

    //*
    // Buffer will be used as an OpenGL ES render target (written by GPU).
    // @note This flag causes gralloc buffer.

    eBUFFER_USAGE_HW_RENDER = 0x00000200U,

    //*
    // Buffer will be used by the HWComposer HAL module.
    // @note This flag causes gralloc buffer.

    eBUFFER_USAGE_HW_COMPOSER = 0x00000800U,

    //*
    // Buffer will be used with the HW video encoder.
    // @note This flag causes gralloc buffer.

    eBUFFER_USAGE_HW_VIDEO_ENCODER = 0x00010000U,

    //*
    // Buffer will be written by the HW camera pipeline.

    eBUFFER_USAGE_HW_CAMERA_WRITE = 0x00020000U,

    //*
    // Buffer will be read by the HW camera pipeline.

    eBUFFER_USAGE_HW_CAMERA_READ = 0x00040000U,

    //*
    // Mask for the camera access values.

    eBUFFER_USAGE_HW_CAMERA_MASK = 0x00060000U,

    //*
    // Buffer will be read and written by the HW camera pipeline.

    eBUFFER_USAGE_HW_CAMERA_READWRITE =
        eBUFFER_USAGE_HW_CAMERA_WRITE | eBUFFER_USAGE_HW_CAMERA_READ,

    //*
    // Mask for all hardware access values.
    // @note This flag causes gralloc buffer.

    eBUFFER_USAGE_HW_MASK = 0x00071F00U,
};

enum class Dataspace : int32_t {
    // V1_0
    UNKNOWN = 0x0,
    ARBITRARY = 0x1,

    STANDARD_SHIFT = 16,
    STANDARD_MASK = 63 << STANDARD_SHIFT, // 0x3F
    STANDARD_UNSPECIFIED = 0 << STANDARD_SHIFT,
    STANDARD_BT709 = 1 << STANDARD_SHIFT,
    STANDARD_BT601_625 = 2 << STANDARD_SHIFT,
    STANDARD_BT601_625_UNADJUSTED = 3 << STANDARD_SHIFT,
    STANDARD_BT601_525 = 4 << STANDARD_SHIFT,
    STANDARD_BT601_525_UNADJUSTED = 5 << STANDARD_SHIFT,
    STANDARD_BT2020 = 6 << STANDARD_SHIFT,
    STANDARD_BT2020_CONSTANT_LUMINANCE = 7 << STANDARD_SHIFT,
    STANDARD_BT470M = 8 << STANDARD_SHIFT,
    STANDARD_FILM = 9 << STANDARD_SHIFT,
    STANDARD_DCI_P3 = 10 << STANDARD_SHIFT,
    STANDARD_ADOBE_RGB = 11 << STANDARD_SHIFT,

    TRANSFER_SHIFT = 22,
    TRANSFER_MASK = 31 << TRANSFER_SHIFT, // 0x1F
    TRANSFER_UNSPECIFIED = 0 << TRANSFER_SHIFT,
    TRANSFER_LINEAR = 1 << TRANSFER_SHIFT,
    TRANSFER_SRGB = 2 << TRANSFER_SHIFT,
    TRANSFER_SMPTE_170M = 3 << TRANSFER_SHIFT,
    TRANSFER_GAMMA2_2 = 4 << TRANSFER_SHIFT,
    TRANSFER_GAMMA2_6 = 5 << TRANSFER_SHIFT,
    TRANSFER_GAMMA2_8 = 6 << TRANSFER_SHIFT,
    TRANSFER_ST2084 = 7 << TRANSFER_SHIFT,
    TRANSFER_HLG = 8 << TRANSFER_SHIFT,

    RANGE_SHIFT = 27,
    RANGE_MASK = 7 << RANGE_SHIFT, // 0x7
    RANGE_UNSPECIFIED = 0 << RANGE_SHIFT,
    RANGE_FULL = 1 << RANGE_SHIFT,
    RANGE_LIMITED = 2 << RANGE_SHIFT,
    RANGE_EXTENDED = 3 << RANGE_SHIFT,

    SRGB_LINEAR = 0x200, // deprecated, use V0_SRGB_LINEAR
    V0_SRGB_LINEAR = STANDARD_BT709 | TRANSFER_LINEAR | RANGE_FULL,
    SRGB = 0x201, // deprecated, use V0_SRGB
    V0_SRGB = STANDARD_BT709 | TRANSFER_SRGB | RANGE_FULL,
    V0_SCRGB = STANDARD_BT709 | TRANSFER_SRGB | RANGE_EXTENDED,
    JFIF = 0x101, // deprecated, use V0_JFIF
    V0_JFIF = STANDARD_BT601_625 | TRANSFER_SMPTE_170M | RANGE_FULL,
    BT601_625 = 0x102, // deprecated, use V0_BT601_625
    V0_BT601_625 = STANDARD_BT601_625 | TRANSFER_SMPTE_170M | RANGE_LIMITED,
    BT601_525 = 0x103, // deprecated, use V0_BT601_525
    V0_BT601_525 = STANDARD_BT601_525 | TRANSFER_SMPTE_170M | RANGE_LIMITED,
    BT709 = 0x104, // deprecated, use V0_BT709
    V0_BT709 = STANDARD_BT709 | TRANSFER_SMPTE_170M | RANGE_LIMITED,

    DCI_P3_LINEAR = STANDARD_DCI_P3 | TRANSFER_LINEAR | RANGE_FULL,
    DCI_P3 = STANDARD_DCI_P3 | TRANSFER_GAMMA2_6 | RANGE_FULL,
    DISPLAY_P3_LINEAR = STANDARD_DCI_P3 | TRANSFER_LINEAR | RANGE_FULL,
    DISPLAY_P3 = STANDARD_DCI_P3 | TRANSFER_SRGB | RANGE_FULL,
    ADOBE_RGB = STANDARD_ADOBE_RGB | TRANSFER_GAMMA2_2 | RANGE_FULL,
    BT2020_LINEAR = STANDARD_BT2020 | TRANSFER_LINEAR | RANGE_FULL,
    BT2020 = STANDARD_BT2020 | TRANSFER_SMPTE_170M | RANGE_FULL,
    BT2020_PQ = STANDARD_BT2020 | TRANSFER_ST2084 | RANGE_FULL,

    DEPTH = 0x1000,
    SENSOR = 0x1001,

    // V1_1
    DEPTH_16 = 0x30,
    DEPTH_24 = 0x31,
    DEPTH_24_STENCIL_8 = 0x32,
    DEPTH_32F = 0x33,
    DEPTH_32F_STENCIL_8 = 0x34,
    STENCIL_8 = 0x35,
    YCBCR_P010 = 0x36,

    // V1_2
    DISPLAY_BT2020 = STANDARD_BT2020 | TRANSFER_SRGB | RANGE_FULL,
    DYNAMIC_DEPTH = 0x1002,
    JPEG_APP_SEGMENTS = 0x1003,
    HEIF = 0x1004,
};

enum ETransform {
    ////Neither rotation nor flipping.
    eTransform_None = 0x00,

    ////Flip source image horizontally (around the vertical axis)
    eTransform_FLIP_H = 0x01,

    ////Flip source image vertically (around the horizontal axis)
    eTransform_FLIP_V = 0x02,

    ////Rotate source image 90 degrees clockwise
    eTransform_ROT_90 = 0x04,

    ////Rotate source image 180 degrees
    eTransform_ROT_180 = 0x03,

    ////Rotate source image 270 degrees clockwise
    eTransform_ROT_270 = 0x07,
};

struct Stream
{
    int32_t id = NO_STREAM;
    StreamType streamType = StreamType::OUTPUT;
    uint32_t width = 0;
    uint32_t height = 0;
    //见 BufferUsageEnum
    uint64_t usage = 0;
    int32_t physicalCameraId = -1; // V3_4
    uint32_t bufferSize = 0;       // V3_4

    //下面4个变量是mtk扩展或自己定义的。
    uint32_t transform = eTransform_None;
    EImageFormat format = eImgFmt_UNKNOWN;
    Dataspace dataSpace = Dataspace::UNKNOWN;
    MiMetadata settings;
    BufPlanes bufPlanes;
};

struct StreamConfiguration
{
    std::vector<Stream> streams;
    StreamConfigurationMode operationMode;
    MiMetadata sessionParams;
    uint32_t streamConfigCounter = 0; // V3_5
};

struct OfflineRequest
{
    uint32_t frameNumber = 0;
    std::vector<int32_t> pendingStreams;
};

struct OfflineStream
{
    int32_t id = NO_STREAM;
    uint32_t numOutstandingBuffers = 0;
    std::vector<uint64_t> circulatingBufferIds;
};

struct CameraOfflineSessionInfo
{
    std::vector<OfflineStream> offlineStreams;
    std::vector<OfflineRequest> offlineRequests;
};

struct HalStream
{
    int32_t id = NO_STREAM;
    EImageFormat overrideFormat = eImgFmt_UNKNOWN;
    uint64_t producerUsage = 0;
    uint64_t consumerUsage = 0;
    uint32_t maxBuffers = 1;
    Dataspace overrideDataSpace = Dataspace::UNKNOWN;
    int32_t physicalCameraId = -1; // V3_4
    bool supportOffline = false;
    MiMetadata results;
};

struct HalStreamConfiguration
{
    std::vector<HalStream> streams;
    MiMetadata sessionResults;
};

struct PhysicalCameraSetting
{
    int32_t physicalCameraId = -1;
    MiMetadata settings;
    MiMetadata halSettings;
};

enum class BufferStatus : uint32_t {
    OK = 0,
    ERROR = 1,
};

struct AppBufferHandleHolder
{
    buffer_handle_t bufferHandle = nullptr;
    bool freeByOthers = false;
    explicit AppBufferHandleHolder(buffer_handle_t);
    ~AppBufferHandleHolder();
};

class MiImageBuffer;
struct StreamBuffer
{
    int32_t streamId = NO_STREAM;
    uint64_t bufferId = 0; // SHOULD ONLY EXIST IN HIDL HAL
    std::shared_ptr<MiImageBuffer> buffer;
    BufferStatus status = BufferStatus::OK;
    int acquireFenceFd = -1;
    int releaseFenceFd = -1;
    MiMetadata bufferSettings;
};

enum class CameraBlobId : uint16_t {
    JPEG = 0x00FF,
    JPEG_APP_SEGMENTS = 0x100, // V3_5
    HEIF = 0x00FE,
};

struct CameraBlob
{
    CameraBlobId blobId;
    uint32_t blobSize;
};

enum class MsgType : uint32_t {

    UNKNOWN = 0,
    ERROR = 1,
    SHUTTER = 2,
};

enum class ErrorCode : uint32_t {
    ERROR_NONE = 0,
    ERROR_DEVICE = 1,
    ERROR_REQUEST = 2,
    ERROR_RESULT = 3,
    ERROR_BUFFER = 4,
};

struct ErrorMsg
{
    uint32_t frameNumber = 0;
    int32_t errorStreamId = NO_STREAM;
    ErrorCode errorCode = ErrorCode::ERROR_DEVICE;
};

struct ShutterMsg
{
    uint32_t frameNumber = 0;
    uint64_t timestamp = 0;
};

struct NotifyMsg
{
    MsgType type = MsgType::UNKNOWN;
    union Message {
        ErrorMsg error;
        ShutterMsg shutter;
    } msg;
};

enum RequestTemplate : uint32_t {
    PREVIEW = 1u,
    STILL_CAPTURE = 2u,
    VIDEO_RECORD = 3u,
    VIDEO_SNAPSHOT = 4u,
    ZERO_SHUTTER_LAG = 5u,
    MANUAL = 6u,
    VENDOR_TEMPLATE_START = 0x40000000u,
};
struct SubRequest
{
    uint32_t subFrameIndex = 1;
    std::vector<StreamBuffer> inputBuffers;                    // logical or physical sensor inputs
    MiMetadata settings;                                       // logical app ctrl
    MiMetadata halSettings;                                    // logical hal ctrl
    std::vector<PhysicalCameraSetting> physicalCameraSettings; // physical app & hal ctrl
};

struct CaptureRequest
{
    uint32_t frameNumber = 0;
    MiMetadata settings;
    MiMetadata halSettings;
    std::vector<StreamBuffer> inputBuffers;
    std::vector<StreamBuffer> outputBuffers;
    std::vector<PhysicalCameraSetting> physicalCameraSettings; // V3_4
    std::vector<SubRequest> subRequests;
};

struct PhysicalCameraMetadata
{
    int32_t physicalCameraId = -1;
    MiMetadata metadata;
    MiMetadata halMetadata;
};

struct CaptureResult
{
    uint32_t frameNumber = 0;
    std::vector<StreamBuffer> outputBuffers;
    std::vector<StreamBuffer> inputBuffers;
    bool bLastPartialResult;
    MiMetadata result;
    MiMetadata halResult;
    std::vector<PhysicalCameraMetadata> physicalCameraMetadata; // V3_4
};

struct BufferCache
{ // SHOULD ONLY EXIST IN HIDL HAL
    int32_t streamId;
    uint64_t bufferId;
};

enum class StreamBufferRequestError : uint32_t {
    NO_ERROR = 0,
    NO_BUFFER_AVAILABLE = 1,
    MAX_BUFFER_EXCEEDED = 2,
    STREAM_DISCONNECTED = 3,
    UNKNOWN_ERROR = 4,
};

struct StreamBuffersVal
{
    StreamBufferRequestError error = StreamBufferRequestError::UNKNOWN_ERROR;
    std::vector<StreamBuffer> buffers;
};

struct StreamBufferRet
{
    int32_t streamId = NO_STREAM;
    StreamBuffersVal val;
};

enum BufferRequestStatus : uint32_t {
    STATUS_OK = 0,
    FAILED_PARTIAL = 1,
    FAILED_CONFIGURING = 2,
    FAILED_ILLEGAL_ARGUMENTS = 3,
    FAILED_UNKNOWN = 4,
};

struct BufferRequest
{
    int32_t streamId = NO_STREAM;
    uint32_t numBuffersRequested = 0;
};

enum DeviceType : uint32_t {
    NORMAL = 0,
    POSTPROC = 1,
};

struct DeviceInfo
{
    int32_t instanceId = -1;
    int32_t virtualInstanceId = -1;
    bool hasFlashUnit = false;
    bool isHidden = false;
    int32_t facing = -1;
    DeviceType type = DeviceType::NORMAL;
};

// ExtCameraDeviceSession
struct ExtConfigurationResults
{
    // int status,
    // camera_metadata//results;
    std::map<int32_t, std::vector<int64_t>> streamResults;
};

// for PostProc;
struct PostProcPhysicalCameraSetting
{
    int32_t physicalCameraId = -1;
    IMetadata settings;
    IMetadata halSettings;
};

struct PostProcParams
{
    // settings
    IMetadata sessionParams;
    // frame
    PostProcPhysicalCameraSetting physicalCameraSettings;
    uint64_t bufferId;
    // request meta
    IMetadata settings;
    IMetadata halSettings;
    IMetadata bufferSettings;
    uint64_t usage;
    int32_t vndRequestNo;
};
struct MiPostProcData
{
    int32_t physicalCameraId = -1;
    PostProcParams *pPostProcParams;
};

struct MiSize
{
    uint32_t width;
    uint32_t height;
    bool isSameRatio(float ratio) const
    {
        auto double_round = [](float src, int bits) {
            std::stringstream stream;
            float result;
            stream << std::fixed << std::setprecision(bits) << src;
            stream >> result;
            return result;
        };
        float thisRatio = double_round(float(width) / height, 1);
        ratio = double_round(ratio, 1);
        return std::fabs(thisRatio - ratio) < std::numeric_limits<float>::epsilon();
    }
};

struct CameraInfo
{
    int32_t cameraId;
    int32_t roleId;
    std::vector<int32_t> physicalCameraIds;
    MiSize activeArraySize;
    MiMetadata staticInfo;
    std::map<int32_t, MiMetadata> physicalMetadataMap;
    bool supportOffline;
    bool supportCoreHBM;
    bool supportAndroidHBM;
};

enum class StreamUsageType {
    UnkownStream = -1,

    PreviewStream = 0,
    SnapshotStream,
    // for snapshot quickview
    QuickViewStream,
    // for preview QR
    QrStream,

    QuickViewCachedStream,

    TuningDataStream,
};

enum class StreamOwner {
    StreamOwnerFwk = 0,
    StreamOwnerVnd,
};

struct MiStream
{
    Stream rawStream;
    int32_t streamId;
    StreamUsageType usageType;
    StreamOwner owner;
};

struct MiConfig
{
    StreamConfiguration rawConfig;
    // <streamId, MiStream>
    std::map<int32_t, std::shared_ptr<MiStream>> streams;
};

struct MiRequest
{
    uint32_t frameNum;
    CaptureRequest vndRequest;
    MiMetadata result;
    MiMetadata halResult;
    std::map<int32_t, PhysicalCameraMetadata> physicalCameraMetadata;
    uint64_t shutter;
    uint32_t receivedBuffersCount = 0;
    bool isFullMetadata = false;
    bool isError = false;
    std::vector<std::shared_ptr<MiStream>> streams;
    std::vector<std::shared_ptr<MiPostProcData>> vPostProcData;
};

enum CameraRoleId {
    RoleIdRearWide = 0,
    RoleIdFront = 1,
    RoleIdRearTele = 20,
    RoleIdRearUltra = 21,
    RoleIdRearMacro = 22,
    RoleIdRearTele4x = 23,
    RoleIdRearMacro2x = 24,
    RoleIdRearDepth = 25,
    RoleIdFrontAux = 40,
    RoleIdRearSat = 60,
    RoleIdRearBokeh = 61,
    RoleIdRearSatVideo = 62,
    RoleIdRearBokeh2 = 63,
    RoleIdRearBokeh3 = 65,
    RoleIdFrontSat = 80,
    RoleIdFrontBokeh = 81,
    RoleIdRearVt = 100,
    RoleIdFrontVt = 101,
    RoleIdParallelVt = 102,
    RoleIdRearSatUWT = 120,
    RoleIdRearSatUWTT4x = 180,
};

// NOTE: could not use the status code value define in AOSP hidl directly, because in custom zone,
//       MTK use the HalStatus defined in
//       include/mtkcam-android/main/hal/core/device/3.x/device/types.h, and HalStatus will not
//       converted to Status(AOSP hidl defined) until in Hidl Adation Layer.
//
//       we define mizone::Status following the enum name of Status(AOSP hidl defined), while using
//       the value defined by MTK HalStatus.
//
//       refer to:
//       1. mapToHidlCameraStatus() defined in
//          mtkcam-android/main/hal/hidl/include/HidlCameraCommon.h;
//       2. HalStatus defined in
//          mtkcam-android/include/mtkcam-android/main/hal/core/device/3.x/device/types.h.
enum Status {
    OK = 0,
    ILLEGAL_ARGUMENT = -EINVAL,
    CAMERA_IN_USE = -EBUSY,
    MAX_CAMERAS_IN_USE = -EUSERS,
    METHOD_NOT_SUPPORTED = -ENOSYS,
    OPERATION_NOT_SUPPORTED = -EOPNOTSUPP,
    CAMERA_DISCONNECTED = -EPIPE,
    INTERNAL_ERROR = -ENODEV,
};

enum SnapMessageType {
    HAL_BUFFER_RESULT = 0,
    HAL_SHUTTER_NOTIFY = 1,
    HAL_MESSAGE_COUNT = 2,
};

}; // namespace mizone

#endif // INCLUDE_MTKCAM_ANDROID_MAIN_HAL_ANDROID_DEVICE_3_X_TYPES_H_
