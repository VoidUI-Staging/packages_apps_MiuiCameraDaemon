#ifndef __MIALGO2CALLBACK_H__
#define __MIALGO2CALLBACK_H__
#include <MiaPluginUtils.h>
#include <MiaPostProcType.h>
#include <VendorMetadataParser.h>
#include <semaphore.h>
#include <system/camera_vendor_tags.h>

using namespace mialgo2;
sem_t sem;
uint32_t sessionStatus = 0;

class MiFrameProcessorCallback2 : public MiaFrameCb
{
public:
    MiFrameProcessorCallback2();
    virtual ~MiFrameProcessorCallback2(){};
    virtual void release(uint32_t frameNumber, int64_t timeStampUs, MiaFrameDirection type,
                         uint32_t streamIdx, buffer_handle_t handle);
    virtual int getBuffer(int64_t timeStampUs, OutSurfaceType type, buffer_handle_t *buffer);

    uint32_t mStatus;
};

MiFrameProcessorCallback2::MiFrameProcessorCallback2() {}

void MiFrameProcessorCallback2::release(uint32_t frameNumber, int64_t timeStampUs,
                                        MiaFrameDirection type, uint32_t streamIdx,
                                        buffer_handle_t handle)
{
    if (type == MiaFrameDirection::INPUT_FRAME) {
        ++sessionStatus;
        MLOGI(Mia2LogGroupCore, "release input frame ");
    } else if (type == MiaFrameDirection::OUPUT_FRAME) {
        ++sessionStatus;
        MLOGI(Mia2LogGroupCore, "release output frame ");
    }
    MLOGI(Mia2LogGroupCore, "frame_number: %d, type: %d", frameNumber, type);
    if (sessionStatus == 2) {
        sem_post(&sem);
    }
}

int MiFrameProcessorCallback2::getBuffer(int64_t timeStampUs, OutSurfaceType type,
                                         buffer_handle_t *buffer)
{
    MLOGE(Mia2LogGroupCore, "unsupported");
    return 0;
}

class MySessionCb2 : public MiaPostProcSessionCb
{
public:
    MySessionCb2();
    virtual ~MySessionCb2() { MLOGI(Mia2LogGroupCore, "MySessionCb has been deconstructed."); };
    virtual void onSessionCallback(MiaResultCbType type, uint32_t frameNum, int64_t timeStamp,
                                   void *msgData);
};

MySessionCb2::MySessionCb2()
{
    MLOGI(Mia2LogGroupCore, "MySessionCb2 has been constructed.");
}

void MySessionCb2::onSessionCallback(MiaResultCbType type, uint32_t frameNum, int64_t timeStamp,
                                     void *msgData)
{
    MLOGI(Mia2LogGroupCore,
          "UT callback:resultCb type %d frameNum: %d timeStamp: " PRIu64 " msgData: %p", type,
          frameNum, timeStamp, msgData);
}

#endif /*__MIALGO2CALLBACK_H__*/
