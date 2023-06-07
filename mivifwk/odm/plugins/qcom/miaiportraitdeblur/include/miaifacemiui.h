#ifndef _MI_AIFACE_H_
#define _MI_AIFACE_H_

#include <mutex>
#include <thread>

#include "MiaPluginUtils.h"
#include "amcomdef.h" //add for E1 front 3d light
#include "facial_restoration_interface.h"

namespace mialgo2 {
class MiAIFaceMiui
{
private:
    void initAIFace(unsigned int isPreview);
    aiface::FacialRestorationInterface *m_facialRecialRestorationInterface;
    std::thread *mPostMFacialRestorationInitThread;
    std::mutex mPostInitMutex;

public:
    MiAIFaceMiui();
    virtual ~MiAIFaceMiui();
    void init(unsigned int isPreview);
    void process(struct MiImageBuffer *input, struct MiImageBuffer *output,
                 aiface::MIAIParams *params);
};

} // namespace mialgo2
#endif
