static inline uint8_t pixelBilinInterp(uint8_t *p, int px, int py, int sx, int ch)
{
    return (((16 - px) * p[0] + px * p[ch]) * (16 - py) +
            ((16 - px) * p[sx * ch] + px * p[(sx + 1) * ch]) * py) >>
           8;
}

static inline void FinalDownscale_Init(int32_t *xf, int x_st, int x_en, int stride, int mx)
{
    int x = x_st, x0 = x * mx;
#ifdef USE_NEON
    uint16x8_t v3, v4, v5;
    uint32x4_t q0, q1, q2 = vdupq_n_u32(mx * 8);
    const uint32_t grad[] = {0, 1, 2, 3};
    q0 = vmlaq_n_u32(vdupq_n_u32(x0), vld1q_u32(grad), mx);
    q1 = vaddq_u32(q0, vdupq_n_u32(mx * 4));

#define M1(v0)                                                                          \
    v0 = vsubq_u16(vcombine_u16(vshrn_n_u32(q0, 16 - 4), vshrn_n_u32(q1, 16 - 4)), v3); \
    q0 = vaddq_u32(q0, q2);                                                             \
    q1 = vaddq_u32(q1, q2);

    if (stride != 2) {
        uint8x16_t c15 = vdupq_n_u8(15);
        for (; x < x_en - 15; x += 16, x0 += 16 * mx) {
            v3 = vdupq_n_u16((x0 >> (16 - 4)) & -16);
            M1(v4)
            M1(v5)
            vst1q_u8((uint8_t *)&xf[x + 4], vcombine_u8(vshrn_n_u16(v4, 4), vshrn_n_u16(v5, 4)));
            vst1q_u8((uint8_t *)&xf[x + 8],
                     vandq_u8(vcombine_u8(vmovn_u16(v4), vmovn_u16(v5)), c15));
            xf[x] = x0 >> 16;
        }
    } else {
        uint8x8_t c1 = vdup_n_u8(1), c2 = vdup_n_u8(~1), c15 = vdup_n_u8(15);
        for (; x < x_en - 7; x += 8, x0 += 8 * mx) {
            v3 = vdupq_n_u16((x0 >> (16 - 4)) & -16);
            M1(v4)
            uint8x8_t v0 = vmovn_u16(v4), v2 = vshr_n_u8(v0, 3);
            uint8x8x2_t v1 = vzip_u8(vand_u8(v2, c2), vorr_u8(v2, c1));
            vst1q_u8((uint8_t *)&xf[x + 4], vcombine_u8(v1.val[0], v1.val[1]));
            vst1_u8((uint8_t *)&xf[x + 2], vand_u8(v0, c15));
            xf[x] = x0 >> 16;
        }
    }
#endif
    for (; x < x_en; x++, x0 += mx)
        xf[x] = x0 >> (16 - 4);
#undef M1
}

static void FinalDownscale_Stripe(uint8_t *in, uint8_t *out, int32_t *xf, int sxi, int syi, int sxo,
                                  int syo, int x_st, int x_en, int y_st, int y_en, int stride,
                                  int my)
{
    int x, y, yi, yf;
#ifdef USE_NEON
    uint8x16_t v0x, v1x, c1 = vdupq_n_u8(16);
    uint16x8_t v0, v1, v2, v3, v4, v5, c2 = vdupq_n_u16(16);

#ifdef __aarch64__
    uint8x16_t c3 = vdupq_n_u8(stride);
#define M4            \
    uint8x16x2_t v4q; \
    uint8x16_t v6 = vld1q_u8((uint8_t *)&xf[x + 4]);
#define M3(sptr, v2, v3)                                                          \
    v4q.val[0] = vld1q_u8(sptr);                                                  \
    v4q.val[1] = vld1q_u8(sptr + 16);                                             \
    v0x = vqtbl2q_u8(v4q, v6);                                                    \
    v1x = vqtbl2q_u8(v4q, vaddq_u8(v6, c3));                                      \
    v2 = vmlal_u8(vmull_u8(vget_low_u8(v0x), vget_low_u8(v4x)), vget_low_u8(v1x), \
                  vget_low_u8(v5x));                                              \
    v3 = vmlal_high_u8(vmull_high_u8(v0x, v4x), v1x, v5x);
#else
    uint8x8_t c3 = vdup_n_u8(stride);
#define M4           \
    uint8x8x4_t v4q; \
    uint8x8_t v6l = vld1_u8((uint8_t *)&xf[x + 4]), v6h = vld1_u8((uint8_t *)&xf[x + 6]);
#define M3(sptr, v2, v3)                                                                           \
    v0x = vld1q_u8(sptr);                                                                          \
    v1x = vld1q_u8(sptr + 16);                                                                     \
    v4q.val[0] = vget_low_u8(v0x);                                                                 \
    v4q.val[1] = vget_high_u8(v0x);                                                                \
    v4q.val[2] = vget_low_u8(v1x), v4q.val[3] = vget_high_u8(v1x);                                 \
    v2 = vmlal_u8(vmull_u8(vtbl4_u8(v4q, v6l), vget_low_u8(v4x)), vtbl4_u8(v4q, vadd_u8(v6l, c3)), \
                  vget_low_u8(v5x));                                                               \
    v3 = vmlal_u8(vmull_u8(vtbl4_u8(v4q, v6h), vget_high_u8(v4x)),                                 \
                  vtbl4_u8(v4q, vadd_u8(v6h, c3)), vget_high_u8(v5x));
#endif
#define M2(pos)                                                \
    v1 = vdupq_n_u16(yf);                                      \
    v0 = vsubq_u16(c2, v1);                                    \
    v2 = vmlaq_u16(vmulq_u16(v4, v1), v2, v0);                 \
    v3 = vmlaq_u16(vmulq_u16(v5, v1), v3, v0);                 \
    v0x = vcombine_u8(vshrn_n_u16(v2, 8), vshrn_n_u16(v3, 8)); \
    vst1q_u8(&out[pos], v0x);
#endif

    if (stride != 2)
        for (y = y_st; y < y_en; y++) {
            yf = (y * my) >> (16 - 4);
            yi = yf >> 4;
            yf &= 15;

            x = x_st;
#ifdef USE_NEON
            for (; x < x_en - 15; x += 16) {
                uint8_t *sptr = &in[xf[x] + yi * sxi];
#if 0
			int i; uint8_t *buf = (uint8_t*)&xf[x+4];
			for (i = 0; i < 16; i++)
				out[x+y*sxo+i] = pixelBilinInterp(&sptr[buf[i]], buf[i+16], yf, sxi, 1);
#else
                M4 uint8x16_t v5x = vld1q_u8((uint8_t *)&xf[x + 8]), v4x = vsubq_u8(c1, v5x);

                M3(sptr, v2, v3)
                M3(sptr + sxi, v4, v5)
                M2(x + y * sxo)
#endif
            }
#endif
            for (; x < x_en; x++)
                out[x + y * sxo] =
                    pixelBilinInterp(&in[(xf[x] >> 4) + yi * sxi], xf[x] & 15, yf, sxi, 1);
        }
    else
        for (y = y_st; y < y_en; y++) {
            yf = (y * my) >> (16 - 4);
            yi = yf >> 4;
            yf &= 15;

            x = x_st;
#ifdef USE_NEON
            for (; x < x_en - 7; x += 8) {
                uint8_t *sptr = &in[(xf[x] + yi * sxi) * 2];
#if 0
			int i; uint8_t *buf = (uint8_t*)&xf[x+2];
			for (i = 0; i < 16; i++)
				out[(x+y*sxo)*2 + i] = pixelBilinInterp(&sptr[buf[i+8]], buf[i>>1], yf, sxi, 2);
#else
                M4 uint8x8_t d5 = vld1_u8((uint8_t *)&xf[x + 2]);
                uint8x8x2_t q5 = vzip_u8(d5, d5);
                uint8x16_t v5x = vcombine_u8(q5.val[0], q5.val[1]), v4x = vsubq_u8(c1, v5x);

                M3(sptr, v2, v3)
                M3(sptr + sxi * 2, v4, v5)
                M2((x + y * sxo) * 2)
#endif
            }
#endif
            for (; x < x_en; x++) {
                out[(x + y * sxo) * 2] =
                    pixelBilinInterp(&in[((xf[x] >> 4) + yi * sxi) * 2], xf[x] & 15, yf, sxi, 2);
                out[(x + y * sxo) * 2 + 1] = pixelBilinInterp(
                    &in[((xf[x] >> 4) + yi * sxi) * 2 + 1], xf[x] & 15, yf, sxi, 2);
            }
        }
#undef M2
#undef M3
#undef M4
}
