/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiPostProcServiceIntf.h"

#include <future>

namespace mialgo {

uint8_t bServiceDied = false;
sp<IMiPostProcService> gRemoteService = nullptr;
static const int32_t sBypassEnable =
    property_get_int32("persist.vendor.camera.mialgoengine.bypass", 0);

struct PostProcClientDeathRecipient : hidl_death_recipient
{
public:
    PostProcClientDeathRecipient() = default;
    virtual ~PostProcClientDeathRecipient() = default;
    virtual void serviceDied(uint64_t cookie, const wp<IBase> &who);
};

void PostProcClientDeathRecipient::serviceDied(uint64_t /*cookie*/, const wp<IBase> & /*who*/)
{
    MLOGW("Received from service PostProcClientDeathRecipient");
    bServiceDied = true;
    std::async([]() { return MiPostProcServiceIntf::getInstance()->serviceDied(); });
}

sp<PostProcClientDeathRecipient> mClientDeathReceipient = nullptr;
static sp<MiPostProcServiceIntf> gLocalService = nullptr;

MiPostProcServiceIntf::MiPostProcServiceIntf() : mState(UNLOADED), mEngineBypass(false) {}

MiPostProcServiceIntf::~MiPostProcServiceIntf() {}

sp<MiPostProcServiceIntf> MiPostProcServiceIntf::getInstance()
{
    if (gLocalService.get() == NULL) {
        gLocalService = new MiPostProcServiceIntf();
    }
    return gLocalService;
}

// init
CDKResult MiPostProcServiceIntf::init()
{
    MLOGI("E");

    Mutex::Autolock autoLock(mInitLock);

    if (mState != UNLOADED) {
        MLOGW("already initialized");
        return MIAS_OK;
    }

    gRemoteService = IMiPostProcService::tryGetService();
    if (gRemoteService == nullptr) {
        MLOGE("get IMiPostProcService faild");
        return MIAS_UNKNOWN_ERROR;
    }
    bServiceDied = false;

    MLOGI("connected to postprocservice %p", gRemoteService.get());
    mClientDeathReceipient = new PostProcClientDeathRecipient();
    gRemoteService->linkToDeath(mClientDeathReceipient, 0L);

    mState = INITIALIZED;
    mSessions.clear();

    MLOGI("initialize end");
    return MIAS_OK;
}

// deinit
CDKResult MiPostProcServiceIntf::deinit()
{
    MLOGI("E");

    Mutex::Autolock autoLock(mInitLock);

    if (mState == UNLOADED) {
        MLOGW("Alread deInited.......");
        return MIAS_OK;
    }

    // clear all created sessions. release all loaded nodes
    {
        Mutex::Autolock autoLock(mLock);
        for (size_t i = 0; i < mSessions.size(); i++) {
            mSessions[i]->destory();
            delete mSessions[i];
        }
    }

    mSessions.clear();
    mState = UNLOADED;

    gRemoteService->unlinkToDeath(mClientDeathReceipient);
    gRemoteService.clear();
    gRemoteService = NULL;
    mClientDeathReceipient.clear();
    mClientDeathReceipient = NULL;

    MLOGI("deinit end");
    return MIAS_OK;
}

CDKResult MiPostProcServiceIntf::setMiviInfo(std::string info)
{
    CDKResult result = MIAS_OK;
    if (mState == UNLOADED) {
        MLOGW("%d,create in unload state", mState);
        result = init();
    }

    if ((result == MIAS_OK) && (gRemoteService != NULL)) {
        Return<void> ret = gRemoteService->setCapabilities(info);
        if (!ret.isOk()) {
            MLOGE("setMiviInfo failed");
            result = MIAS_INVALID_PARAM;
        }
    }
    return result;
}

// postproc interface complete
SessionHandle MiPostProcServiceIntf::createSession(GraphDescriptor *gd, SessionOutput *output,
                                                   MiaSessionCb *cb)
{
    MLOGD("E GraphDescriptor:%p cb:%p", gd, cb);

    mEngineBypass = (sBypassEnable == 0 ? false : true);

    CDKResult ret = MIAS_OK;
    if (mState == UNLOADED) {
        MLOGW("%d,create in unload state", mState);
        if (!mEngineBypass) {
            init();
        } else {
            mState = INITIALIZED;
            mSessions.clear();
            gRemoteService = NULL;
            mClientDeathReceipient = NULL;
        }
    }

    // create and build a new session
    MiPostProcSessionIntf *session = new MiPostProcSessionIntf();
    if (!session) {
        MLOGE("allocate session fail");
        return NULL;
    }

    session->setEngineBypass(mEngineBypass);
    ret = session->initialize(gd, output, cb);
    if (ret != MIAS_OK) {
        MLOGE("setCallback fail with %d", ret);
        return NULL;
    }

    {
        Mutex::Autolock autoLock(mLock);
        mSessions.push_back(session);
    }
    mState = INWORKING;

    MLOGI("created session:%p numSessions:%zu", session, mSessions.size());

    return (SessionHandle)session;
}

CDKResult MiPostProcServiceIntf::processFrame(SessionHandle session, MiaFrame *frame)
{
    if (mState != INWORKING) {
        MLOGE("process frame in invalid state %d, session %p", mState, session);
        return MIAS_INVALID_CALL;
    }
    MiPostProcSessionIntf *s = (MiPostProcSessionIntf *)session;
    return s->processFrame(frame);
}

CDKResult MiPostProcServiceIntf::quickFinish(SessionHandle session, int64_t timeStamp,
                                             bool needImageResult)
{
    if (mState != INWORKING) {
        MLOGE("process frame in invalid state %d, session %p", mState, session);
        return MIAS_INVALID_CALL;
    }
    MiPostProcSessionIntf *s = (MiPostProcSessionIntf *)session;
    return s->quickFinish(timeStamp, needImageResult);
}

CDKResult MiPostProcServiceIntf::preProcessFrame(SessionHandle session, PreProcessInfo *params)
{
    if (mState != INWORKING) {
        MLOGE("preProcess frame in invalid state %d, session %p", mState, session);
        return MIAS_INVALID_CALL;
    }
    MiPostProcSessionIntf *s = (MiPostProcSessionIntf *)session;
    return s->preProcessFrame(params);
}

CDKResult MiPostProcServiceIntf::processFrameWithSync(SessionHandle session,
                                                      ProcessRequestInfo *processRequestInfo)
{
    MiPostProcSessionIntf *s = (MiPostProcSessionIntf *)session;
    return s->processFrameWithSync(processRequestInfo);
}

CDKResult MiPostProcServiceIntf::destroySession(SessionHandle session)
{
    size_t i = 0;

    MLOGD("E session:%p", session);
    if (mState != INWORKING) {
        MLOGW("destroy in invalid state %d", mState);
        return MIAS_INVALID_CALL;
    }

    {
        Mutex::Autolock autoLock(mLock);
        // check if it's a valid session
        for (i = 0; i < mSessions.size(); i++) {
            if ((void *)mSessions[i] == session)
                break;
        }
        if (i == mSessions.size()) {
            MLOGE("destroy an invalid session");
            return MIAS_INVALID_PARAM;
        }

        // flush and delete
        mSessions[i]->destory();
        delete mSessions[i];

        mSessions.erase(mSessions.begin() + i);
        if (mSessions.empty()) {
            mState = INITIALIZED;
        }
    }
    MLOGD("X session:%p", session);
    return MIAS_OK;
}

CDKResult MiPostProcServiceIntf::flush(SessionHandle session)
{
    MLOGD("E");
    MiPostProcSessionIntf *s = (MiPostProcSessionIntf *)session;
    auto iter = std::find(mSessions.begin(), mSessions.end(), session);
    if (iter != mSessions.end()) {
        s->flush();
    }
    return MIAS_OK;
}

void MiPostProcServiceIntf::serviceDied()
{
    for (size_t i = 0; i < mSessions.size(); i++) {
        mSessions[i]->setProcessStatus();
    }
    MLOGI("numSessions:%zu", mSessions.size());
    if (!mSessions.empty()) {
        MiPostProcSessionIntf *session = mSessions.back();
        session->onSessionCallback(MIA_SERVICE_DIED, 0, NULL);
    }

    for (size_t i = 0; i < mSessions.size(); i++) {
        flush(mSessions[i]);
    }

    deinit();
}

} // namespace mialgo
