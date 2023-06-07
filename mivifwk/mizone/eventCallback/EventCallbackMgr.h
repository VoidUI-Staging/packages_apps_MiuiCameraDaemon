#ifndef __EVENTCALLBACKMGR_H__
#define __EVENTCALLBACKMGR_H__

#include <inttypes.h>

namespace mihal {

enum EventType {
    ClientDied = 0,
    EventMax,
};

struct EventData
{
    EventType type;
    int32_t clientId;
};

typedef void (*EventCBFunc)(EventData data);

class EventCallbackMgr
{
public:
    static EventCallbackMgr *getInstance();

    virtual bool init() = 0;

    // xiaomi hal regiester API
    virtual bool registerEventCB(EventCBFunc func) = 0;

    virtual void resetCallback() = 0;

    virtual void onEventNotify(EventData data) = 0;

protected:
    /**
     *@brief EventCallbackMgr constructor
     */
    EventCallbackMgr(){};

    /**
     *@brief EventCallbackMgr destructor
     */
    virtual ~EventCallbackMgr(){};
};

}; // namespace mihal

#endif // __EVENTCALLBACKMGR_H__
