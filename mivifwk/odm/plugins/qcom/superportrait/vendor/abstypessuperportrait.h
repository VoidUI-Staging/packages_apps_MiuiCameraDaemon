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

#ifndef _ABS_TYPES_superportrait_H_
#define _ABS_TYPES_superportrait_H_

#include "amcomdef.h"

#define ABS_MAX_FACE_NUM				10
#define ABS_MAX_IMAGE_FACE_NUM			10
#define ABS_MAX_VIDEO_FACE_NUM			4

/* Error Definitions */

// No Error.
#define ABS_OK							0
// Unknown error.
#define ABS_ERR_UNKNOWN					-1
// Invalid parameter, maybe caused by NULL pointer.
#define ABS_ERR_INVALID_INPUT			-2
// User abort.
#define ABS_ERR_USER_ABORT				-3
// Unsupported image format.
#define ABS_ERR_IMAGE_FORMAT			-101
// Image Size unmatched.
#define ABS_ERR_IMAGE_SIZE_UNMATCH		-102
// Out of memory.
#define ABS_ERR_ALLOC_MEM_FAIL			-201
// No face input while faces needed.
#define ABS_ERR_NOFACE_INPUT			-903

// Begin of feature key definitions
// Base value of feature key.
static const MInt32 FEATURE_BASE_superporttrait								= 200;
// Feature key for enable(level > 0) or disable status of Super Portrait.
static const MInt32 FEATURE_SUPER_PORTRAIT_KEY 				= FEATURE_BASE_superporttrait+115;

typedef struct _tagFaces {
	MRECT		prtFaces[ABS_MAX_FACE_NUM];			// The array of face rectangles
	MUInt32		lFaceNum;							// Number of faces array
	MInt32		plFaceRoll[ABS_MAX_FACE_NUM];		// The roll angle of each face, between [0, 360)
} ABS_TFaces;
//
typedef struct _tagLevelArray {
	MInt32		pLevels[ABS_MAX_FACE_NUM];			// The array of feature levels
	MUInt32		lFaceNum;							// Number of faces
} ABS_TLevelArray;

typedef struct _tagFacesResult_superporttrait {
	MInt32		pAges[ABS_MAX_FACE_NUM];			// The array of faces' age
	MInt32		pGenders[ABS_MAX_FACE_NUM];		    /** The array of faces' gender
	                                                "1" represents female, "0" represents male, and "-1" represents unknown.**/
	MInt32		pRaces[ABS_MAX_FACE_NUM];		    /** The array of faces' race
	                                                "0" represents yellow,"1" represents white,"2" represents black, "3" represents india,and "-1" represents unknown.**/
	MInt32      pSkinQuality[ABS_MAX_FACE_NUM];     /** The array of skin quality
	                                                "0" represents A+,"1" represents A,"2" represents B, "3" represents C,and "4" represents D.**/
	MUInt32		lFaceNum;							// Number of faces array
} ABS_TFacesResult_superporttrait;

#endif /* _ABS_TYPES_H_ */
