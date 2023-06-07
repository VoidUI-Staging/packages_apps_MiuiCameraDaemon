/*
 * Copyright (c) 2018. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef __MI_POSTPROCSERVICEINTF__
#define __MI_POSTPROCSERVICEINTF__

#include "MiPostProcSessionIntf.h"

namespace mialgo {

typedef enum { UNLOADED = 0, INITIALIZED = 1, INWORKING = 2 } MiServiceState;

class MiPostProcServiceIntf : virtual public RefBase
{
public:
    ~MiPostProcServiceIntf();
    static sp<MiPostProcServiceIntf> getInstance();

    CDKResult init();
    CDKResult deinit();
    CDKResult setMiviInfo(std::string info);
    SessionHandle createSession(GraphDescriptor *gd, SessionOutput *output, MiaSessionCb *cb);
    CDKResult destroySession(SessionHandle session);
    CDKResult processFrame(SessionHandle session, MiaFrame *frame);
    CDKResult preProcessFrame(SessionHandle session, PreProcessInfo *params);
    CDKResult processFrameWithSync(SessionHandle session, ProcessRequestInfo *processRequestInfo);
    CDKResult flush(SessionHandle session);
    CDKResult quickFinish(SessionHandle session, int64_t timeStamp, bool needImageResult);
    void serviceDied();

private:
    MiPostProcServiceIntf();
    // basic fuctions

private:
    MiServiceState mState;
    // session list
    std::vector<MiPostProcSessionIntf *> mSessions;
    mutable Mutex mLock;
    Mutex mInitLock;

    bool mEngineBypass;
};

} // namespace mialgo
#endif /* __MI_POSTPROCSERVICEINTF__ */
