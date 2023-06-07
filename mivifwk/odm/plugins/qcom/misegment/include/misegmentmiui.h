#ifndef _MI_SEGMENT_H_
#define _MI_SEGMENT_H_

#include <utils/Log.h>

#include <mutex>
#include <thread>

#include "MiaPluginUtils.h"
#include "bokeh.h"
using namespace bokeh;
namespace mialgo2 {

class MiSegmentMiui
{
private:
    std::thread *mPostMiSegmentInitThread;
    std::thread *mPreviewMiSegmentInitThread;
    MiBokeh *mPostMiSegment;
    MiBokeh *mPreviewMiSegment;
    std::mutex mPreviewInitMutex;
    std::mutex mPostInitMutex;
    bool mPreviewInitFinish;
    bool mCanAlgoDoProcess;
    MiImageBuffer mAnalyzeBuffer;
    void initMiSegment(unsigned int isPreview);

public:
    MiSegmentMiui();
    virtual ~MiSegmentMiui();
    void init(unsigned int isPreview);
    void process(struct MiImageBuffer *input, struct MiImageBuffer *output, unsigned int isPreview,
                 int rotate, int sceneflag, int64_t timestamp);
};

} // namespace mialgo2
#endif
