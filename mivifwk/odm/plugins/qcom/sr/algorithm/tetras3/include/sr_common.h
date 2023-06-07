#ifndef __SR_COMMON_H__
#define __SR_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

#define SR_SDK_API extern "C" __attribute__((visibility("default")))

typedef struct
{
    int x;
    int y;
    int width;
    int height;
} sr_rect_t;

/// pixel format definition
typedef enum {
    SR_PIX_FMT_YUV420P, ///< YUV  4:2:0   12bpp ( 3通道, 一个亮度通道, 另两个为U分量和V分量通道,
                        ///< 所有通道都是连续的 )
    SR_PIX_FMT_NV12, ///< YUV  4:2:0   12bpp ( 2通道, 一个通道是连续的亮度通道, 另一通道为UV分量交错
                     ///< )
    SR_PIX_FMT_NV21, ///< YUV  4:2:0   12bpp ( 2通道, 一个通道是连续的亮度通道, 另一通道为VU分量交错
                     ///< )

    ///< 10Bit数据格式的标志，该数据格式用16Bits表示10Bits的数据，使用小端字节序布局，高6Bits是无效数据。
    SR_FMT_PLAIN_10BITS,

    ///< P010
    ///< 高通10Bit数据格式，该数据格式用16Bits表示10Bits的数据，使用小端字节序布局，低6Bits是无效数据
    SR_FMT_PLAIN_P010,

    ///< bayer pattern - bggr
    PIX_FMT_BAYER_BGGR,

    ///< bayer pattern - grbg
    PIX_FMT_BAYER_GRBG,

    ///< bayer pattern - rggb
    PIX_FMT_BAYER_RGGB,

    ///< bayer pattern - gbrg
    PIX_FMT_BAYER_GBRG,

    ///< data buffer type - uchar
    SR_DATA_BUFFER_UCHAR,

    ///< data buffer type - float
    SR_DATA_BUFFER_FLOAT,

    ///< YUV  4:2:0   24bpp ( 3通道, 一个亮度通道, 另两个为U分量和V分量通道, 所有通道都是连续的 )
    SR_PIX_FMT_YUV420P_PLAIN_10BITS = SR_PIX_FMT_YUV420P + (SR_FMT_PLAIN_10BITS << 8),
    ///< YUV  4:2:0   24bpp ( 2通道, 一个通道是连续的亮度通道, 另一通道为UV分量交错 )
    SR_PIX_FMT_NV12_PLAIN_10BITS = SR_PIX_FMT_NV12 + (SR_FMT_PLAIN_10BITS << 8),
    ///< YUV  4:2:0   24bpp ( 2通道, 一个通道是连续的亮度通道, 另一通道为VU分量交错 )
    SR_PIX_FMT_NV21_PLAIN_10BITS = SR_PIX_FMT_NV21 + (SR_FMT_PLAIN_10BITS << 8),

    ///< YUV  4:2:0   24bpp ( 3通道, 一个亮度通道, 另两个为U分量和V分量通道, 所有通道都是连续的 )
    SR_PIX_FMT_YUV420P_PLAIN_P010 = SR_PIX_FMT_YUV420P + (SR_FMT_PLAIN_P010 << 8),
    ///< YUV  4:2:0   24bpp ( 2通道, 一个通道是连续的亮度通道, 另一通道为UV分量交错 )
    SR_PIX_FMT_NV12_PLAIN_P010 = SR_PIX_FMT_NV12 + (SR_FMT_PLAIN_P010 << 8),
    ///< YUV  4:2:0   24bpp ( 2通道, 一个通道是连续的亮度通道, 另一通道为VU分量交错 )
    SR_PIX_FMT_NV21_PLAIN_P010 = SR_PIX_FMT_NV21 + (SR_FMT_PLAIN_P010 << 8),

    ///< bayer 数据10bit格式
    PIX_FMT_BAYER_BGGR_10BITS = SR_PIX_FMT_YUV420P + (PIX_FMT_BAYER_BGGR << 8),
    PIX_FMT_BAYER_GRBG_10BITS = SR_PIX_FMT_YUV420P + (PIX_FMT_BAYER_GRBG << 8),
    PIX_FMT_BAYER_RGGB_10BITS = SR_PIX_FMT_YUV420P + (PIX_FMT_BAYER_RGGB << 8),
    PIX_FMT_BAYER_GBRG_10BITS = SR_PIX_FMT_YUV420P + (PIX_FMT_BAYER_GBRG << 8),

    ///< bayer 数据12bit格式
    PIX_FMT_BAYER_BGGR_12BITS = SR_PIX_FMT_NV12 + (PIX_FMT_BAYER_BGGR << 8),
    PIX_FMT_BAYER_GRBG_12BITS = SR_PIX_FMT_NV12 + (PIX_FMT_BAYER_GRBG << 8),
    PIX_FMT_BAYER_RGGB_12BITS = SR_PIX_FMT_NV12 + (PIX_FMT_BAYER_RGGB << 8),
    PIX_FMT_BAYER_GBRG_12BITS = SR_PIX_FMT_NV12 + (PIX_FMT_BAYER_GBRG << 8),

    ///< bayer 数据14bit格式
    PIX_FMT_BAYER_BGGR_14BITS = SR_PIX_FMT_NV21 + (PIX_FMT_BAYER_BGGR << 8),
    PIX_FMT_BAYER_GRBG_14BITS = SR_PIX_FMT_NV21 + (PIX_FMT_BAYER_GRBG << 8),
    PIX_FMT_BAYER_RGGB_14BITS = SR_PIX_FMT_NV21 + (PIX_FMT_BAYER_RGGB << 8),
    PIX_FMT_BAYER_GBRG_14BITS = SR_PIX_FMT_NV21 + (PIX_FMT_BAYER_GBRG << 8),

    ///< bayer 数据16bit格式
    PIX_FMT_BAYER_BGGR_16BITS = SR_FMT_PLAIN_10BITS + (PIX_FMT_BAYER_BGGR << 8),
    PIX_FMT_BAYER_GRBG_16BITS = SR_FMT_PLAIN_10BITS + (PIX_FMT_BAYER_GRBG << 8),
    PIX_FMT_BAYER_RGGB_16BITS = SR_FMT_PLAIN_10BITS + (PIX_FMT_BAYER_RGGB << 8),
    PIX_FMT_BAYER_GBRG_16BITS = SR_FMT_PLAIN_10BITS + (PIX_FMT_BAYER_GBRG << 8),
} sr_pixel_format_t;

// hdr功能输入帧类型
typedef enum {
    HDR_NORMAL = 0, //正常曝光帧　EV0
    HDR_SHORT = 1,  //欠爆帧  EV-
    HDR_LONG = 2    //长爆帧　EV+
} sr_image_shutter_style_t;

typedef struct sr_prev_calc_value
{
    int shutterNum;    ///< 曝光序列个数
    int shutters[128]; ///< 曝光序列指针
} sr_prev_calc_value_t;

typedef struct sr_bhist_adrc
{
    float fRealAdrc;
    float fAdrc;
} sr_bhist_adrc_t;

#define HDR_SHORTER HDR_LONG //更短曝光

// hdr方法
typedef enum {
    THREE_NV12_FRAMES_HDR = 0, //三帧数据的HDR
} st_image_hdr_methods_t;

typedef struct
{
    float evn_expo;              //欠爆帧曝光参数　曝光时间
    float ev0_expo;              //正常曝光帧曝光参数
    float evp_expo;              //过爆帧曝光参数
    float evn_iso;               //欠爆帧增益
    float ev0_iso;               //正常曝光帧增益
    float evp_iso;               //过曝帧增益
    float DR_value;              //当前动态范围DR值
    float DR_threshold;          // HDR场景判断阈值
    float color_temperature_evn; //欠爆帧hdr色温
    float color_temperature_ev0; //正常曝光帧hdr色温
    float color_temperature_evp; //过爆帧hdr色温
} st_hdr_param_t;

/// @brief 时间戳定义
typedef struct sr_time_t
{
    long int tv_sec;  ///< 秒
    long int tv_usec; ///< 微秒
} sr_time_t;

/// 图像格式定义
typedef struct
{
    unsigned char *planes[4];       ///< 图像数据指针
    sr_pixel_format_t pixel_format; ///< 像素格式
    int width;                      ///< 宽度(以像素为单位)
    int height;                     ///< 高度(以像素为单位)
    int stride;                     ///< 跨度, 即每行所占的字节数
    sr_time_t time_stamp;           ///< 时间戳
} sr_image_t;

typedef struct sr_image_param
{
    int nImageGain; /// 拍照ISO
    float fAdrc;    /// 拍照ADRC值
} sr_image_param_t;

typedef struct
{
    float fImgEdgeEnhanceS;
    float fImgContrastEnhanceS;
    float fMotionDetectS;               //[0,10]
    float fModelDenoiseS;               // input[0,1], if < 0, use self-adaption choose
    float fModelFinetuneDenoiseS;       //[0,1]
    float fModelFinetuneMotionDenoiseS; //[0,2]
    int fSharpFilterRadius;
    int fSharpGradinetThresh;
    int fModelRefMaskRadius;
    int fModelRefMaskThresh;
    int fModelRefMaskRatio; // 1 2 4 8
    float fModelFinetuneRefMask;
} sr_specific_param_t;

#define UVDENOISE_LEVEL 5
typedef struct
{
    float levelStrength[UVDENOISE_LEVEL];
    float motionLevelStrength[UVDENOISE_LEVEL];
} sr_uv_denoise_param_t;

typedef struct
{
    float ctThresh;
    float edgeSigma;
    float edgeAmount;
    float cgIncValue;
} sr_detail_sharp_param_t;

typedef struct
{
    sr_image_param_t imageParam;
    sr_specific_param_t specificParam;
    sr_uv_denoise_param_t uvDenoiseParam;
    sr_detail_sharp_param_t detailSharpParam;
} sr_interface_param_t;

#define MAX_INPUT_FRAME_NUM 8
// camera post process data structure
typedef struct
{
    unsigned char *inputYBuffers[MAX_INPUT_FRAME_NUM];  // y point for input frames
    unsigned char *inputUVBuffers[MAX_INPUT_FRAME_NUM]; // uv point for input frames
    unsigned char *outputYBuffer;
    unsigned char *outputUVBuffer;
} sr_buffer_t;

typedef enum {
    PIXEL_DEPTH_8BIT,
    PIXEL_DEPTH_10BIT,
} sr_pixel_depth_e;

typedef struct
{
    int id;                       // camera的id。比如cameraId:1/2/3
    char *name;                   // camera的名字。比如name:wide/tel
    sr_pixel_depth_e pixel_depth; // 输出的bit数，比如8bit、10bit
} sr_camera_info_t;

typedef enum {
    SR_EVENT_IMG_REF_INDEX, // get imgRefIdx
    SR_EVENT_IMG_CROP_ROI,  // get imgCropRoi
    SR_EVENT_MAX,
} sr_event_type_t;

typedef struct
{
    sr_event_type_t type;     // the event type
    union {                   // use union to hold different values according to specified event
        int imgRefIdx;        // reference image index
        sr_rect_t imgCropRoi; // ROI of cropped image
    };
} sr_event_t;

typedef struct
{
    sr_rect_t *pfaceRects;
    int faceNumber;
    int timeStamp;
} sr_face_info_t;

// event callback function type
typedef void (*FUNCPTR_SR_EVENT_CALLBACK)(sr_event_t *event);

enum {
    ARG_IDX_BLACK_LEVEL = 0,
    ARG_IDX_ISO,
    ARG_IDX_AWB_GAINS,
    ARG_IDX_CCM,
    ARG_IDX_R_LSCM,
    ARG_IDX_Gr_LSCM,
    ARG_IDX_Gb_LSCM,
    ARG_IDX_B_LSCM,
    ARG_IDX_R_CURVE,
    ARG_IDX_G_CURVE,
    ARG_IDX_B_CURVE,
    ARG_IDX_Gr_CURVE,
    ARG_IDX_Gb_CURVE,
    ARG_IDX_DIGITAL_GAIN,
};

#define ARG_COUNT_BLACK_LEVEL 3
#define ARG_COUNT_ISO         1
#define ARG_COUNT_AWB_GAINS   3
#define ARG_COUNT_CCM         (3 * 3)
#define ARG_COUNT_R_LSCM      (13 * 17)
#define ARG_COUNT_Gr_LSCM     (13 * 17)
#define ARG_COUNT_Gb_LSCM     (13 * 17)
#define ARG_COUNT_B_LSCM      (13 * 17)
#define ARG_COUNT_R_CURVE     65
#define ARG_COUNT_G_CURVE     65
#define ARG_COUNT_B_CURVE     65

#define ARG_COUNT_DIGITAL_GAIN    1
#define ARG_COUNT_AWB_GAINS_MTK   4
#define ARG_COUNT_BLACK_LEVEL_MTK 4
#define ARG_COUNT_R_CURVE_MTK     192
#define ARG_COUNT_B_CURVE_MTK     192
#define ARG_COUNT_Gr_CURVE_MTK    192
#define ARG_COUNT_Gb_CURVE_MTK    192

typedef enum {
    SR_EXTENTION_NM_ISP_EV0,
    SR_EXTENTION_AI_FULLSIZE,
    SR_EXTENTION_XML_PARAMS,
    SR_EXTENTION_NM_ISP_MTK = 1000,
    SR_EXTENTION_OSSR_DUMP_SWITCH,
    SR_EXTENTION_OSSR_CAM_RATIO,
    SR_EXTENTION_FASE_PARAM,
    SR_EXTENTION_CUST_DUMP_SWITCH,
    SR_EXTENTION_CUST_DUMP_DIR,
} sr_extention_t;

/// ＠brief 用于设置扩展接口参数
/// @param [in] 不同类型扩展接口，其参数为固定长度，并且使用union中指定成员
typedef union {
    int i[16];
    float f[16];
    short s[32];
    char c[64];
} sr_extention_para_t;

typedef enum {
    // 通用类型，chart/int/float等
    SR_EXT_ARG_NORMAL_TYPE = 0,
    SR_EXT_ARG_CHAR_PRT,
    SR_EXT_ARG_INT_PRT,
    SR_EXT_ARG_FOLAT_PRT,
    // .....

    // 非通用的struct类型或者其他自定义类型
    SR_EXT_ARG_STRUCT_TYPE = 128,
    SR_EXT_ARG_SR_IMAGE_PTR, // st_sr_image_t
    SR_EXT_ARG_SR_RECT_PTR,  // st_sr_rect_t
    SR_EXT_ARG_SR_FACE_RECTS_PTR,
    // .....

} sr_ext_arg_type_t;

/* Defines abs camera id according to different camera module */
#define SR_CAMERA_ID_DEFUALT                  100000000
#define SR_CAMERA_ID_J1_TELE_2X_S5K2L7        200010300
#define SR_CAMERA_ID_J1_TELE_4X_OV08A10       220020300
#define SR_CAMERA_ID_J13SH_WIDE_HMX           100000300
#define SR_CAMERA_ID_J13SH_WIDE_2M_CROP_HMX   100000310
#define SR_CAMERA_ID_K2_WIDE_HMX              100000400
#define SR_CAMERA_ID_K2_WIDE_2M_CROP_HMX      100000410
#define SR_CAMERA_ID_K2_WIDE_2M_CROP_HMX_MOON 100000440
#define SR_CAMERA_ID_J17A_WIDE_IMX            100030500
#define SR_CAMERA_ID_J1S_WIDE_OV48C           100040600
#define SR_CAMERA_ID_J1S_2X_S5K2L7            200010600
#define SR_CAMERA_ID_J1S_5X_IMX586            230050600
#define SR_CAMERA_ID_J1S_10X_CROP_IMX586      240050610
#define SR_CAMERA_ID_J1S_48M_OV48C            420040650
#define SR_CAMERA_ID_K11_WIDE_IMX686          100060400
#define SR_CAMERA_ID_K11_WIDE_S5KHM2          100090401
#define SR_CAMERA_ID_K11A_WIDE_IMX582         100081100
#define SR_CAMERA_ID_K9_WIDE_GW3              100070700
#define SR_CAMERA_ID_K9A_WIDE_GW3             100070900
#define SR_CAMERA_ID_K7A_WIDE_IMX582          100080800
#define SR_CAMERA_ID_K6_WIDE_S5KHM2           100090900
#define SR_CAMERA_ID_K6_WIDE_GW3              100070901
#define SR_CAMERA_ID_K7B_WIDE_OV64B           100101000
#define SR_CAMERA_ID_K19_WIDE_OV48B           100116100
#define SR_CAMERA_ID_J20S_WIDE_IMX582         100081200
#define SR_CAMERA_ID_J18_WIDE_S5KHM2          100090400
#define SR_CAMERA_ID_J18_3X_OV08A             210120400
#define SR_CAMERA_ID_J18_3X_OV08A_MOON        210120440
#define SR_CAMERA_ID_K10A_WIDE_OV64B40        100136200
#define SR_CAMERA_ID_K10_WIDE_OV64B40         100136300
#define SR_CAMERA_ID_J2S_WIDE_HMX             100001100
#define SR_CAMERA_ID_C3J1_WIDE_GM1            100146400
#define SR_CAMERA_ID_K82_WIDE_OV13B10         100151300
#define SR_CAMERA_ID_K3S_WIDE_HM2             100161400
#define SR_CAMERA_ID_K8_WIDE_HMX              100001400
#define SR_CAMERA_ID_K8_TELE_5X_HI847         230191400
#define SR_CAMERA_ID_K81_WIDE_OV50C           100181500
#define SR_CAMERA_ID_K81_50M_OV50C            430181560
#define SR_CAMERA_ID_K81A_WIDE_OV13B10        100151500
#define SR_CAMERA_ID_K11T_WIDE_OV64B          100106300
#define SR_CAMERA_ID_K11R_WIDE_HM2            100166300
#define SR_CAMERA_ID_K11T_WIDE_CROP_OV64B     100106310
#define SR_CAMERA_ID_K11R_WIDE_CROP_HM2       100166310
#define SR_CAMERA_ID_K9D_WIDE_GW3             100071600
#define SR_CAMERA_ID_K19A_WIDE_JN1            100236500
#define SR_CAMERA_ID_K19B_WIDE_OV50C          100186500
#define SR_CAMERA_ID_K1_WIDE_GN2              100240400
#define SR_CAMERA_ID_K1_2X_CROP_GN2           200240410
#define SR_CAMERA_ID_K1_5X_IMX586             230050400
#define SR_CAMERA_ID_K1_10X_CROP_IMX586       240050410
#define SR_CAMERA_ID_K1_50M_GN2               430240460
#define SR_CAMERA_ID_K16_WIDE_HM2             100166600
#define SR_CAMERA_ID_L3_WIDE_IMX766           100501700
#define SR_CAMERA_ID_L3_WIDE_CROP_IMX766      100501710
#define SR_CAMERA_ID_K16A_WIDE_S5KJN1         100256100
#define SR_CAMERA_ID_K16A_WIDE_OV50C40        100266100
#define SR_CAMERA_ID_L3A_WIDE_IMX766          100501100
#define SR_CAMERA_ID_L2_WIDE_IMX707           100531701
#define SR_CAMERA_ID_L2_2X_JN1                200231700
#define SR_CAMERA_ID_L2_2X_JN1_MOON           200231740
#define SR_CAMERA_ID_L2B_WIDE_IMX707          100531701
#define SR_CAMERA_ID_L2B_2X_JN1               200231700
#define SR_CAMERA_ID_L2B_2X_JN1_MOON          200231740
#define SR_CAMERA_ID_L10A_WIDE_IMX582         100081101
#define SR_CAMERA_ID_L10_WIDE_IMX686          100061700

#ifdef __cplusplus
}
#endif

#endif /* __SR_COMMON_H__ */
