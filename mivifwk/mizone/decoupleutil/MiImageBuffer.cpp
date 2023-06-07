#include "MiImageBuffer.h"

#include <mtkcam-android/utils/imgbuf/IGraphicImageBufferHeap.h>

#include <memory>

#include "MiZoneTypes.h"

namespace mizone {

auto MiImageBuffer::createBuffer(char const *clientName, char const *bufName, ImgParam const &param,
                                 bool enableLog) -> std::shared_ptr<MiImageBuffer>
{
    std::shared_ptr<MiImageBuffer> buffer = std::make_shared<MiImageBuffer>();
    mcam::IImageBufferAllocator::ImgParam mtkParam(
        param.format, mcam::MSize(param.width, param.height), param.usage);
    // NOTE: for buffer send to mialgoengine, the align info here should match that define in
    //       MiaImage::MiaImage(MiaFrame *miaFrame, MiaImageType type)
    if (param.format == eImgFmt_MTK_YUV_P010) {
        mtkParam.strideAlignInByte = 32;
    } else if (param.format == eImgFmt_RAW16) {
        // if stream format is eImgFmt_RAW16, it need to allocate blob buffer
        mtkParam =
            mcam::IImageBufferAllocator::ImgParam::getBlobParam(param.bufferSize, param.usage);
    } else {
        mtkParam.strideAlignInPixel = 64;
    }
    buffer->mImageBufferHeap =
        mcam::IImageBufferHeapFactory::create(clientName, bufName, mtkParam, enableLog);
    if (buffer->mImageBufferHeap != nullptr) {
        return buffer;
    }
    return nullptr;
}

auto MiImageBuffer::getImgFormat() const -> int
{
    return mImageBufferHeap->getImgFormat();
}

MSize const &MiImageBuffer::getImgSize() const
{
    return mImageBufferHeap->getImgSize();
}

int MiImageBuffer::getImgWidth() const
{
    return mImageBufferHeap->getImgSize().w;
}

int MiImageBuffer::getImgHight() const
{
    return mImageBufferHeap->getImgSize().h;
}

size_t MiImageBuffer::getBufSizeInBytes(size_t index) const
{
    return mImageBufferHeap->getBufSizeInBytes(index);
}

size_t MiImageBuffer::getBufStridesInBytes() const
{
    return mImageBufferHeap->getBufStridesInBytes(0);
}

size_t MiImageBuffer::getBufSize() const
{
    int num = mImageBufferHeap->getPlaneCount();
    size_t res = 0;
    for (size_t i = 0; i < num; i++) {
        res += mImageBufferHeap->getBufSizeInBytes(i);
    }
    return res;
}

unsigned char *MiImageBuffer::getBufVA(size_t index) const
{
    return (unsigned char *)mImageBufferHeap->getBufVA(index);
}

int MiImageBuffer::getFd(size_t index) const
{
    return mImageBufferHeap->getHeapID(index);
}

buffer_handle_t *MiImageBuffer::getBufferHandlePtr()
{
    if (mImageBufferHeap == nullptr) {
        return nullptr;
    }

    auto *graphicBufferHeap = NSCam::IGraphicImageBufferHeap::castFrom(mImageBufferHeap.get());
    if (graphicBufferHeap == nullptr) {
        return nullptr;
    }

    return graphicBufferHeap->getBufferHandlePtr();
}

std::shared_ptr<mcam::IImageBufferHeap> MiImageBuffer::getBufferHeap()
{
    return mImageBufferHeap;
}

bool MiImageBuffer::lockBuf(char const *szCallerName, int usage)
{
    return mImageBufferHeap->lockBuf(szCallerName, usage);
}

bool MiImageBuffer::unlockBuf(char const *szCallerName)
{
    return mImageBufferHeap->unlockBuf(szCallerName);
}
}; // namespace mizone
