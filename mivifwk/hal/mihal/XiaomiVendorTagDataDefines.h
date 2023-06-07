#ifndef __XIAOMI_TAGS_DATA_DEFINE_H__
#define __XIAOMI_TAGS_DATA_DEFINE_H__

namespace mihal {

#define METADATA_VALUE_NUMBER_MAX 16
#define MAX_REAR_REQUEST_NUM      16
#define MAX_FRONT_REQUEST_NUM     8

static const int MaxUIRelatedMetaCnt = 8;
static const int MaxTagNameLen = 128;

typedef char UIRelatedTagInfo[MaxTagNameLen];

typedef char ASDExifInfo[1024];

/**
 *请求的帧数和每张图的ev值(人像/普通拍照下有传)
 */
typedef struct
{
    int32_t sum;
    int32_t ev[METADATA_VALUE_NUMBER_MAX];
} SuperNightChecker;

/**
 * 夜景Trigger（人像/普通拍照一致)
 * 对应tag: xiaomi.ai.misd.NonSemanticScene
 * 传的是一个AiMisdScene[2]数组
 * 当type字段等于3时，其所对应的value是夜景检测的结果。
 * trigger的结果分两部分：
 * 1.第0位到第7位：app是否需要显示超夜图标（旧项目使用，现在已遗弃）
 * 2.从第8位到第31位：是否trigger夜景算法。0：不触发，1：手持虹软SE，2：手持夜枭
 *（以前就只有虹软SE，所以 0：不触发，1：触发。现在有所改动，请注意）
 **
 * 脚架检测
 * 对应tag: xiaomi.ai.misd.StateScene
 * 传的是一个AiMisdScene结构体
 * type应该为4，value值 0：不认为手机在脚架上，1：认为手机在脚架上
 */
typedef struct
{
    int32_t type;
    int32_t value;
} AiMisdScene;

typedef struct
{
    AiMisdScene aiMisdScene[2];
} AiMisdScenes;

/**
 * 夜枭专用脚架检测
 * 对应的tag: xiaomi.ai.misd.ELLTripod
 * 一个int32的整数
 * type应该为4，value值 0：不认为手机在脚架上，1：认为手机在脚架上
 * 0：不认为手机在脚架上
 * 1：认为手机在脚架上
 */

/**
 * 夜景拍摄倒计时(人像/普通拍照下有传)
 * listSize表示列表中有效元素的个数。
 * list为有listSize个有效元素的SuperNightCaptureExpTimeEntry型数组，最大长度为16
 * SuperNightCaptureExpTimeEntry中的type表示不同的超夜算法，1：虹软SE+lls，2：夜枭+lls，3：mfnr+lls
 * expTime字段表示如果当前帧拍摄夜景需要多久的时间，单位纳秒
 */
typedef struct
{
    uint64_t type;
    uint64_t expTime;
} SuperNightCaptureExpTimeEntry;

typedef struct
{
    uint32_t listSize;
    SuperNightCaptureExpTimeEntry list[METADATA_VALUE_NUMBER_MAX];
} SuperNightCaptureExpTimeList;

struct ImageRect
{
    int32_t left;
    int32_t top;
    int32_t width;
    int32_t height;
};

struct ImageSize
{
    uint32_t width;
    uint32_t height;
    // NOTE: if matchedDecimalBitsis is set to n, then for isSameRatio() to return true, the decimal
    // part of two ratios must match at least n numbers
    bool isSameRatio(float ratio, int matchedDecimalNumbers = 1) const;
};

struct IdealRawInfo
{
    int32_t isIdealRaw;   ///< if ideal raw ouput is requested set to TRUE otherwise FALSE
    int32_t idealRawType; ///< 0 -- MIPI Raw, 1 -- IFE IDEAL RAW, 2 -- BPS IDEAL RAW
};

enum SuperNightTriggerResultType {
    NoneType = 0,
    NormalSE,
    NormalELLC,
    NormalSNSC,
    NormalSNSCNonZSL,
    NormalSNSCStg,
    InvalidTriggerResultType
};

enum MiviSuperNightMode {
    MIVISuperNightNone = 0,
    MIVISuperNightHand = 1,
    MIVISuperNightTripod = 2,
    MIVISuperNightHandExt = 3,
    MIVISuperNightTripodExt = 4,
    MIVISuperLowLightHand = 5,
    MIVISuperLowLightTripod = 6,
    MIVISuperLowLightHandExt = 7,
    MIVISuperLowLightTripodExt = 8,
    MIVINightMotionHand = 9,
    MIVINightMotionHandNonZSL = 10,
    MIVINightMotionHandStg = 11,
    MIVISuperNightManuallyOff = 12,
};

typedef enum {
    Begin = 0,
    PortraitFlag,
    BacklitFlag,
    NightSceneSuperNight,
    OnTripod,
    LowLightMode,
    Dirty,
    MacroDistance,
    HDRDetect,
    WideAngle,
    SuperNightSceneSNSC,
    End
} ASDMetadataSemantic;

typedef struct
{
    // hdr meta
    uint8_t *data;
    uint32_t size;
} SnapshotReqInfo;

typedef struct
{
    uint32_t count;
    UIRelatedTagInfo tagInfo[MaxUIRelatedMetaCnt];
} UIRelatedMetas;

typedef struct
{
    size_t size; ///< Size in bytes of the debug data buffer
    void *pData; ///< Pointer to the debug data buffer
} DebugData;

} // namespace mihal

#endif // __XIAOMI_TAGS_DATA_DEFINE_H__
