#ifndef _MI_BOKEH_H_
#define _MI_BOKEH_H_

#include <MiaPluginUtils.h>

#include <mutex>
#include <thread>

#include "amcomdef.h" //add for E1 front 3d light
#include "bokeh.h"

namespace mialgo2 {

class MiBokehMiui
{
private:
    std::thread *mPostMiBokehInitThread;
    std::thread *mPreviewMiBokehInitThread;
    bokeh::MiBokeh *mPostMiBokeh;
    bokeh::MiBokeh *mPreviewMiBokeh;
    std::mutex mPreviewInitMutex;
    std::mutex mPostInitMutex;
    bool mPreviewInitFinish;
    void initMiBokeh(unsigned int isPreview);

public:
    MiBokehMiui();
    virtual ~MiBokehMiui();
    void init(unsigned int isPreview);
    void GetRawDepthImage(void *depthImage, bool flip);
    void process(struct MiImageBuffer *input, struct MiImageBuffer *output, unsigned int isPreview,
                 int rotate, int64_t timestamp, float fNumberApplied, int sceneflag);
    MInt32 process(struct MiImageBuffer *input, struct MiImageBuffer *output, MUInt8 *outputDepth,
                   unsigned int isPreview, int rotate, int64_t timestamp, unsigned int currentFlip,
                   float fNumberApplied, int sceneflag); //
};
/*begin add for E1 front 3d light*/
typedef struct tag_disparity_fileheader_t
{
    signed int i32Header;
    signed int i32HeaderLength;
    signed int i32PointX;
    signed int i32PointY;
    signed int i32BlurLevel;
    signed int reserved[32];
    signed int i32DataLength;
} disparity_fileheader_t;
/*end add for E1 front 3d light*/

} // namespace mialgo2
#endif
