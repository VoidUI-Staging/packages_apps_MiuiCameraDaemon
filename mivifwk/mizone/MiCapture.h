/*
 * Copyright (c) 2021. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef _MIZONE_MICAPTURE_H_
#define _MIZONE_MICAPTURE_H_

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <vector>

#include "MiMetadata.h"
#include "MiZoneTypes.h"

namespace mizone {

class MiAlgoCamMode;
class MiBufferManager;
struct MiRequest;
struct MiStream;

class MiCapture
{
public:
    enum class CaptureType {
        PreviewType,
        SnapshotType,
    };

    struct CreateInfo
    {
        CaptureType type;
        MiAlgoCamMode *mode;
        CameraInfo cameraInfo;
        CaptureRequest fwkRequest;
        bool quickViewEnable = true;
        uint32_t frameCount;
        std::vector<std::shared_ptr<MiStream>> streams;
        MiMetadata settings;
        // <vndStreamId, fwkStreamId>
        std::map<int32_t, int32_t> mapStreamIdToFwk;
        std::vector<MiMetadata> settingsPerframe; // optional
        // <physicalCameraId, settings>
        std::map<int32_t, MiMetadata> physicalSettings; // optional
        // For app sanpshot control
        std::string captureTypeName;
        uint32_t operationMode;
        int32_t quickShotDelayTime = 0;
        bool isZslEnable = false;
        bool singleFrameEnable = false;
        uint8_t isAppZslEnable = 1;
        bool mQuickShotEnabled = false;
    };

protected:
    MiCapture(const CreateInfo &info);

public:
    virtual ~MiCapture();
    MiCapture(const MiCapture &) = delete;
    MiCapture &operator=(const MiCapture &) = delete;
    static std::unique_ptr<MiCapture> create(const CreateInfo &info);

    // processVendorRequest(), processVendorResult(), and processVendorNotify() have default
    // implementation in base class, but they still declared as pure virtual function. So if any
    // derived class want to reuse them, should call them in their overrided version explicitly.
    virtual int processVendorRequest() = 0;
    virtual void processVendorResult(const CaptureResult &result) = 0;
    virtual void processVendorNotify(const NotifyMsg &msg) = 0;

    bool isSingleFrameEnabled() { return mSingleFrameEnable; }
    uint32_t getFwkFrameNumber() { return mFwkRequest.frameNumber; }
    std::string getmCaptureTypeName() { return mCaptureTypeName; }
    uint32_t getFirstVndFrameNum() { return mFirstVndFrameNum; }
    std::map<uint32_t, std::shared_ptr<MiRequest>> getMiRequests();

    bool isSnapshot() { return mType == CaptureType::SnapshotType; };

protected:
    CaptureType mType;
    MiAlgoCamMode *mCurrentCamMode;
    CameraInfo mCameraInfo;
    // <vndFrameNum, MiRequest>
    std::map<uint32_t, std::shared_ptr<MiRequest>> mMiRequests;
    CaptureRequest mFwkRequest;
    bool mQuickViewEnable;
    // <streamId, MiStream>
    std::map<int32_t, std::shared_ptr<MiStream>> mStreams;
    // <vndStreamId, fwkStreamId>
    std::map<int32_t, int32_t> mMapStreamIdToFwk;
    uint32_t mFrameCount;
    uint32_t mFirstVndFrameNum;
    bool mIsError;
    std::string mCaptureTypeName; // For app sanpshot control
    uint32_t mOperationMode;
    bool mSingleFrameEnable;
    //<vndStreamId>
    std::set<int32_t> mRequestsCompleted;
    bool mInAlgoEngine;
    // pure virtual functions for process logic, derived class should implement them.
    virtual void processMetaResult(const CaptureResult &result) = 0;
    virtual void processBufferResult(const CaptureResult &result) = 0;
    virtual void processShutterMsg(const ShutterMsg &shutterMsg) = 0;
    virtual void processErrorMsg(const ErrorMsg &errorMsg) = 0;
    virtual void onCompleted() = 0;
    virtual void singleFrameOnCompleted(int32_t frameNumber) = 0;
    virtual void updateResult(std::shared_ptr<MiRequest> miReq) = 0;

    void updateRecords(const CaptureResult &result);
    void updateRecords(const NotifyMsg &msg);
    bool isCompleted();
    bool isCompleted(uint32_t frameNumber);

    void pickBuffersOwnedToFwk(const std::vector<StreamBuffer> &vndBuffers,
                               std::vector<StreamBuffer> &fwkBuffers);

    //                             <streamId, vector<StreamBuffer>>
    bool requestFwkBuffers(std::map<int32_t, std::vector<StreamBuffer>> &bufferMap);
    //                             <streamId, vector<StreamBuffer>>
    bool requestVndBuffers(std::map<int32_t, std::vector<StreamBuffer>> &bufferMap,
                           std::map<int32_t, std::shared_ptr<MiStream>> &streams);
};

} // namespace mizone

#endif //_MIZONE_MICAPTURE_H_
