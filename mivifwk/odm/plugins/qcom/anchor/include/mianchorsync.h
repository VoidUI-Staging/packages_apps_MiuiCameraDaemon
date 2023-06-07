#ifndef __MIANCHORSYNC_H
#define __MIANCHORSYNC_H

#include <MiaPluginWraper.h>
#include <time.h>

#include <condition_variable>
#include <mutex>
#include <thread>

#include "MiaPluginUtils.h"
#include "mialgo/mialgo_log.h"
#include "mialgo/mialgo_mat.h"
#include "mialgo/mialgo_raw.h"
#include "mialgo/mialgo_rfs.h"
#include "mialgo/mialgo_utils.h"
#include "mialgo/mialgo_utils_cdsp.h"

struct MiFaceRect
{
    uint32_t x;      ///< x coordinate for top-left point
    uint32_t y;      ///< y coordinate for top-left point
    uint32_t width;  ///< width
    uint32_t height; ///< height
};
struct MiFaceInfo
{
    uint32_t faceNum;
    MiFaceRect faceRectInfo;
};
struct MiCropRect
{
    uint32_t x;      ///< x coordinate for top-left point
    uint32_t y;      ///< y coordinate for top-left point
    uint32_t width;  ///< width
    uint32_t height; ///< height
};
struct MiImageInfo
{
    uint32_t width;  ///< Width of the image in pixels.
    uint32_t height; ///< Height of the image in pixels.
    uint32_t format; ///< Format of the image.
    int frameIndex;
    void *addr;
    int fd;
};

// Anchor ParamInfo
struct AnchorParamInfo
{
    int inputNum;
    std::vector<ImageParams> *pBufferInfo;
    unsigned int motionEnable : 1;
    unsigned int lumaEnable : 1;
    unsigned int unused : 30;

    float aecLux;
    MiCropRect cropRegion;
    MiFaceInfo faceInfo;
    bool isQuadCFASensor;
    int32_t bayerPattern;

    std::vector<float> luma;
    std::vector<int> sharpness;
    std::vector<int> groupId;
    std::vector<int> outputOrder;
};

enum STATE {
    Uninit = 0,
    Initing,
    Idle,
    Runing,
};

class SharpnessMiAlgo
{
public:
    static SharpnessMiAlgo *GetInstance();
    void AnchorInitAlgoLib();
    int AddRef();
    int ReleaseRef();
    int Calculate(AnchorParamInfo *pAnchorParamInfo);
    void Preprocess(AnchorParamInfo *pAnchorParamInfo);

private:
    SharpnessMiAlgo();
    ~SharpnessMiAlgo(){};
    STATE mState;
    int mRefCount;
    std::mutex mAlgoMutex;
    std::condition_variable mWaitAlgoCond;

    MialgoEngine mHandle;
    MialgoRFSConfig mConfig;
    MI_S32 mSizes[3];
    MI_S32 mAligned[3];
    int mBufSize;
    MI_S32 mFlag;

    uint32_t AlignGeneric32(uint32_t operand, unsigned int alignment);
    void roiCropParamUpdata(const AnchorParamInfo *pAnchorParamInfo);
    void faceParamUpdata(const AnchorParamInfo *pAnchorParamInfo, const int &i);
};
class LumaScreen
{
public:
    std::vector<int> operator()(std::vector<float> &luma);
    void calculateVarAndIsolatedPoint(std::vector<float> &luma, std::vector<int> &mask, float &std,
                                      int &isolatedIndex);
    LumaScreen()
    {
        minFrame = 3;
        char value[PROPERTY_VALUE_MAX];
        property_get("persist.vendor.camera.lumathreshold1", value, "1.5");
        VarThreshold = std::stof(value);
    };

private:
    int minFrame;
    float VarThreshold;
};

#endif // __MIANCHORSYNC_H
