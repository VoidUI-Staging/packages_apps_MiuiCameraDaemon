/* ------------------------------------------------------------------------- *\

 Almalence, Inc.
 3803 Mt. Bonnell Rd
 Austin, 78731
 Texas, USA

 CONFIDENTIAL: CONTAINS CONFIDENTIAL PROPRIETARY INFORMATION OWNED BY
 ALMALENCE, INC., INCLUDING BUT NOT LIMITED TO TRADE SECRETS,
 KNOW-HOW, TECHNICAL AND BUSINESS INFORMATION. USE, DISCLOSURE OR
 DISTRIBUTION OF THE SOFTWARE IN ANY FORM IS LIMITED TO SPECIFICALLY
 AUTHORIZED LICENSEES OF ALMALENCE, INC. ANY UNAUTHORIZED DISCLOSURE
 IS A VIOLATION OF STATE, FEDERAL, AND INTERNATIONAL LAWS.
 BOTH CIVIL AND CRIMINAL PENALTIES APPLY.

 DO NOT DUPLICATE. UNAUTHORIZED DUPLICATION IS A VIOLATION OF STATE,
 FEDERAL AND INTERNATIONAL LAWS.

 USE OF THE SOFTWARE IS AT YOUR SOLE RISK. THE SOFTWARE IS PROVIDED ON AN
 "AS IS" BASIS AND WITHOUT WARRANTY OF ANY KIND. TO THE MAXIMUM EXTENT
 PERMITTED BY LAW, ALMALENCE EXPRESSLY DISCLAIM ALL WARRANTIES AND
 CONDITIONS OF ANY KIND, WHETHER EXPRESS OR IMPLIED, INCLUDING, BUT NOT
 LIMITED TO THE IMPLIED WARRANTIES AND CONDITIONS OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.

 ALMALENCE DOES NOT WARRANT THAT THE SOFTWARE WILL MEET YOUR REQUIREMENTS,
 OR THAT THE OPERATION OF THE SOFTWARE WILL BE UNINTERRUPTED OR ERROR-FREE,
 OR THAT DEFECTS IN THE SOFTWARE WILL BE CORRECTED. UNDER NO CIRCUMSTANCES,
 INCLUDING NEGLIGENCE, SHALL ALMALENCE, OR ITS DIRECTORS, OFFICERS,
 EMPLOYEES OR AGENTS, BE LIABLE TO YOU FOR ANY INCIDENTAL, INDIRECT,
 SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE, MISUSE OR
 INABILITY TO USE THE SOFTWARE OR RELATED DOCUMENTATION.

 COPYRIGHT 2010-2014, ALMALENCE, INC.

 ---------------------------------------------------------------------------

  Super Zoom public header

\* ------------------------------------------------------------------------- */

#ifndef __SUPERZOOM_H__
#define __SUPERZOOM_H__

#if defined __cplusplus
extern "C" {
#endif

#include "almashot.h"

typedef struct
{
    const char *name;
    union {
        int64_t i;
        double f;
        void *p;
    };
} alma_config_t;

enum {
    ALMA_TYPE_BAD_CAMERA = -2,
    ALMA_TYPE_UNKNOWN = -1,
    ALMA_TYPE_INT = 0,
    ALMA_TYPE_FLOAT = 1,
    ALMA_TYPE_INT_ARRAY = 2,
    ALMA_TYPE_FLOAT_ARRAY = 3,
    ALMA_TYPE_BOOL = 4
};

typedef union {
    int32_t i;
    float f;
    struct
    {
        void *ptr;
        int len;
    } arr;
} alma_value_t;

int Super_ReadProfile(int cameraIndex, const char *name, alma_value_t *out);
int Super_UpdateProfile(int cameraIndex, alma_config_t *strconf);
int Super_ProcessNew(void **inst, uint8_t **in, uint8_t *out, int sx, int sy, int sxo, int syo,
                     int nImages, int iso, alma_config_t *conf);
void Super_CancelProcessing(void *instance);
int Super_StopStreaming(void *instance, int keepInstance);

int convertYUV10_to_NV21(uint16_t *y10, uint16_t *uv10, uint8_t *nv21, uint8_t *nv21uv, int cx,
                         int cy, int width, int height, int stride, int sy, int ostride);
int convertNV21_to_YUV10(uint8_t *nv21, uint8_t *nv21uv, uint16_t *y10, uint16_t *uv10, int cx,
                         int cy, int width, int height, int stride, int sy, int ostride);

// Maximum accepted input frames
#define SUPERZOOM_MAX_FRAMES 15

// Input frames should be that much larger at each edge
// to provide expected zoom level when SizeGuaranteeMode is set
#ifndef SIZE_GUARANTEE_BORDER // Eugene: added this to make it possible to define the border size in
                              // Makefile
#define SIZE_GUARANTEE_BORDER 64
#endif

// -------------------------------------------------------------------
// Still image superzoom functions

enum {
    ALMA_SUPER_NOSRES = 1,
    ALMA_SUPER_NOLARGEDISP = 2, // !largeDisplacements
    ALMA_SUPER_DROLOCAL = 4,
    ALMA_SUPER_NOSIZEGUARANTEE = 0x10,
    ALMA_SUPER_VIDEOBOUND = 0x20, // special frame bounding for RTSS
    ALMA_SUPER_DROSLOVENIAN = 0x100,
    ALMA_SUPER_NLNET = 0x200,
    ALMA_SUPER_FUSEBICUBIC = 0x400,
    ALMA_SUPER_NORMALIZE = 0x800,
    ALMA_SUPER_FORCE15X = 0x4000,
    ALMA_SUPER_NEWFILTER = 0x8000,
    ALMA_SUPER_ADRCFIX = 0x80000000,

    ALMA_SUPER2_ENHEDGES3 = 0x80,
    ALMA_SUPER2_ENHEDGES5 = 0x400,
    ALMA_SUPER2_BIL11UV = 0x200,
    ALMA_SUPER2_PRECISEALIGNMENT = 0x800
};

#if defined __cplusplus
}
#endif

#endif // __SUPERZOOM_H__
