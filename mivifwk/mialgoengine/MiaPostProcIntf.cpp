////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2020 Xiaomi Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Xiaomi Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "MiaPostProcIntf.h"

#include <list>
#include <mutex>

#include "MiaOfflineSession.h"

namespace mialgo2 {

std::mutex allSessionsMutex;
std::vector<MiaOfflineSession *> allSessions;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MiaPostProc_Create
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void *MiaPostProc_Create(MiaCreateParams *params)
{
    MLOGI(Mia2LogGroupCore, "E");
    MiaOfflineSession *session = MiaOfflineSession::create(params);
    if (NULL == session) {
        MLOGE(Mia2LogGroupCore, "MiaOfflineSession create  failed");
        return NULL;
    }
    MLOGI(Mia2LogGroupCore, "X session: %p", session);
    std::lock_guard<std::mutex> locker(allSessionsMutex);
    allSessions.push_back(session);

    return reinterpret_cast<void *>(session);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MiaPostProc_PreProcess
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult MiaPostProc_PreProcess(void *postProcInstance, MiaPreProcParams *preParams)
{
    CDKResult result = MIAS_OK;
    MLOGI(Mia2LogGroupCore, "E session: %p", postProcInstance);
    MiaOfflineSession *session{reinterpret_cast<MiaOfflineSession *>(postProcInstance)};

    if (session != nullptr) {
        result = session->preProcess(preParams);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MiaPostProc_Process
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult MiaPostProc_Process(void *postProcInstance, MiaProcessParams *sessionParams)
{
    CDKResult result = MIAS_OK;
    MLOGI(Mia2LogGroupCore, "E session: %p", postProcInstance);
    MiaOfflineSession *session = reinterpret_cast<MiaOfflineSession *>(postProcInstance);
    if (NULL != session) {
        result = session->postProcess(sessionParams);
    } else {
        result = MIAS_INVALID_PARAM;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MiaPostProc_Flush
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult MiaPostProc_Flush(void *postProcInstance, bool isForced)
{
    CDKResult result = MIAS_OK;
    MLOGI(Mia2LogGroupCore, "E session: %p, isForced: %d", postProcInstance, isForced);
    MiaOfflineSession *session = reinterpret_cast<MiaOfflineSession *>(postProcInstance);
    if (NULL != session) {
        result = session->flush(isForced);
    } else {
        result = MIAS_INVALID_PARAM;
    }

    MLOGI(Mia2LogGroupCore, "X session: %p, end", postProcInstance);
    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MiaPostProc_Destroy
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void MiaPostProc_Destroy(void *postProcInstance)
{
    MLOGI(Mia2LogGroupCore, "E session: %p", postProcInstance);
    MiaOfflineSession *session = reinterpret_cast<MiaOfflineSession *>(postProcInstance);

    if (session != NULL) {
        std::unique_lock<std::mutex> locker(allSessionsMutex);
        auto it = std::find(allSessions.begin(), allSessions.end(), session);
        if (it != allSessions.end()) {
            allSessions.erase(it);
        }
        locker.unlock();

        delete session;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MiaPostProc_QuickFinish
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
QFResult MiaPostProc_QuickFinish(void *postProcInstance, QuickFinishMessageInfo &messageInfo)
{
    MLOGI(Mia2LogGroupCore, "E session: %p", postProcInstance);
    MiaOfflineSession *session = static_cast<MiaOfflineSession *>(postProcInstance);

    std::unique_lock<std::mutex> locker(allSessionsMutex);
    auto it = std::find(allSessions.begin(), allSessions.end(), session);
    if (it == allSessions.end()) {
        return sessionNotFound;
    }
    locker.unlock();

    QFResult result = session->quickFinish(messageInfo);

    MLOGI(Mia2LogGroupCore, "X session: %p quickFinish(%d) [%s] result:%d", postProcInstance,
          messageInfo.needImageResult, messageInfo.fileName.c_str(), result);

    return result;
}

} // namespace mialgo2
