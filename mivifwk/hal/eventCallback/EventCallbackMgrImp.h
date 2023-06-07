#ifndef _EVENTCALLBACKMGR_IMP_H_
#define _EVENTCALLBACKMGR_IMP_H_

#include <EventCallbackMgr.h>
#include <utils/Mutex.h>

namespace mihal {

class EventCallbackMgrImp : public EventCallbackMgr
{
public:
    virtual bool init();

public:
    EventCallbackMgrImp();
    virtual ~EventCallbackMgrImp();

    virtual bool registerEventCB(EventCBFunc func);

    void resetCallback();

    virtual void onEventNotify(EventData data);

private:
    mutable android::Mutex mLock;
    EventCBFunc mEventCbFunc = nullptr;
};

}; // namespace mihal

#endif // _EVENTCALLBACKMGR_IMP_H_