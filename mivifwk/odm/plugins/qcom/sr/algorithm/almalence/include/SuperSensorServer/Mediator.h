#ifndef __MEDIATOR_H__
#define __MEDIATOR_H__

#include <SuperSensorServer/InputStream.h>
#include <SuperSensorServer/Lock.h>
#include <SuperSensorServer/OutputStream.h>

namespace almalence {

/**
 *  Class for transferring data between Services.
 *  Service can write to mediator data of type IN, using asOutput() method.
 *  And Service can read from mediator data of type OUT, using asInput() method.
 *  So, pipeline could look like this:
 *  ServiceA -> Mediator.asOutput() -> InternalMediatorStorage -> Mediator.asInput() -> ServiceB.
 *
 * @tparam IN input type.
 * @tparam OUT output type.
 * @tparam IN_PARAMS input parameters type list.
 */
template <typename IN, typename OUT, typename IN_REQUEST>
class Mediator : public Lockable
{
public:
    virtual ~Mediator() {}

    // Get resource for writing (where to write). Used to put data.
    virtual OutputStream<IN, IN_REQUEST> &asOutput() = 0;

    // Get resource for reading (where to read from). Used to retrieve data.
    virtual InputStream<OUT> &asInput() = 0;

    void endStream() { this->asInput().endStream(); }

    virtual void waitDrained() = 0;
};

} // namespace almalence

#endif //__MEDIATOR_H__
