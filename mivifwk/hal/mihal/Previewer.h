#ifndef __PREVIEWER_H__
#define __PREVIEWER_H__

#include <bitset>

#include "Session.h"

namespace mihal {

class Previewer final : public AsyncAlgoSession::RequestExecutor
{
public:
    Previewer(const CaptureRequest *fwkRequest, Session *session, bool isLowCapability = false);
    Previewer(std::shared_ptr<const CaptureRequest> fwkRequest, Session *session,
              bool isLowCapability = false);
    int execute() override;
    int processVendorResult(CaptureResult *result) override;
    std::vector<uint32_t> getVendorRequestList() const override;
    int processVendorNotify(const NotifyMessage *msg) override;
    std::string toString() const override;
    std::string getName() const override { return "Previewer"; }
    int processEarlyResult(std::shared_ptr<StreamBuffer> inputBuffer);
    bool isFinish(uint32_t vndFrame, int32_t *frames) override;

protected:
    std::shared_ptr<StreamBuffer> mapToRequestedBuffer(uint32_t frame,
                                                       const StreamBuffer *buffer) const override;

private:
    void init();
    void initPendingSet();

    std::unique_ptr<CaptureRequest> mVendorRequest;
    bool mIsFullMeta;
    std::bitset<16> mPendingBuffers;
    mutable std::mutex mMutex;
    std::atomic_int mStatus;

    // NOTE: in some cases, Qcom HAL preview pipeline will only output one port. In such cases, we
    // should only send quickview buffer to Qcom to make sure early pic mechanism behave normally.
    // For details refer to doc: https://xiaomi.f.mioffice.cn/docs/dock4qQImIBogDbm7qEcpxLllld
    bool mIsLowCapability;
    std::atomic<int64_t> mResultTs;
};

} // namespace mihal

#endif
