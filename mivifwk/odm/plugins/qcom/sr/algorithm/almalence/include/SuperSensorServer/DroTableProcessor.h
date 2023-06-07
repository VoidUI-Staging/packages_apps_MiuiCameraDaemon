#ifndef __DROTABLEPROCESSOR_H__
#define __DROTABLEPROCESSOR_H__

#include <cstdint>

namespace almalence {

/**
 * DRO tables calculation helper class.
 * Separated from Super_process because it should be calculated on CPU and should not waste DSP
 * time.
 */
class DroTableProcessor
{
private:
    const bool local_mapping;

    long long last;
    float last_gamma;
    bool last_local_mapping;

    float max_black_level;
    float black_level_atten;

    std::uint32_t hist_loc[3][3][256];
    std::uint32_t hist_sum[256];

    std::uint32_t hist_base[256];
    std::int32_t lookup_table_base[3][3][256];
    std::int32_t lookup_table_update[3][3][256];
    std::int32_t lookup_table_cur[3][3][256];
    std::int32_t lookup_table[3][3][256];
    int transition_counter;
    int chill_counter;

    int lookup_table_update_is_valid;

    float mix_factor;
    float mix_factor_tone;

    float gamma_curve_offset;
    float gamma_curve_strength;

public:
    DroTableProcessor();

    virtual ~DroTableProcessor();

    int compute(uint8_t *const image, const int sx, const int sy, const int stride,
                const int sensitivity, const float gamma, const float gammaCurveOffset,
                const float gammaCurveStrength, const float blackLevelAttenuation);

    float getCorrectedGamma() const;

    void get(float &gammaCorrected, int32_t lookup_table[3][3][256]);
};

} // namespace almalence

#endif //__DROTABLEPROCESSOR_H__
