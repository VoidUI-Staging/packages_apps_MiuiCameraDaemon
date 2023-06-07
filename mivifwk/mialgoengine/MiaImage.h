/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef __MI_IMAGE__
#define __MI_IMAGE__

#include <nativebase/nativebase.h>
#include <vndk/hardware_buffer.h>

#include "MiaBufferManager.h"
#include "MiaPostProcIntf.h"
#include "MiaUtil.h"

typedef enum {
    MI_IMG_SOURCE,       // from app input
    MI_IMG_INTERNAL,     // from mialgoengine internal
    MI_IMG_VIRTUAL_SINK, // buffer from external and need a metadata in this MiaImage
    MI_IMG_SINK          // from app output
} MiaImageType;

typedef enum {
    MI_IMG_STATE_UNINITIALED = -1,
    MI_IMG_STATE_INITIALED,
    MI_IMG_STATE_RELEASED,
} MiaImageState;

#define IMAGE_MAX_USERS_CNT 8

namespace mialgo2 {

class MiaImage : public virtual RefBase
{
public:
    MiaImage(MiaFrame *miaFrame, MiaImageType type);
    MiaImage(sp<MiaBufferManager> bufferManager, MiaImageFormat format);
    MiaImage(MiaImageFormat format, int64_t timestamp, uint32_t portId, MiaFrameCb *callback,
             MiaImageType sinkType);
    virtual ~MiaImage();

    MiaImageType getImageType() { return mType; }
    void setImageType(MiaImageType type) { mType = type; }

    MiaImageFormat getImageFormat() { return mFormat; }
    void setImageFormat(MiaImageFormat format) { mFormat = format; }

    inline uint32_t getMergeInputNumber() { return mInputNumber; }
    inline uint32_t getTotalReqNum() { return mTotalReqNum; }
    inline void setMergeInputNumber(uint32_t inputNum) { mInputNumber = inputNum; }

    std::string getImageName() { return mImageName; }
    void setImageName(std::string &newImageName) { mImageName = newImageName; }

    inline uint32_t getPortId() { return mPortId; }

    inline uint32_t getFrameNumber() { return mFrameNum; }

    inline int64_t getTimestamp() { return mTimestamp; }

    MiaImageState getImageState() { return mState; }

    MiaImageBuffer *getMiaImageBuffer() { return &mBuffer; }
    std::shared_ptr<DynamicMetadataWraper> getMetadataWraper() { return mMetaWraper; }

    void copyParams(const sp<MiaImage> &image, int64_t timeStamp, int mergeNum);

    void releaseExternalFrame();

    uint32_t decreaseUseCount()
    {
        std::lock_guard<std::mutex> lock(mUseCountMutex);
        return --mUseCount;
    }
    uint32_t increaseUseCount()
    {
        std::lock_guard<std::mutex> lock(mUseCountMutex);
        return ++mUseCount;
    }
    uint32_t getUseCount()
    {
        std::lock_guard<std::mutex> lock(mUseCountMutex);
        return mUseCount;
    }

    const char *getImageTypeName(int type);

private:
    sp<MiaBufferManager> mBufferManager;
    std::shared_ptr<DynamicMetadataWraper> mMetaWraper;
    MiaBufferHandle *mInternalBufHandle;
    MiaImageBuffer mBuffer;
    MiaFrameCb *mFrameCb;
    uint64_t mFrameNum;
    int64_t mTimestamp;
    uint32_t mInputNumber;
    uint32_t mTotalReqNum;
    uint32_t mPortId;
    MiaImageFormat mFormat;
    MiaImageType mType;
    MiaImageState mState;
    std::string mImageName;
    uint32_t mUseCount;
    std::mutex mUseCountMutex;
}; // end class image define

} // namespace mialgo2
#endif // END define __MI_IMAGE__
