#ifndef _PLUGINWRAPER_H_
#define _PLUGINWRAPER_H_

#include <cutils/native_handle.h>
#include <system/camera_metadata.h>

#include "MiaPerfManager.h"
#include "MiaPluginUtils.h"
#include "MiaProvider.h"
#include "pluma.h"

using namespace mialgo2;

union Flags {
    struct
    {
        uint32_t isInplace : 1;    ///< Is inplace node i.e. output buffer same as input buffer
        uint32_t isRealTime : 1;   ///< Is this node part of a real time pipeline
        uint32_t isBypassable : 1; ///< Is this node bypassable
        uint32_t reserved : 25;    ///< Reserved
    };

    uint32_t value;
};

struct ImageFormat
{
    uint32_t format;   ///< format
    uint32_t width;    ///< width
    uint32_t height;   ///< height
    uint32_t cameraId; ///< physical cameraId
};

typedef struct
{
    camera_metadata_t *metadata;
    uint32_t cameraId;
    ImageFormat format;
} MiaParams;

typedef struct CreateInfo
{
    Flags nodeFlags;          ///< Node flags
    std::string nodeInstance; ///< Node instance
    int nodePropertyId;
    uint32_t frameworkCameraId; ///< Framework Camera Id
    uint32_t operationMode;
    ImageFormat inputFormat;  ///< Input buffer params
    ImageFormat outputFormat; ///< Output buffer params
    uint32_t logicalCameraId;
    int32_t roleId;
} CreateInfo;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief This structure used to provide Handle params in session request
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ImageParams
{
    ImageFormat format;
    uint32_t planeStride;               // stride
    uint32_t sliceheight;               // scanline
    camera_metadata_t *pMetadata;       ///< logical camera metadata
    camera_metadata_t *pPhyCamMetadata; ///< physical camera metadata
    uint32_t numberOfPlanes;
    int fd[mialgo2::MaxPlanes];           ///< File descriptor, used by node to map the buffer to FD
    uint8_t *pAddr[mialgo2::MaxPlanes];   ///< Starting virtual address of the allocated buffer.
    const native_handle_t *pNativeHandle; ///< Native handle
    uint64_t bufferSize;
    int32_t bufferType;
    uint32_t cameraId;
    int32_t roleId;
    int32_t portId;

    void *reserved;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief This enumerates encoder status
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum ProcessRetStatus {
    PROCSUCCESS = 0,  ///< Success
    PROCFAILED = 1,   ///< Postproc Failed
    PROCBADSTATE = 2, ///< Postproc in bad state
};

struct PreMetaProxy
{
    camera_metadata_t *mPreMeta;
    PreMetaProxy(camera_metadata_t *preMeta) { mPreMeta = preMeta; }
    ~PreMetaProxy()
    {
        if (NULL != mPreMeta) {
            free(mPreMeta);
            mPreMeta = NULL;
        }
    }
};

struct PreProcessInfo
{
    std::map<uint32_t, ImageFormat> inputFormats;  ///< PortId <--> InputFormat map
    std::map<uint32_t, ImageFormat> outputFormats; ///< PortId <--> InputFormat map
    std::shared_ptr<PreMetaProxy> metadata;
};

struct ProcessRequestInfo
{
    // NOTE: PortId is consistent with JSON file configuration
    // PortId is Link->DstPort->PortId of current node
    std::map<uint32_t, std::vector<ImageParams>> inputBuffersMap; ///< PortId <-> Inputbuffers
    // PortId is Link->SrcPort->PortId of current node
    std::map<uint32_t, std::vector<ImageParams>> outputBuffersMap; ///< PortId <-> Outputbuffers
    uint32_t frameNum;                                             ///< Frame number requested
    int64_t timeStamp;
    bool isFirstFrame;
    int maxConcurrentNum;
    int runningTaskNum;
};

struct ProcessRequestInfoV2
{
    std::map<uint32_t, std::vector<ImageParams>> inputBuffersMap; ///< PortId <--> Inputbuffers map
    uint32_t frameNum;                                            ///< Frame number requested
    int64_t timeStamp;
    int maxConcurrentNum;
    int runningTaskNum;
};

struct FlushRequestInfo
{
    uint64_t frameNum; ///< Frame number for current request
    bool isForced;
};

typedef void (*PFNNODESETMETADATA)(void *owner, uint32_t frameNum, int64_t timeStamp,
                                   std::string &result);

typedef void (*PFNNODESETOUTPUTFORMAT)(void *owner, ImageFormat imageFormat);

typedef void (*PFNNODESETRESULTBUFFER)(void *owner, uint32_t frameNum);

typedef void (*PFNNODERELEASEABLEUNNEEDBUFFER)(void *owner, int portId, int releaseBufferNumber,
                                               PluginReleaseBuffer releaseBufferMode);

struct MiaNodeInterface
{
    void *owner;
    PFNNODESETMETADATA pSetResultMetadata;
    PFNNODESETOUTPUTFORMAT pSetOutputFormat;
    PFNNODESETRESULTBUFFER pSetResultBuffer;
    PFNNODERELEASEABLEUNNEEDBUFFER pReleaseableUnneedBuffer;
};

template <typename... Args>
void WriteLog(const char *level, const char *format, Args &&... args)
{
    Log::writeLogToFile(Mia2LogGroupAlgorithm, level, format, std::forward<Args>(args)...);
}

/////////////////////////////////////////////////////////
/// Plugin Interface
/// That's the kind of objects that plugins will provide.
/////////////////////////////////////////////////////////
class PluginWraper
{
public:
    static std::string getPluginName() { return "It's a base plugin"; };

    virtual int initialize(CreateInfo *createInfo, MiaNodeInterface nodeInterface) = 0;

    // Do something before the main algorithm process if needed
    virtual int preProcess(PreProcessInfo preProcessInfo)
    {
        (void)preProcessInfo;

        return 0;
    }

    bool needPreProcess(MiaParams settings)
    {
        return supportPreProcess() && !isPreProcessed() && isEnabled(settings);
    }

    virtual ProcessRetStatus processRequest(ProcessRequestInfo *processRequestInfo) = 0;

    virtual ProcessRetStatus processRequest(ProcessRequestInfoV2 *processRequestInfo) = 0;

    // Do something after the main algorithm process if needed
    virtual int postProcess() { return 0; }

    virtual int flushRequest(FlushRequestInfo *flushRequestInfo) = 0;

    virtual void destroy() = 0;

    virtual bool isEnabled(MiaParams settings) = 0;

    virtual void reset(){};

    virtual ~PluginWraper() {}
    // (...)

protected:
    virtual void convertImageParams(ImageParams &imageParams, mialgo2::MiImageBuffer &imageBuffer)
    {
        imageBuffer.format = imageParams.format.format;
        imageBuffer.width = imageParams.format.width;
        imageBuffer.height = imageParams.format.height;
        imageBuffer.stride = imageParams.planeStride;
        imageBuffer.scanline = imageParams.sliceheight;
        imageBuffer.numberOfPlanes = imageParams.numberOfPlanes;
        for (int i = 0; i < imageParams.numberOfPlanes; i++) {
            imageBuffer.plane[i] = imageParams.pAddr[i];
            imageBuffer.fd[i] = imageParams.fd[i];
        }
    };

private:
    virtual bool isPreProcessed() { return false; }
    virtual bool supportPreProcess() { return false; }
};

PLUMA_PROVIDER_HEADER(PluginWraper);

#endif // _PLUGINWRAPER_H_
