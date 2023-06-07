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

 COPYRIGHT 2012-2014, ALMALENCE, INC.

 ---------------------------------------------------------------------------

 Dynamic range optimizer interface

\* ------------------------------------------------------------------------- */

#ifndef __DRO_H__
#define __DRO_H__

#include <stdint.h>

#define LOOKUP_FRAC  10
#define LOOKUP_HALF  (1 << (LOOKUP_FRAC - 1))
#define LOOKUP_UNITY (1 << LOOKUP_FRAC)

#if defined __cplusplus
extern "C" {
#endif

// GetHistogramNV21 - computes histogram
// Input:
//         in       - frame in NV21 format
//         sx,sy    - frame dimensions
//         hist     - array to hold global histogram
//         hist_loc - arrays to hold local histograms, pass NULL to not compute local histograms
//         mix_factor - used in local histograms computation.
//                    Defines the level of inter-dependence of image areas. Range: [0..1].
//                    Default: 1.0
// Output:
//         hist     - Global histogram
//         hist_loc - Local histograms
void Dro_GetHistogramNV21(uint8_t *in, uint8_t *inuv, uint32_t hist[256],
                          uint32_t hist_loc[3][3][256], int sx, int sy, int stride,
                          float mix_factor);

// MixLocalTables - mix local tables (either lookup or histograms) proportionally to mix_factor
void MixLocalTables(uint32_t in[3][3][256], uint32_t out[3][3][256], float mix_factor,
                    int normalize);

// ComputeToneTable - compute tone modification table lookup_table[256].
// Input:
//         hist - pointer to histogram with 256 elements
// Output:
//         lookup_table - filled with tone-table multipliers (in q5.10 fixed-point format)
//
// Parameters:
//         gamma        - Defines how 'bright' the output image will be. Lower values cause brighter
//         output.
//                        Default: 0.5
//         max_black_level - threshold for black level correction. Default: 16
//         black_level_atten - how much to attenuate black level. Default: 0.5
//         min_limit[3] - Minimum limit on contrast reduction. Range: [0..0.9]. Default: {0.5, 0.5,
//         0.5} max_limit[3] - Maximum limit on contrast enhancement. Range: [1..10]. Default:
//                        [0] - for shadows, [1] - for midtones, [2] - for highlights.
//                        {4.0, 2.0, 2.0} - for hdr-like effects;
//                        {3.0, 2.0, 2.0} - for more balanced results.
//         global_limit - Maximum limit on total brightness amplification. Recommended: 4
void Dro_ComputeToneTable(uint32_t hist[256], int32_t lookup_table[256], float gamma,
                          float max_black_level, float black_level_atten, float min_limit[3],
                          float max_limit[3], float global_limit);

// ComputeToneTableLocal - same as ComputeToneTable, but compute 3x3 tone tables.
// Input:
//         hist_loc - 3x3 histograms each with 256 elements
// Output:
//         lookup_table_loc - filled with multipliers for each of 3x3 image areas
//
// Parameters:
//         gamma        - Defines how 'bright' the output image will be. Lower values cause brighter
//         output.
//                        Default: 0.5
//         max_black_level - threshold for black level correction. Default: 16
//         black_level_atten - how much to attenuate black level. Default: 0.5
//         min_limit[3] - Minimum limit on contrast reduction. Range: [0..0.9]. Default: {0.5, 0.5,
//         0.5} max_limit[3] - Maximum limit on contrast enhancement. Range: [1..10]. Default:
//                        [0] - for shadows, [1] - for midtones, [2] - for highlights.
//                        {4.0, 2.0, 2.0} - for hdr-like effects;
//                        {3.0, 2.0, 2.0} - for more balanced results.
//         global_limit - Maximum limit on total brightness amplification. Recommended: 4
//         mix_factor   - Defines the level of inter-dependence of image areas. Range: [0..1].
//                        Recommended: 0.2
void Dro_ComputeToneTableLocal(uint32_t hist_loc[3][3][256], int32_t lookup_table_loc[3][3][256],
                               float gamma, float max_black_level, float black_level_atten,
                               float min_limit[3], float max_limit[3], float global_limit,
                               float mix_factor);

#define ALMA_DRO_SLOVENIAN 1

void Dro_ComputeToneTableEx(uint32_t hist[256], int32_t lookup_table[256], float gamma,
                            float gammaCurveOffset, float gammaCurveStrength, float max_black_level,
                            float black_level_atten, float min_limit[3], float max_limit[3],
                            float global_limit, int flags);

void Dro_ComputeToneTableLocalEx(uint32_t hist_loc[3][3][256], int32_t lookup_table_loc[3][3][256],
                                 float gamma, float gammaCurveOffset, float gammaCurveStrength,
                                 float max_black_level, float black_level_atten, float min_limit[3],
                                 float max_limit[3], float global_limit, float mix_factor,
                                 int flags);

int Dro_ApplyToneTableFilteredNV21(uint8_t *in, uint8_t *inUV, int32_t lookup_table[256],
                                   int32_t lookup_local[3][3][256], int filter, int uv_desat,
                                   int dark_uv_desat, int dark_noise_pass, int sx, int sy,
                                   int stride, void *mem);

#if defined __cplusplus
}
#endif

#endif /* __DRO_H__ */
