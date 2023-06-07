#ifndef MIIMAGEBUFFER_H__
#define MIIMAGEBUFFER_H__

#include <memory>

#include "MiZoneTypes.h"
namespace mizone {

enum { // for MERROR return value
    MI_BUFFER_CREATE_OK = 0,
    MI_BUFFER_CREATE_FAIL = -1
};

struct ImgParam
{
    int format;
    int width;
    int height;
    uint32_t bufferSize;
    unsigned long usage;
};

class MiImageBuffer
{
public:
    MiImageBuffer(std::shared_ptr<mcam::IImageBufferHeap> heap) : mImageBufferHeap(heap) {}
    MiImageBuffer() {}
    static auto createBuffer(char const *clientName, char const *bufName, ImgParam const &param,
                             bool enableLog = false) -> std::shared_ptr<MiImageBuffer>;

    bool isEmpty() { return mImageBufferHeap == nullptr; }

    //获取buffer信息
    int getImgFormat() const;
    MSize const &getImgSize() const;

    int getImgWidth() const;
    int getImgHight() const;
    //返回每个plane的大小
    size_t getBufSizeInBytes(size_t index = 0) const;

    //返回width对应的stride
    size_t getBufStridesInBytes() const;

    //返回image的总大小
    size_t getBufSize() const;

    //返回image
    // buffer虚拟地址,调用前需要进行lock操作,使用SW的usage进行lock。index对应plane,一般用于ion类型。
    unsigned char *getBufVA(size_t index = 0) const;

    //返回buffer对应的fd,调用前需要进行lock操作
    int getFd(size_t index = 0) const;

    // return buffer_handle_t, if this image buffer is IGraphicImageBufferHeap,
    // otherwise, return nullptr
    buffer_handle_t *getBufferHandlePtr();

    // lock操作
    bool lockBuf(char const *szCallerName,
                 int usage = eBUFFER_USAGE_HW_CAMERA_READWRITE | eBUFFER_USAGE_SW_READ_OFTEN);
    bool unlockBuf(char const *szCallerName);

    std::shared_ptr<mcam::IImageBufferHeap> getBufferHeap();

private:
    friend class DecoupleUtil;
    std::shared_ptr<mcam::IImageBufferHeap> mImageBufferHeap = nullptr;
};
}; // namespace mizone

#endif
