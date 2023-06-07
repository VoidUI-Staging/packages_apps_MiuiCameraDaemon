#ifndef MIPOSTPROCDEVICE_H
#define MIPOSTPROCDEVICE_H

#include "MiPostProcDeviceUtil.h"

namespace mizone {
namespace mipostprocinterface {

class IMiPostProcEngine
{
public:
    virtual ~IMiPostProcEngine() = default;

public:
    virtual int process(ProcessInfos &pInfos) = 0;
    virtual int configure(StreamConfiguration &cfg) = 0;
    virtual int request(std::vector<CaptureRequest> &requests) = 0;
    virtual void destroy() = 0;
};

class MiPostProcDevice
{
public:
    class MiPostProcDeviceCb : public IMtkcamDeviceCallback
    {
    public:
        MiPostProcDeviceCb(MiPostProcDevice *pParent) : mParent(pParent) {}
        ~MiPostProcDeviceCb() {}

    public:
        void notify(const std::vector<NotifyMsg> &msgs) override { mParent->handleNotify(msgs); }
        void processCaptureResult(const std::vector<CaptureResult> &results) override
        {
            mParent->handleResult(results);
        }

    private:
        MiPostProcDevice *mParent;
    };

public:
    static auto create(CreationInfos &rInfos) -> std::shared_ptr<MiPostProcDevice>;
    int process(ProcessInfos &pInfos);
    int configure(StreamConfiguration &streamCfg);
    int request(std::vector<CaptureRequest> &requests);
    int flush();
    int destroy();

public:
    MiPostProcDevice(CreationInfos &rInfos);
    ~MiPostProcDevice();

private:
    void getPostProcEngine(CreationInfos &rInfos);
    void getPostProcProvider();
    void getPostProcSession(uint32_t engineTag);

    void handleNotify(const std::vector<NotifyMsg> &msgs);
    void handleResult(const std::vector<CaptureResult> &results);

private:
    MiPostProcAdaptorCb *mAdaptorCb;
    std::shared_ptr<MiPostProcDeviceCb> mDeviceCb;
    std::shared_ptr<IMiPostProcEngine> mEngineSession;

    std::shared_ptr<IMtkcamProvider> mProvider;
    std::shared_ptr<IMtkcamDevice> mDevice;
    std::shared_ptr<IMtkcamDeviceSession> mDeviceSession;
};

class MiCaptureEngine : public IMiPostProcEngine
{
public:
    static auto openPostProcEngine() -> std::shared_ptr<IMiPostProcEngine>;
    MiCaptureEngine();
    ~MiCaptureEngine();

private:
    int process(ProcessInfos &pInfos) override;
    int configure(StreamConfiguration &cfg) override;
    int request(std::vector<CaptureRequest> &requests) override;
    void destroy() override;

private:
    ProcessInfos mInfos;
};

} // namespace mipostprocinterface
} // namespace mizone

#endif
