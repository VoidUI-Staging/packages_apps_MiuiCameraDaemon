#ifndef __MI_FMT__
#define __MI_FMT__

namespace mialgo2 {

#define MAX_PLANES 3

typedef struct _MiaDim
{
    uint32_t width;
    uint32_t height;
} MiaDim;

struct LenOffset
{
    uint32_t len;
    uint32_t offset;
    int32_t offset_x;
    int32_t offset_y;
    int32_t stride;
    int32_t stride_in_bytes;
    int32_t scanline;
    int32_t width;  /* width without padding */
    int32_t height; /* height without padding */
};

enum {
    MI_PAD_NONE = 1,
    MI_PAD_TO_2 = 2,
    MI_PAD_TO_4 = 4,
    MI_PAD_TO_WORD = MI_PAD_TO_4,
    MI_PAD_TO_8 = 8,
    MI_PAD_TO_16 = 16,
    MI_PAD_TO_32 = 32,
    MI_PAD_TO_64 = 64,
    MI_PAD_TO_256 = 256,
    MI_PAD_TO_512 = 512,
    MI_PAD_TO_1K = 1024,
    MI_PAD_TO_2K = 2048,
    MI_PAD_TO_4K = 4096,
    MI_PAD_TO_8K = 8192
};

struct PaddingInfo
{
    uint32_t width_padding;
    uint32_t height_padding;
    uint32_t plane_padding;
    uint32_t min_stride;
    uint32_t min_scanline;
};

struct LenOffsetInfo
{
    uint32_t num_planes;
    LenOffset mp[MAX_PLANES];
    uint32_t frame_len;
};

} // namespace mialgo2

#endif // END define __MI_FMT__
