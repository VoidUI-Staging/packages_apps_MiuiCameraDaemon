/*----------------------------------------------------------------------------------------------
 *
 * This file is ArcSoft's property. It contains ArcSoft's trade secret, proprietary and
 * confidential information.
 *
 * The information and code contained in this file is only for authorized ArcSoft employees
 * to design, create, modify, or review.
 *
 * DO NOT DISTRIBUTE, DO NOT DUPLICATE OR TRANSMIT IN ANY FORM WITHOUT PROPER AUTHORIZATION.
 *
 * If you are not an intended recipient of this file, you must not copy, distribute, modify,
 * or take any action in reliance on it.
 *
 * If you have received this file in error, please immediately notify ArcSoft and
 * permanently delete the original and any copy of any file and any printout thereof.
 *
 *-------------------------------------------------------------------------------------------------*/
/*
 * abstypes.h
 *
 *
 */

#ifndef _ABS_TYPES_H_
#define _ABS_TYPES_H_

#include "amcomdef.h"

#define ABS_MAX_FACE_NUM       10
#define ABS_MAX_IMAGE_FACE_NUM 10
#define ABS_MAX_VIDEO_FACE_NUM 4

// For different preview's skinsoften of different platform
#define ABS_VIDEO_PLATFORM_TYPE_CPU 0x00000000
#define ABS_VIDEO_PLATFORM_TYPE_HVX 0x00000001

/* Error Definitions */

// No Error.
#define ABS_OK 0
// Unknown error.
#define ABS_ERR_UNKNOWN -1
// Invalid parameter, maybe caused by NULL pointer.
#define ABS_ERR_INVALID_INPUT -2
// User abort.
#define ABS_ERR_USER_ABORT -3
// Unsupported image format.
#define ABS_ERR_IMAGE_FORMAT -101
// Image Size unmatched.
#define ABS_ERR_IMAGE_SIZE_UNMATCH -102
// Out of memory.
#define ABS_ERR_ALLOC_MEM_FAIL -201
// No face input while faces needed.
#define ABS_ERR_NOFACE_INPUT -903
// Interface version error. Wrong head file for so.
#define ABS_ERR_INTERFACE_VERSION -999

static const MInt32 ABS_INTERFACE_VERSION = 230002;

// Begin of feature key definitions
// Feature key for enable(level > 0) or disable status of De-Blemish.
static const MInt32 ABS_KEY_BASIC_DE_BLEMISH = 0x02010101;
// Feature key for intensities of Eye Brighten. Intensity from 0 to 100.
static const MInt32 ABS_KEY_BASIC_EYE_BRIGHTEN = 0x02010202;
// Feature key for intensities of Pupil Line. Intensity from 0 to 100.
static const MInt32 ABS_KEY_BASIC_PUPIL_LINE = 0x02010401;
// Feature key for intensities of De-Pouch. Intensity from 0 to 100.
static const MInt32 ABS_KEY_BASIC_DE_POUCH = 0x02010501;
// Feature key for intensities and color of Blush. Intensity from 0 to 100.
static const MInt32 ABS_KEY_MAKEUP_BLUSH = 0x02020601;
// Feature key for intensities of Teeth whiten. Intensity from 0 to 100.
static const MInt32 ABS_KEY_BASIC_TEETH_WHITEN = 0x02010701;
// Feature key for intensity of Skin Brighten. Intensity from 0 to 100.
static const MInt32 ABS_KEY_BASIC_SKIN_BRIGHTEN = 0x02010901;
// Feature key for intensity of Clarity. Intensity from 0 to 100.
static const MInt32 ABS_KEY_BASIC_CLARITY = 0x02010902;
// Feature key for intensity of Skin Styling. Intensity from 0 to 100.
static const MInt32 ABS_KEY_BASIC_SKIN_STYLING_LEVEL = 0x02010903;
// Feature key for value of Skin Styling. Value from -100 to 100. Default value is 0.
static const MInt32 ABS_KEY_BASIC_SKIN_STYLING_VALUE = 0x02010904;
// Feature key for intensities of skin soften. Intensity from 0 to 100.
static const MInt32 ABS_KEY_BASIC_SKIN_SOFTEN = 0x02010b01;
// Feature key for intensities and color of Eyebrow Dyeing. Intensity from 0 to 100.
static const MInt32 ABS_KEY_BASIC_EYEBROW_DYEING = 0x02010d01;
// Feature key for intensities of Lip Beauty. Intensity from 0 to 100.
static const MInt32 ABS_KEY_MAKEUP_LIP_BEAUTY = 0x02020101;
// Feature key for intensity of Lip Gloss. Intensity from 0 to 100.
static const MInt32 ABS_KEY_MAKEUP_LIP_GLOSS = 0x02020202;
// Feature key for intensities of Hairline. Intensity from -100 to 100.
static const MInt32 ABS_KEY_SHAPE_HAIRLINE = 0x02040101;
// Feature key for intensities of eye enlargement. Intensity from 0 to 100.
static const MInt32 ABS_KEY_SHAPE_EYE_ENLARGEMENT = 0x02040201;
// Feature key for intensities of face slender. Intensity from 0 to 100.
static const MInt32 ABS_KEY_SHAPE_FACE_SLENDER = 0x02041401;
// Feature key for intensities of Nose Augment Shape. Intensity from 0 to 100.
static const MInt32 ABS_KEY_SHAPE_NOSE_AUGMENT_SHAPE = 0x02040601;
// Feature key for intensities of Nose Highlight. Intensity from 0 to 100.
static const MInt32 ABS_KEY_SHAPE_NOSE_HIGHLIGHT = 0x02040603;
// Feature key for intensities of Apple Zone Plump. Intensity from 0 to 100.
static const MInt32 ABS_KEY_SHAPE_APPLE_ZONE_PLUMP = 0x02040901;
// Feature key for intensities of Lip Plump. Intensity from -100 to 100.
static const MInt32 ABS_KEY_SHAPE_LIP_PLUMP = 0x02040c01;
// Feature key for intensities of Chin Lengthen. Intensity from -100 to 100.
static const MInt32 ABS_KEY_SHAPE_CHIN_LENGTHEN = 0x02041001;

// End of feature key definitions

typedef struct _tagFaces
{
    MRECT prtFaces[ABS_MAX_FACE_NUM];    // The array of face rectangles
    MUInt32 lFaceNum;                    // Number of faces array
    MInt32 plFaceRoll[ABS_MAX_FACE_NUM]; // The roll angle of each face, between [0, 360)
} ABS_TFaces;

#endif /* _ABS_TYPES_H_ */
