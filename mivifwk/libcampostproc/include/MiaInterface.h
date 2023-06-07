/*
 * Copyright (c) 2018. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

/*
 * MIA stands for MI Imaging Algorithm
 * This file defines the interfaces that MIA provides
 */

#ifndef __MI_INTERFACE__
#define __MI_INTERFACE__

#include <stdint.h>
#include <string.h>

#include <map>
#include <string>

#include "MiaFmt.h"

namespace mialgo {

static const int32_t MiFormatsMaxPlanes = 3;
static const int32_t MiMaxImagesInBatch = 8;
static const int32_t MiMaxInputNum = 20;

typedef struct FrameInfo
{
    uint32_t width;   ///< Width of the image in pixels.
    uint32_t height;  ///< Height of the image in pixels.
    MiaFormat format; ///< Format of the image.
} MiImageFormat;

typedef struct _MiImage
{
    uint32_t size;                      ///< Size of the structure
    int fd[MiFormatsMaxPlanes];         ///< File descriptor, used by node to map the buffer to FD
    uint8_t *pAddr[MiFormatsMaxPlanes]; ///< Starting virtual address of the allocated buffer.
} MiImage;

/*Define the image format space*/
typedef struct _MiImageList
{
    MiImageFormat format;
    int32_t planeStride;
    int32_t sliceHeight;
    uint32_t numberOfPlanes;
    size_t planeSize[MiFormatsMaxPlanes];   ///< Per plane size.
    uint32_t imageCount;                    ///< The count of image in the pImageList
    MiImage pImageList[MiMaxImagesInBatch]; ///< Array of ChiImage
    void *nativeHandle;
} MiImageList_t, *MiImageList_p;

struct MiaFrame;
/*
 * Callback function for notify algo framework that this image that
 * allocated by algo can be recycled.
 * This callback is provided by algo framework when notify output buffer
 * As a input frame from app, it will be NULL.
 */
class MiaFrameCb
{
public:
    virtual ~MiaFrameCb(){};
    virtual void release(MiaFrame *frame) = 0;
};

#define CLEAR(x) memset(&(x), 0, sizeof(x))

struct MiaFrame
{
    MiaFrame() : stream_index(0), frame_number(0), callback(NULL), requestId(0)
    {
        CLEAR(mibuffer);
        CLEAR(result_metadata);
        CLEAR(request_metadata);
        CLEAR(phycam_metadata);
    };
    uint32_t stream_index;
    uint64_t frame_number;
    uint32_t requestId;
    MiImageList_t mibuffer;
    void *result_metadata;  // HAL result camera metadata
    void *request_metadata; // App request camera metadata
    void *phycam_metadata;  // HAL physical result camera metadata
    MiaFrameCb *callback;
};

// the CDKResult is a extension set of android status_t
typedef int32_t CDKResult;
enum {
    MIAS_OK = 0,
    MIAS_UNKNOWN_ERROR = (-2147483647 - 1), // int32_t_MIN value
    MIAS_INVALID_PARAM,
    MIAS_NO_MEM,
    MIAS_MAP_FAILED,
    MIAS_UNMAP_FAILED,
    MIAS_OPEN_FAILED,
    MIAS_INVALID_CALL,
    MIAS_UNABLE_LOAD
};

// --- exposed functions
#ifdef __cplusplus
extern "C" {

// private types
typedef void *SessionHandle;

enum ResultType {
    MIA_SERVICE_DIED,
    MIA_RESULT_METADATA,
    MIA_ANCHOR_CALLBACK = 10,
    MIA_ABNORMAL_CALLBACK = 20
};

/*
 * Cast the data to the specific output data for this node like
 * NODE:  NDTP_MI_GENDERAGE the data would be the pointer to
 *  struct OutputMetadataFaceAnalyze defined in MI_AlgoParam
 */
struct MiaResult
{
    ResultType type;
    uint32_t resultId;
    int64_t timeStamp;
    std::map<std::string, std::string> metaData;
};

union MiaMsgData {
    //* the frame pointer's lifetime is when app call to release this frame
    MiaFrame *frame;
    //* the result pointer's lifetime is just during this callback function
    MiaResult *result;
};

enum SessionOutputType { SESSION_OUT_BUFFERS, SESSION_OUT_SURFACE };

enum SurfaceType {
    SURFACE_TYPE_SNAPSHOT_MAIN = 0,
    SURFACE_TYPE_SNAPSHOT_AUX,
    SURFACE_TYPE_SNAPSHOT_DEPTH,
    SURFACE_TYPE_VIEWFINDER,
    SURFACE_TYPE_COUNT
};

/*
 * App should describe its expected outputs in creating session
 * if SessionOutputType is buffer, the surfaces array can be NULL
 * but FrameInfo must be filled correctly.
 * Otherwise, App should fill the surfaces array in correct place
 * it needed and ensure the surface lifttime over the session
 */
struct SessionOutput
{
    SessionOutput() : type(SESSION_OUT_BUFFERS)
    {
        fInfo = {0, 0, CAM_FORMAT_JPEG};
        CLEAR(surfaces);
    }
    SessionOutputType type;
    FrameInfo fInfo;
    void *surfaces[SURFACE_TYPE_COUNT];
};

struct GraphDescriptor
{
    GraphDescriptor() : operation_mode(0), is_snapshot(false), camera_mode(0), num_streams(0){};
    uint32_t operation_mode;
    uint32_t num_streams; // e.g. 2 stream needed for dual camera boken case
    // bool     is_portrait;
    bool is_snapshot;
    // bool     is_frontcamera;
    uint32_t camera_mode;
    // may need a metadata
};

/*
 * Callback function for instant message notification and buffer callback
 * Includes:
 *   - pipeline output frame, metadata is carrying along with it
 *   - pipeline output specific messages such as if age_gender node enabled
 *     it will report the age and gender data.
 *   - pipeline input frame release notification.
 *   - pipeline instant notifications like:
 *       * error in processing frame N
 *       * timeout in processing frame N
 *       * buffer overflow
 */
enum MsgType {
    MIA_MSG_PO_RESULT,       // pipeline output result, cast the msgData to MiaMsgData.result
    MIA_MSG_PP_ERROR,        // pipeline notifications, cast the msgData to uint32_t (node ID)
    MIA_MSG_PP_TIMEOUT,      // pipeline notifications, cast the msgData to uint32_t
    MIA_MSG_PP_BUFOVERFLOW,  // pipeline notifications, cast the msgData to uint32_t
    MIA_MSG_SERIVCE_DIED,    // service notifications, service died
    MIA_MSG_ABNORMAL_PROCESS // MIVI exception handling notification
};

class MiaSessionCb
{
public:
    virtual ~MiaSessionCb(){};
    virtual void onSessionResultCb(uint32_t msgType, void *data) = 0;
};

enum ProcessAlgoType {
    MIA_PROCESS_SR,
    MIA_PROCESS_CLEARSHOT,
    MIA_PROCESS_HHT,
    MIA_PROCESS_MAX,
};

typedef struct _ProcessRequestInfo
{
    uint64_t frameNum;                          //< Frame number for current request
    ProcessAlgoType algoType;                   //< process algo type
    void *meta[MiMaxInputNum];                  //< native CameraMetadata*
    uint32_t inputNum;                          //< Number of input buffer handles
    MiImageList_p phInputBuffer[MiMaxInputNum]; //< Pointer to array of input buffer handles
    uint32_t outputNum;                         //< Number of output buffer
    MiImageList_p phOutputBuffer[1];            //< Pointer to array of output buffer handles
    int32_t baseFrameIndex;                     //< algo output
} ProcessRequestInfo;

struct PreProcessInfo
{
    int32_t cameraId;
    int32_t width;
    int32_t height;
    int32_t format;
    void *meta;
};

/*
 * Init and Deinit is the start and end function call before or finish using MIA
 */
CDKResult miaInit(void *params);
CDKResult miaDeinit();
CDKResult miaSetMiviInfo(std::string info);

/*
 * Session management functions, Session and Graph is 1:1
 */
SessionHandle miaCreateSession(GraphDescriptor *gd, SessionOutput *output, MiaSessionCb *cb);
CDKResult miaDestroySession(SessionHandle session);

/*
 * Process a task, the task content contains all needed thing in this
 * time of processing. Input buffers, output buffer, and settings.
 * Result will be delivered by callback when processing done.
 * It's an async call, app should make sure the frame life time until we
 * Callback the output of this frame
 */
CDKResult miaProcessFrame(SessionHandle session, MiaFrame *frame);

CDKResult miaPreProcess(SessionHandle session, PreProcessInfo *params);

/*
 * Process a task, the task content contains all needed thing in this
 * time of processing. Input buffers, output buffer, and settings.
 * It's an sync call
 */
CDKResult miaProcessFrameWithSync(SessionHandle session, ProcessRequestInfo *pProcessRequestInfo);

/*
 * flush all the pending tasks, return pending frames immediately
 */
CDKResult miaFlush(SessionHandle session);

void *miOpenLib(void);

// for dlsys
typedef struct mia_interface
{
    CDKResult (*init)(void *params);
    CDKResult (*deinit)();
    CDKResult (*setMiViInfo)(std::string info);
    SessionHandle (*createSession)(GraphDescriptor *gd, SessionOutput *output, MiaSessionCb *cb);
    CDKResult (*destroySession)(SessionHandle session);
    CDKResult (*processFrame)(SessionHandle session, MiaFrame *frame);
    CDKResult (*processFrameWithSync)(SessionHandle session,
                                      ProcessRequestInfo *pProcessRequestInfo);
    CDKResult (*flush)(SessionHandle session);
    CDKResult (*quickFinish)(SessionHandle session, int64_t timeStamp, bool needImageResult);
} MiaInterface;

void *GetExtInterafce(void);

int GetExtVersion();

struct MiaExtInterface
{
    CDKResult (*preProcess)(SessionHandle session, PreProcessInfo *params);
};

} // extern "C"
#endif //__cplusplus

} // namespace mialgo
#endif /*__MI_INTERFACE__*/
