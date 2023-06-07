
#include "SdkMode.h"

#include <cmath>
#include <sstream>

#include "BokehPhotographer.h"
#include "BurstPhotographer.h"
#include "CameraManager.h"
#include "HdrPhotographer.h"
#include "HwmfPhotographer.h"
#include "LogUtil.h"
#include "Metadata.h"
#include "Photographer.h"
#include "SrPhotographer.h"
#include "SupernightPhotographer.h"
namespace mihal {

SdkMode::SdkMode(AlgoSession *session, CameraDevice *vendorCamera,
                 const StreamConfig *configFromFwk, std::shared_ptr<EcologyAdapter> ecologyAdapter)
    : CameraMode(session, vendorCamera, configFromFwk),
      mMasterFormat(0),
      mEcologyAdapter(ecologyAdapter)
{
    MLOGI(LogGroupHAL, "SdkMode SdkMode SdkMode SdkMode SdkMode");
}

void SdkMode::buildSessionParameterOverride()
{
    bool isYuvSnapShot = false;

    camera3_stream_configuration *raw = mConfigFromFwk->toRaw();
    for (int i = 0; i < raw->num_streams; i++) {
        auto stream = Stream::toRichStream(raw->streams[i]);
        if (stream->isSnapshot()) {
            if (HAL_PIXEL_FORMAT_YCBCR_420_888 == stream->getFormat()) {
                isYuvSnapShot = true;
            }
            break;
        }
    }

    if (isYuvSnapShot) {
        uint8_t isThirdPartySnapshot = 1;
        mSessionParameter.update(SDK_SESSION_PARAMS_YUVSNAPSHOT, &isThirdPartySnapshot, 1);
    }
}

std::shared_ptr<LocalConfig> SdkMode::buildConfigToDarkroom(int type)
{
    CameraDevice *cameraDevice = mVendorCamera;
    std::vector<std::shared_ptr<Stream>> streams;
    std::string sig = "SDK_" + mVendorInternalStreamInfoMap.signature;
    uint32_t cameraId = static_cast<uint32_t>(atoi(mVendorCamera->getId().c_str()));

    for (const auto &stream : mConfigFromFwk->getStreams()) {
        if (!stream->isSnapshot())
            continue;
        std::shared_ptr<Stream> targetStream = nullptr;

        if (HAL_PIXEL_FORMAT_YCBCR_420_888 == stream->getFormat())
            sig += "_Yuv_";
        else if (HAL_PIXEL_FORMAT_BLOB == stream->getFormat())
            sig += "_Jpeg_";
        std::string cameraIdStr = mVendorCamera->getId();

        if (isSat() || isExtension()) {
            cameraId = mMasterId.load();
            cameraIdStr = std::to_string(cameraId);

            for (auto camera : mVendorCamera->getPhyCameras()) {
                if (cameraIdStr == camera->getId()) {
                    cameraDevice = camera;
                    break;
                }
            }
            MASSERT(cameraDevice);
        } else {
            cameraId = getBokehMasterId();
        }
        sig += cameraIdStr;
        sig += "_" + std::to_string(stream->getWidth());
        sig += "x" + std::to_string(stream->getHeight());

        if (mConfigToDarkroom) {
            std::string lastSignatureStr;
            const Metadata *meta = mConfigToDarkroom->getMeta();
            auto entry = meta->find(MI_MIVI_POST_SESSION_SIG);
            if (entry.count) {
                lastSignatureStr = reinterpret_cast<const char *>(entry.data.u8);
                MLOGD(LogGroupHAL, "lastSignatureStr: %s", lastSignatureStr.c_str());

                if (lastSignatureStr == sig) {
                    MLOGD(LogGroupHAL,
                          "already has the same size config:%s, no need to config darkroom "
                          "again",
                          sig.c_str());
                    return mConfigToDarkroom;
                }
            }
        }

        targetStream = LocalStream::create(cameraDevice, stream->getWidth(), stream->getHeight(),
                                           stream->getFormat(), stream->getUsage(),
                                           stream->getStreamUsecase(), stream->getDataspace(), true,
                                           stream->getPhyId(), CAMERA3_STREAM_OUTPUT);
        targetStream->setStreamId(streams.size());
        streams.push_back(targetStream);
    }
    MASSERT(streams.size());

    for (const auto &stream : mConfigToVendor->getStreams()) {
        if (!stream->isInternal() || !stream->isSnapshot())
            continue;
        if (isSat() && cameraId != atoi(stream->getPhyId()))
            continue;
        auto inStream = LocalStream::create(cameraDevice, stream->getWidth(), stream->getHeight(),
                                            stream->getFormat(), stream->getUsage(),
                                            stream->getStreamUsecase(), stream->getDataspace(),
                                            true, stream->getPhyId(), CAMERA3_STREAM_INPUT);
        inStream->setStreamId(streams.size());
        streams.push_back(inStream);
    }

    Metadata meta{};
    meta.update(MI_MIVI_POST_SESSION_SIG, sig.c_str());
    MLOGI(LogGroupHAL, "[sdk] prepara create post_session sig : %s", sig.c_str());

    mConfigToDarkroom =
        std::make_shared<LocalConfig>(streams, mEcologyAdapter->getPostProcOpMode(), meta);
    return mConfigToDarkroom;
}

bool SdkMode::sdkCanDoHdr(std::shared_ptr<CaptureRequest> request)
{
    auto entry = request->findInMeta(SDK_HDR_MODE);
    if (entry.count && entry.data.u8[0] == 1) {
        return canDoHdr();
    } else {
        return false;
    }
}

int SdkMode::determinPhotographer(std::shared_ptr<CaptureRequest> request)
{
    int type = Photographer::Photographer_SIMPLE;
    LocalRequest *lr = reinterpret_cast<LocalRequest *>(request.get());
    if (isFeatureSuperNightSupported() && canDoSuperNight(request)) {
        return Photographer::Photographer_SN;
    } else if ((mVendorInternalStreamInfoMap.featureMask & FeatureMask::FeatureBokeh) &&
               isDualBokeh()) {
        return Photographer::Photographer_BOKEH;
    } else if (isFeatureHdrSupported() && sdkCanDoHdr(request)) {
        if (isFeatureHdrSrSupported() && canDoHdrSr()) {
            return Photographer::Photographer_HDRSR;
        } else {
            return Photographer::Photographer_HDR;
        }
    } else if (isFeatureMfnrSupported()) {
        return Photographer::Photographer_SIMPLE;
    }

    if (mConfigFromFwk->getOpMode() == static_cast<uint32_t>(ThirdPartyOperation::AutoExtension)) {
        if (canDoHdr()) {
            if (canDoHdrSr()) {
                type = Photographer::Photographer_HDRSR;
            } else {
                const uint8_t mode = MIVISuperNightNone;
                lr->updateMeta(MI_MIVI_SUPERNIGHT_MODE, (const uint8_t *)&mode, 1);
                type = Photographer::Photographer_HDR;
            }
        } else if (canDoSuperNight(request)) {
            uint8_t rawHdrEnable = false;
            lr->updateMeta(MI_RAWHDR_ENABLE, &rawHdrEnable, 1);
            type = Photographer::Photographer_SN;
        } else if (canDoSr()) {
            type = Photographer::Photographer_SR;
        }
    }
    return type;
}

std::shared_ptr<Photographer> SdkMode::createPhotographerOverride(
    std::shared_ptr<CaptureRequest> fwkRequest)
{
    std::vector<std::shared_ptr<Stream>> streams;
    int type = determinPhotographer(fwkRequest);
    MLOGI(LogGroupHAL, "[sdk] createPhotographerOverride type: %d", type);
    streams = getCurrentCaptureStreams(fwkRequest, type);

    buildConfigToDarkroom(type);
    buildConfigToMock(fwkRequest);

    Photographer *phtgph{};
    Metadata extraMeta = {};
    LocalRequest *lr = reinterpret_cast<LocalRequest *>(fwkRequest.get());
    if (isExtension()) {
        uint32_t cameraId = mMasterId.load();
        std::string cameraIdStr = std::to_string(cameraId);
        CameraDevice *cameraDevice = NULL;
        for (auto camera : mVendorCamera->getPhyCameras()) {
            if (cameraIdStr == camera->getId()) {
                cameraDevice = camera;
                break;
            }
        }
        if (NULL != cameraDevice) {
            int isUltra = CameraDevice::RoleIdRearUltra == cameraDevice->getRoleId() ? 1 : 0;
            MLOGI(LogGroupHAL,
                  "[sdk] createPhotographerOverride isExtension!"
                  " cameraId:%d, getRoleId:%d, isUltra:%d",
                  cameraId, cameraDevice->getRoleId(), isUltra);
            uint8_t snProtraitRepairEnabled = isUltra;
            lr->updateMeta(MI_PROTRAIT_REPAIR_ENABLED, (const uint8_t *)&snProtraitRepairEnabled,
                           1);
            uint8_t snDepurpleEnabled = isUltra;
            lr->updateMeta(MI_DEPURPLE_ENABLE, (const uint8_t *)&snDepurpleEnabled, 1);
            int32_t snUltraWideLDCEnabled = isUltra;
            lr->updateMeta(MI_DISTORTION_LEVEL, (const int32_t *)&snUltraWideLDCEnabled, 1);
            uint8_t snUltraWideLDCLevel = isUltra;
            lr->updateMeta(MI_DISTORTION_ULTRAWIDE_LEVEL, (const uint8_t *)&snUltraWideLDCLevel, 1);
            // MI_DISTORTION_ULTRAWIDE_ENABLE
        } else {
            MLOGE(LogGroupHAL, "[sdk] createPhotographerOverride cameraDevice is null");
        }
    }
    switch (type) {
    case Photographer::Photographer_SN: {
        if (HAL_PIXEL_FORMAT_RAW16 == streams[0]->getFormat()) {
            phtgph =
                new RawSupernightPhotographer(fwkRequest, mSession, streams[0], mConfigToDarkroom,
                                              &mApplySuperNightData, extraMeta);
        } else {
            phtgph = new SupernightPhotographer(fwkRequest, mSession, streams[0], mConfigToDarkroom,
                                                &mApplySuperNightData, extraMeta);
        }
        break;
    }
    case Photographer::Photographer_BOKEH: {
        phtgph = new BokehPhotographer{fwkRequest, mSession, streams, mConfigToDarkroom, extraMeta};
        break;
    }
    case Photographer::Photographer_HDR: {
        phtgph = new HdrPhotographer{fwkRequest,    mSession,  streams[0],
                                     mApplyHdrData, extraMeta, mConfigToDarkroom};
        break;
    }
    case Photographer::Photographer_HDRSR: {
        phtgph = new SrHdrPhotographer{fwkRequest,    mSession,  streams[0],
                                       mApplyHdrData, extraMeta, mConfigToDarkroom};
        break;
    }
    case Photographer::Photographer_SR: {
        phtgph = new SrPhotographer{
            fwkRequest, mSession, streams[0],       static_cast<uint32_t>(getSrCheckerSum()),
            extraMeta,  false,    mConfigToDarkroom};
        break;
    }
    case Photographer::Photographer_SIMPLE: {
        uint8_t hdrEnable = 0;
        lr->updateMeta(MI_HDR_ENABLE, &hdrEnable, 1);
        uint8_t srEnable = 0;
        lr->updateMeta(MI_SUPER_RESOLUTION_ENALBE, &srEnable, 1);
        uint8_t zslEnable = 0;
        lr->updateMeta(ANDROID_CONTROL_ENABLE_ZSL, &zslEnable, 1);
        bool snMfnrEnabled = false;
        lr->updateMeta(MI_SUPERNIGHT_MFNR_ENABLED, (const uint8_t *)&snMfnrEnabled, 1);
        if (isExtension()) {
            uint8_t snMfnrEnabled = 1;
            lr->updateMeta(MI_MFNR_ENABLE, (const uint8_t *)&snMfnrEnabled, 1);
            int32_t numFrames = 5;
            lr->updateMeta(MI_MFNR_FRAMENUM, &numFrames, 1);
            // zslEnable = 1;
            // lr->updateMeta(ANDROID_CONTROL_ENABLE_ZSL, &zslEnable, 1);
            uint8_t snNightMode = 0;
            lr->updateMeta(MI_SUPERNIGHT_ENABLED, (const uint8_t *)&snNightMode, 1);
        }
        phtgph = new Photographer{fwkRequest, mSession, streams[0], extraMeta, mConfigToDarkroom};
        break;
    }
    }

    return std::shared_ptr<Photographer>{phtgph, [](Photographer *p) {
                                             MLOGI(LogGroupHAL, "delete Photographer: \n%s",
                                                   p->toString().c_str());
                                             delete p;
                                         }};
}

}; // namespace mihal
