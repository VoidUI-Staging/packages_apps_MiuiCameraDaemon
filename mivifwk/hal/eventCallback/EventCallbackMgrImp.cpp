#include "EventCallbackMgrImp.h"

#include <inttypes.h>
#include <utils/Log.h>

#include "MiDebugUtils.h"

using namespace android;
using namespace mihal;
using namespace midebug;

#undef LOG_TAG
#define LOG_TAG "MiCamService"

EventCallbackMgr *EventCallbackMgr::getInstance()
{
    static EventCallbackMgrImp gEventCallbackMgr;
    return &gEventCallbackMgr;
}

EventCallbackMgrImp::EventCallbackMgrImp() {}

EventCallbackMgrImp::~EventCallbackMgrImp() {}

bool EventCallbackMgrImp::registerEventCB(EventCBFunc func)
{
    Mutex::Autolock _l(mLock);
    mEventCbFunc = func;
    return true;
}

void EventCallbackMgrImp::resetCallback()
{
    Mutex::Autolock _l(mLock);
    mEventCbFunc = nullptr;
}

void EventCallbackMgrImp::onEventNotify(EventData data)
{
    Mutex::Autolock _l(mLock);
    if (mEventCbFunc == nullptr) {
        MLOGW(Mia2LogGroupService, "EventCallback(mEventCbFunc) function is empty!");
        return;
    }

    mEventCbFunc(data);
}

bool EventCallbackMgrImp::init()
{
    return true;
}
