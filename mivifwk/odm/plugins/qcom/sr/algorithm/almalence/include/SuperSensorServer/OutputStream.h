#ifndef __OUTPUT_STREAM_H__
#define __OUTPUT_STREAM_H__

#include <SuperSensorServer/Lock.h>

namespace almalence {

/**
 * Data OutputStream template.
 * @tparam T writable type.
 * @tparam P parameter type.
 */
template <typename T, typename P>
class OutputStream
{
public:
    virtual ~OutputStream() {}

    virtual Lock<T> acquire(P param) = 0;
};

} // namespace almalence

#endif // __OUTPUT_STREAM_H__
