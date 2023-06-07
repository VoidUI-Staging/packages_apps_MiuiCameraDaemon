#ifndef __ALMASHOTHOLDER_H__
#define __ALMASHOTHOLDER_H__

#include <SuperSensorServer/Lock.h>
#include <almashot/hvx/superzoom.h>

namespace almalence {

/**
 * Class responsible form DSP AlmaShot initialization.
 * Initialization at launch, Release on shutdown.
 */
class AlmaShotHolder
{
private:
    static int code;
    static int counter;

public:
    AlmaShotHolder();

    virtual ~AlmaShotHolder();

    static int getErrorCode();

    static bool isGood();

    static Lock<int> acquire();
};

} // namespace almalence

#endif // __ALMASHOTHOLDER_H__
