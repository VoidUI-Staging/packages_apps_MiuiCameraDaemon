#ifndef __MI_BOKEH_H__
#define __MI_BOKEH_H__

#include <mutex>
#include <thread>

#include "bokeh.h"
#include "device/device.h"

namespace mialgo2 {

typedef enum __miBokehDebug {
    MI_BOKEH_DEBUG_INFO = (1) << (0),
} MI_BOKEH_DEBUG;

class MiBokehLdc
{
public:
    static MiBokehLdc *GetInstance()
    {
        static MiBokehLdc miBokehLdc;
        return &miBokehLdc;
    }
    void SetDebugLevel(U32 debug);
    void AddRef();
    void AddRefIfZero();
    void ReleaseRef();
    S32 GetDepthMask(DEV_IMAGE_BUF *input, DEV_IMAGE_BUF *outputDepth, S32 rotate, S32 flip);
    bool m_doBokehProcess;

private:
    MiBokehLdc();
    ~MiBokehLdc();
    MiBokehLdc(const MiBokehLdc &) = delete;
    MiBokehLdc &operator=(const MiBokehLdc &) = delete;
    S32 Init();
    void Destroy();
    U32 m_debug = 0; // "persist.vendor.camera.LDC.debug".
    std::thread *m_postMiBokehInitThread;
    bokeh::MiBokeh *m_postMiBokeh;
    std::mutex m_postInitMutex;
    std::mutex m_refCountMutex;
    S32 m_bokehInitCount = 0;
};

} // namespace mialgo2
#endif //__MI_BOKEH_H__
