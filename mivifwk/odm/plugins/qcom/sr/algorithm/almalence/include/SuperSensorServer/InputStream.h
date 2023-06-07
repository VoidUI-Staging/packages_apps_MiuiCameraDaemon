#ifndef __INPUT_STREAM_H__
#define __INPUT_STREAM_H__

#include <SuperSensorServer/Lock.h>

namespace almalence {

/**
 * @tparam T readable type.
 */
template <typename T>
class InputStream
{
public:
    virtual ~InputStream() {}

    virtual Lock<T> acquire(bool next = true) = 0;

    virtual void endStream() = 0;
};

} // namespace almalence

#endif // __INPUT_STREAM_H__
