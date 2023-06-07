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

#define ABS_MAX_FACE_NUM						10
#define ABS_MAX_IMAGE_FACE_NUM					10
#define ABS_MAX_VIDEO_FACE_NUM					4

typedef MInt32	ABS_MODE;
#define ABS_MODE_0						        0x00000000		// TRADITION mode for female
#define ABS_MODE_1						        0x00000001		// TEXTURE mode for male
#define ABS_MODE_2						        0x00000002		// Performance first mode

typedef MInt32	ABS_PLATFORM_TYPE;
#define ABS_PLATFORM_TYPE_CPU					0x00000000		// CPU
#define ABS_PLATFORM_TYPE_HVX					0x00000001		// QC HVX

typedef MInt32	ABS_REGION;
#define ABS_REGION_DEFAULT				        0x00000000
#define ABS_REGION_CHINA				        0x00000001
#define ABS_REGION_INDIAN				        0x00000002

/* Error Definitions */

// No Error.
#define ABS_OK									0
// Unknown error.
#define ABS_ERR_UNKNOWN							-1
// Invalid parameter, maybe caused by NULL pointer.
#define ABS_ERR_INVALID_INPUT					-2
// User abort.
#define ABS_ERR_USER_ABORT						-3
// Unsupported image format.
#define ABS_ERR_IMAGE_FORMAT					-101
// Image Size unmatched.
#define ABS_ERR_IMAGE_SIZE_UNMATCH				-102
// Out of memory.
#define ABS_ERR_ALLOC_MEM_FAIL					-201
// No face input while faces needed.
#define ABS_ERR_NOFACE_INPUT					-903
// Interface version error. Wrong head file for so.
#define ABS_ERR_INTERFACE_VERSION				-999

static const MInt32 ABS_INTERFACE_VERSION							= 11301;

// Begin of feature key definitions

// Feature key for intensities of Eye Brighten. Intensity from 0 to 100.
static const MInt32 ABS_KEY_BASIC_EYE_BRIGHTEN						= 0x02010202;
// Feature key for intensity of Skin Tone Filter Style. Intensity from 0 to 100.
static const MInt32 ABS_KEY_BASIC_SKIN_TONE_FILTER_STYLE			= 0x0201090b;
// Feature key for intensities of Skin Soften. Intensity from 0 to 100.
static const MInt32 ABS_KEY_BASIC_SKIN_SOFTEN						= 0x02010b01;
// Feature key for intensities of Makeup Level. Intensity from 0 to 100,default 50.
static const MInt32 ABS_KEY_MAKEUP_LEVEL_NEW						= 0x02020001;
// Feature key for intensities of Hairline. Intensity from -100 to 100.
static const MInt32 ABS_KEY_SHAPE_HAIRLINE							= 0x02040101;
// Feature key for intensities of Eye Enlargement. Intensity from 0 to 100.
static const MInt32 ABS_KEY_SHAPE_EYE_ENLARGEMENT					= 0x02040201;
// Feature key for intensities of Face Shrink. Intensity from 0 to 100.
static const MInt32 ABS_KEY_SHAPE_FACE_SHRINK						= 0x02041501;
// Feature key for intensities of Nose Augment Shape. Intensity from 0 to 100.
static const MInt32 ABS_KEY_SHAPE_NOSE_AUGMENT_SHAPE				= 0x02040601;
// Feature key for intensities of Lip Plump. Intensity from -100 to 100.
static const MInt32 ABS_KEY_SHAPE_LIP_PLUMP							= 0x02040c01;
// Feature key for intensities of Chin Lengthen. Intensity from -100 to 100.
static const MInt32 ABS_KEY_SHAPE_CHIN_LENGTHEN						= 0x02041001;

// End of feature key definitions

typedef struct _tagFaces {
	MRECT		prtFaces[ABS_MAX_FACE_NUM];							// The array of face rectangles
	MUInt32		lFaceNum;											// Number of faces array
	MInt32		plFaceRoll[ABS_MAX_FACE_NUM];						// The roll angle of each face, between [0, 360)
} ABS_TFaces;

#endif /* _ABS_TYPES_H_ */
