#ifndef __MI_FMT__
#define __MI_FMT__

namespace mialgo {

#define VIDEO_MAX_PLANES 8

typedef enum {
    CAM_FORMAT_JPEG = 0x100,        // ImageFormat's JPG
    CAM_FORMAT_RAW16 = 0x20,
    CAM_FORMAT_YUV_420_NV12 = 0x23, // ImageFormat's YUV_420_888
    CAM_FORMAT_RAW10 = 0x25,
    CAM_FORMAT_RAW12 = 0x26,
    CAM_FORMAT_YUV_420_NV21 = 0x11, // ImageFormat's NV21
    CAM_FORMAT_Y16 = 0x20363159,    // ImageFormat's Y16
    CAM_FORMAT_Y8 = 0x20203859,     // ImageFormat's Y8
    CAM_FORMAT_P010 = 0x7FA30C0A,   // ImageFormat's P010
    CAM_FORMAT_MAX
} MiaFormat;

typedef struct _MiaDim
{
    uint32_t width;
    uint32_t height;
} MiaDim;

typedef struct
{
    uint32_t len;
    uint32_t y_offset;
    uint32_t cbcr_offset;
} mia_sp_len_offset_t; // 2 ORIGNAL: cam_sp_len_offset_t

typedef struct
{
    uint32_t len;
    uint32_t offset;
    int32_t offset_x;
    int32_t offset_y;
    int32_t stride;
    int32_t stride_in_bytes;
    int32_t scanline;
    int32_t width;     /* width without padding */
    int32_t height;    /* height without padding */
} mia_mp_len_offset_t; // 2 ORIGNAL:cam_mp_len_offset_t;

typedef enum {
    MI_PAD_NONE = 1,
    MI_PAD_TO_2 = 2,
    MI_PAD_TO_4 = 4,
    MI_PAD_TO_WORD = MI_PAD_TO_4,
    MI_PAD_TO_8 = 8,
    MI_PAD_TO_16 = 16,
    MI_PAD_TO_32 = 32,
    MI_PAD_TO_64 = 64,
    MI_PAD_TO_1K = 1024,
    MI_PAD_TO_2K = 2048,
    MI_PAD_TO_4K = 4096,
    MI_PAD_TO_8K = 8192
} mia_pad_format_t,
    *mia_pad_format_p; // ORIGNAL: cam_pad_format_t

typedef struct
{
    uint32_t width_padding;
    uint32_t height_padding;
    uint32_t plane_padding;
    uint32_t min_stride;
    uint32_t min_scanline;
} mia_padding_info_t, *mia_padding_info_p; // ORIGNAL: cam_padding_info_t

typedef struct
{
    uint32_t num_planes;
    union {
        mia_sp_len_offset_t sp;
        mia_mp_len_offset_t mp[VIDEO_MAX_PLANES];
    };
    uint32_t frame_len;
} MiaFrame_len_offset_t, *MiaFrame_len_offset_p; // 2 ORIGNAL: cam_frame_len_offset_t

} // namespace mialgo

#endif // END define __MI_FMT__
