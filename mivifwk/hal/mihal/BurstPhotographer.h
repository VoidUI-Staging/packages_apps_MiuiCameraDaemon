#ifndef __BURSTPHOTOGRAPHER_H__
#define __BURSTPHOTOGRAPHER_H__
#include "Metadata.h"
#include "Photographer.h"

namespace mihal {

class BurstPhotographer final : public Photographer
{
public:
    BurstPhotographer(/* args */) = delete;

    BurstPhotographer(std::shared_ptr<const CaptureRequest> fwkRequest, Session *session,
                      std::shared_ptr<Stream> extraStream, const Metadata extraMeta,
                      const Metadata requestMetadata,
                      std::shared_ptr<StreamConfig> darkroomConfig = nullptr);

    ~BurstPhotographer() = default;

    std::string getName() const override { return "BurstPhotographer"; }
    int execute() override;

protected:
    std::string getImageName() const override;

private:
    Metadata mRequestMetadata;
};

};     // namespace mihal
#endif // __BURSTPHOTOGRAPHER_H__
