#ifndef __MI_FRAME_WRAPPER__
#define __MI_FRAME_WRAPPER__

#include <nativebase/nativebase.h>

#include "LogUtil.h"

namespace mialgo {

class MiaFrameWrapper : public virtual RefBase
{
public:
    MiaFrameWrapper(MiaFrame *frameHandle);
    virtual ~MiaFrameWrapper();

    inline MiaFrame *getMiaFrame() { return &mFrame; };

    inline uint32_t getMergeInputNumber() { return mInputNumber; };

    inline int64_t getTimestampUs() { return mTimestampUs; };
    inline std::string &getFileName() { return mFileName; };

private:
    int32_t mergeAppRequestSetting(camera_metadata_t *src, camera_metadata_t *&dest);

    MiaFrame mFrame;
    uint32_t mInputNumber;
    int64_t mTimestampUs;

    MiImageFormat mFormat;
    std::string mFileName;

    int32_t mRequestId;
}; // end class image define

} // namespace mialgo
#endif // END define __MI_FRAME_WRAPPER__
