#ifndef __MI_NODEPortraitRepair__
#define __MI_NODEPortraitRepair__
#include <MiaPluginWraper.h>
#include <VendorMetadataParser.h>
#include <pthread.h>
#include <system/camera_metadata.h>
// clang-format off
#include "chivendortag.h"
#include "chioemvendortag.h"
// clang-format on
#include "arcsoft_utils.h"

#include "mutex"
#include "stm_portrait_repair_api.h"

namespace mialgo2 {
typedef struct ChiRectEXT
{
    int left;   ///< x coordinate for top-left point
    int top;    ///< y coordinate for top-left point
    int right;  ///< x coordinate for bottom-right point
    int bottom; ///< y coordinate for bottom-right point
} CHIRECTEXT;

typedef struct
{
    int faceNumber;
    ChiRectEXT chiFaceInfo[10];
} FaceResult;

/// @brief Structure containing width and height integer values, along with a start offset
typedef struct ChiRect
{
    uint32_t left;   ///< x coordinate for top-left point
    uint32_t top;    ///< y coordinate for top-left point
    uint32_t width;  ///< width
    uint32_t height; ///< height
} CHIRECT;

class PortraitRepair : public PluginWraper
{
public:
    static std::string getPluginName() { return "com.xiaomi.plugin.portraitrepair"; }
    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface);
    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo);
    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo);
    virtual int flushRequest(FlushRequestInfo *flushRequestInfo);
    virtual void destroy();
    virtual bool isEnabled(MiaParams settings);
    virtual int preProcess(PreProcessInfo preProcessInfo);
    stm_result_t mPortraitRepairResult;

private:
    ~PortraitRepair();
    virtual bool isPreProcessed() { return false; };
    virtual bool supportPreProcess() { return true; };
    int CopyBuffer(stm_image_t *stm_output, stm_image_t *stm_input);
    int CopyYuvBuffer(yuv_image_t *stm_output, yuv_image_t *stm_input);
    void DumpYuvBuffer(const char *fileName, struct MiImageBuffer *miBuf);
    bool IsFrontCamera();
    void loadConfigParams(uint8_t stylizationType);
    void getFaceInfo(_stm_rect *faceInfo, CHIRECT *cropRegion, camera_metadata_t *metaData,
                     camera_metadata_t *physicalMeta, int *faceNum, ChiRectEXT *afRect);
    void drawFaceRect(MiImageBuffer *image, _stm_rect *faceInfo, int facenum, ChiRectEXT afRect);
    void prepareImage(MiImageBuffer *image, ASVLOFFSCREEN &stImage);

    bool isProcessFace(_stm_rect *faceInfo, int faceNum, int *processFaceNum, ChiRectEXT afRect);
    bool isFaceFocusFrameOverlap(_stm_rect *faceInfo, ChiRectEXT afRect, int faceNum);
    bool isFaceFocusFrameOverlap(stm_rect_t faceInfo, ChiRectEXT afRect);

    bool mEnableStatus;
    bool mPortraitEnable;
    bool mIsDrawFaceRect;
    MiaNodeInterface mNodeInterface;
    int mQualityDebug;
    int mFrameworkCameraId;
    int mOperationMode;
    int mWidth;
    int mHeight;
    pthread_t m_thread;
};
PLUMA_INHERIT_PROVIDER(PortraitRepair, PluginWraper);
PLUMA_CONNECTOR
bool connect(ProviderManager &host)
{
    host.add(new PortraitRepairProvider());
    return true;
}
} // namespace mialgo2
#endif //__MI_NODEMIBOKEH__
