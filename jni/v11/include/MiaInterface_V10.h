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

#ifndef __MI_INTERFACE_V10__
#define __MI_INTERFACE_V10__

#include "MiaInterface.h"

namespace mialgo {

// --- exposed functions
#ifdef __cplusplus
extern "C" {

enum ResultType_V10 { MIA_RESULT_FLAW, MIA_RESULT_RAWSN};
enum MsgType_V10 {
    MIA_MSG_PO_FRAME_V10, // pipeline output frame, cast the msgData to MiaMsgData.frame
    MIA_MSG_PO_RESULT_V10,// pipeline output result, cast the msgData to MiaMsgData.result
    MIA_MSG_PI_FRAME_V10, // pipeline input  frame release, cast the msgData to MiaMsgData.frame
    MIA_MSG_PP_ERROR_V10, // pipeline notifications, cast the msgData to uint32_t (node ID)
    MIA_MSG_PP_TIMEOUT_V10, // pipeline notifications, cast the msgData to uint32_t
    MIA_MSG_PP_BUFOVERFLOW_V10 // pipeline notifications, cast the msgData to uint32_t
};

typedef struct _FlawResult {
    bool face_blur;
    bool eye_close;
    bool occlusion;
    int person_cls;
    float prob_eyeclose;
    float prob_occlusion;
    float prob_faceblur;
    int flaw_result;
} FlawResult;

struct MiaResultV10 {
    ResultType type;
    uint32_t   resultId;
    union {
        FlawResult *flawData;
    };
};

union MiaMsgDataV10 {
    //* the frame pointer's lifetime is when app call to release this frame
    MiaFrame *frame;
    //* the result pointer's lifetime is just during this callback function
    MiaResultV10 *result;
};

class MiaSessionCb_V10 {
public:
    virtual ~MiaSessionCb_V10() {};
    virtual void onSessionStatusChanged(uint32_t msg, void* msgData, void* priv) = 0;
    //void  *priv; // pass by this if app need carry any private data in callback
};

/*
 * libcamera_metadata is not allowed to be used by vendor sphal library.
 * MIA use this way (the callback) to implemente the meta parsing functionality
 */
struct MiaMetaUtilCb {
    // for meatadata parsing
    int   (*getVTagValue)(void* mdata, const char *tagname, void **ppdata);
    int   (*getTagValue)(void* mdata, uint32_t tagID, void **ppdata);
    int   (*setVTagValue)(void* mdata, const char *tagname, void *data, size_t count);
    int   (*setTagValue)(void* mdata, uint32_t tagID, void *data, size_t count);
    void* (*allocateCopyMetadata)(void* src);
    void* (*allocateCopyInterMetadata)(void* src);
    void  (*freeMetadata)(void* meta);
    int (*getVTagValueExt)(void* mdata, const char *tagname, void **ppdata, size_t& count);
    int (*getTagValueExt)(void* mdata, uint32_t tagID, void **ppdata, size_t& count);
};

typedef struct MiaInitParams {
    int        calFilePathLen;
    char      *calFilePath;    ///< bokeh calibration data file path
    void      *reserved;
} MiaInitParams_t, *MiaInitParams_p;

typedef struct _ProcessRequestInfo_V1 {
    uint64_t frameNum;                          //< Frame number for current request
    ProcessAlgoType algoType;                   //< process algo type
    void *meta;                                 //< native CameraMetadata*
    uint32_t inputNum;                          //< Number of input buffer handles
    MiImageList_p phInputBuffer[10]; //< Pointer to array of input buffer handles
    uint32_t outputNum;                         //< Number of output buffer
    MiImageList_p phOutputBuffer[1];            //< Pointer to array of output buffer handles
    int32_t baseFrameIndex;                     //< algo output
} ProcessRequestInfo_V1;

typedef struct mia_interface_v10
{
    CDKResult        (*Init)(MiaMetaUtilCb cb, MiaInitParams_p params);
    CDKResult        (*Deinit)();
    SessionHandle    (*CreateSession)(GraphDescriptor *gd, SessionOutput *output,
                                      MiaSessionCb_V10 *cb);
    CDKResult        (*DestroySession)(SessionHandle session);
    CDKResult        (*ProcessFrame)(SessionHandle session, MiaFrame *frame);
    CDKResult        (*ProcessFrameWithSync)(SessionHandle session, void *pProcessRequestInfo);
    CDKResult        (*Flush)(SessionHandle session);
} MiaInterface_v10;

} // extern "C"
#endif //__cplusplus



}// namespace mialgo

#endif /*__MI_INTERFACE__*/
