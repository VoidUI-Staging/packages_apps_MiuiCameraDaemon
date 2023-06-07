#include "MiMetadata.h"

#include <memory>

#include "mtkcam-halif/def/TypeManip.h"

namespace mizone {

//模仿IMetadata,不加const会报错，不知道为啥?
MiMetadata::MiMetadata(MiMetadata const &m)
{
    mIMetadata = std::make_shared<NSCam::IMetadata>(*m.mIMetadata);
}

MiMetadata::MiMetadata(IMetadata const &m)
{
    mIMetadata = std::make_shared<NSCam::IMetadata>(m);
}

MiMetadata::MiMetadata()
{
    mIMetadata = std::make_shared<NSCam::IMetadata>();
}

MiMetadata::~MiMetadata()
{
    mIMetadata = nullptr;
}

IMetadata MiMetadata::getIMetadata()
{
    return *mIMetadata.get();
}

MBOOL MiMetadata::isEmpty() const
{
    return mIMetadata->isEmpty();
}

MiMetadata &MiMetadata::operator+=(MiMetadata const &other)
{
    IMetadata &one = *mIMetadata;
    IMetadata &theOne = *(other.mIMetadata);
    one += theOne;
    return *this;
}

MiMetadata &MiMetadata::operator=(MiMetadata const &other)
{
    IMetadata &one = *mIMetadata;
    IMetadata &theOne = *(other.mIMetadata);
    one = theOne;
    return *this;
}

MiMetadata MiMetadata::operator+(MiMetadata const &other)
{ // helper function
    return MiMetadata(*this) += other;
}

//只比较entry中的的第一项，尚不全面
bool MiMetadata::operator==(MiMetadata const &other) const
{
    IMetadata &one = *mIMetadata;
    IMetadata &theOne = *(other.mIMetadata);
    if (one.count() != theOne.count()) {
        return false;
    }
    int size = one.count();
    for (int i = 0; i < size; i++) {
        IMetadata::IEntry a = one.entryAt(i);
        IMetadata::IEntry b = theOne.entryAt(i);
        if (a.tag() != b.tag()) {
            return false;
        }
        if (a.type() != b.type()) {
            return false;
        }
        int len = 0;
        switch (a.type()) {
        case NSCam::TYPE_MUINT8:
            len = 1;
            break;
        case NSCam::TYPE_MINT32:
        case NSCam::TYPE_MFLOAT:
            len = 4;
            break;
        case NSCam::TYPE_MINT64:
        case NSCam::TYPE_MDOUBLE:
        case NSCam::TYPE_MRational:
            len = 8;
            break;
        case NSCam::TYPE_MSize: {
            MSize s1 = a.itemAt(0, Type2Type<MSize>());
            MSize s2 = b.itemAt(0, Type2Type<MSize>());
            if (!(s1 == s2)) {
                // return false;
            }
            break;
        }
        case NSCam::TYPE_MPoint: {
            MPoint p1 = a.itemAt(0, Type2Type<MPoint>());
            MPoint p2 = b.itemAt(0, Type2Type<MPoint>());
            if (!(p1 == p2)) {
                return false;
            }
            break;
        }
        case NSCam::TYPE_MRect: {
            MRect r1 = a.itemAt(0, Type2Type<MRect>());
            MRect r2 = b.itemAt(0, Type2Type<MRect>());
            if (!(r1 == r2)) {
                return false;
            }
            break;
        }
        case NSCam::TYPE_IMetadata: {
            IMetadata m1 = a.itemAt(0, Type2Type<IMetadata>());
            IMetadata m2 = b.itemAt(0, Type2Type<IMetadata>());
            MiMetadata M1(m1);
            MiMetadata M2(m2);
            if (!(M1 == M2)) {
                return false;
            }
            break;
        }
        case NSCam::TYPE_Memory: {
            NSCam::IMetadata::Memory mm1 = a.itemAt(0, Type2Type<NSCam::IMetadata::Memory>());
            NSCam::IMetadata::Memory mm2 = b.itemAt(0, Type2Type<NSCam::IMetadata::Memory>());
            if (!(mm1 == mm2)) {
                return false;
            }
            break;
        }
        }
        if (len != 0) {
            if (memcmp(a.data(), b.data(), len) != 0) {
                return false;
            }
        }
    }
    return true;
}

template <typename T>
bool MiMetadata::getEntry(const MiMetadata *metadata, Tag_t const tag, T &val, size_t index)
{
    if (nullptr == metadata)
        return false;
    return IMetadata::getEntry(metadata->mIMetadata.get(), tag, val, index);
}

template <typename T>
MERROR MiMetadata::setEntry(MiMetadata *metadata, Tag_t const tag, T const &val)
{
    if (nullptr == metadata)
        return MI_METADATA_FAIL;
    return IMetadata::setEntry(metadata->mIMetadata.get(), tag, val);
}

MERROR MiMetadata::remove(Tag_t tag)
{
    return mIMetadata->remove(tag);
}

MUINT MiMetadata::count() const
{
    return mIMetadata->count();
}
MVOID MiMetadata::clear()
{
    return mIMetadata->clear();
}

MERROR MiMetadata::update(Tag_t tag, MiMetadata::MiEntry const &entry)
{
    return mIMetadata->update(tag, *entry.mIEntry);
}

MERROR MiMetadata::update(MiEntry const &entry)
{
    return mIMetadata->update(*entry.mIEntry);
}

MiMetadata::MiEntry MiMetadata::entryFor(Tag_t tag, MBOOL isTakeAway) const
{
    MiEntry entry;
    entry.mIEntry =
        std::make_shared<NSCam::IMetadata::IEntry>(mIMetadata->entryFor(tag, isTakeAway));
    return entry;
}

MiMetadata::MiEntry MiMetadata::takeEntryFor(Tag_t tag)
{
    return entryFor(tag, MTRUE);
}

MiMetadata::MiEntry MiMetadata::entryAt(MUINT index) const
{
    MiEntry entry;
    entry.mIEntry = std::make_shared<NSCam::IMetadata::IEntry>(mIMetadata->entryAt(index));
    return entry;
}

void MiMetadata::dump(int layer, bool forceOutput)
{
    mIMetadata->dump(layer, forceOutput);
}

////////////////////////////////////////
////////////////////////////////////////
// MiEntry 相关

MiMetadata::MiEntry::MiEntry(Tag_t tag)
{
    mIEntry = std::make_shared<NSCam::IMetadata::IEntry>(tag);
}

MiMetadata::MiEntry::MiEntry(MiEntry const &other)
{
    mIEntry = std::make_shared<NSCam::IMetadata::IEntry>(*other.mIEntry);
}

MiMetadata::MiEntry &MiMetadata::MiEntry::operator=(const MiEntry &other)
{
    auto &theOne = *this->mIEntry;
    auto &theOther = *other.mIEntry;
    theOne = theOther;
    return *this;
}

MiMetadata::MiEntry::~MiEntry()
{
    mIEntry = nullptr;
}

MiMetadata::Tag_t MiMetadata::MiEntry::tag() const
{
    return mIEntry->tag();
}

MINT32 MiMetadata::MiEntry::type() const
{
    return mIEntry->type();
}

MBOOL MiMetadata::MiEntry::isEmpty() const
{
    return mIEntry->isEmpty();
}

MUINT MiMetadata::MiEntry::count() const
{
    return mIEntry->count();
}

MVOID MiMetadata::MiEntry::clear()
{
    mIEntry->clear();
}

MERROR MiMetadata::MiEntry::removeAt(MUINT index)
{
    return mIEntry->removeAt(index);
}

const void *MiMetadata::MiEntry::data() const
{
    return mIEntry->data();
}

template <typename T, typename U>
MVOID MiMetadata::MiEntry::push_back(T const &item, Type2Type<U> type)
{
    mIEntry->push_back(item, type);
}

template <typename T, typename U>
MVOID MiMetadata::MiEntry::push_back(T const *array, size_t size, Type2Type<U> type)
{
    mIEntry->push_back(array, size, type);
}

template <typename T, typename U>
MVOID MiMetadata::MiEntry::replaceItemAt(MUINT index, T const &item, Type2Type<U> type)
{
    mIEntry->replaceItemAt(index, item, type);
}

template <typename T, typename U>
MVOID MiMetadata::MiEntry::replaceItemAt(MUINT index, T const *array, size_t size,
                                         Type2Type<U> type)
{
    mIEntry->replaceItemAt(index, array, size, type);
}

template <typename T>
T MiMetadata::MiEntry::itemAt(MUINT index, Type2Type<T> type) const
{
    return mIEntry->itemAt(index, type);
}

template <typename T>
MBOOL MiMetadata::MiEntry::itemAt(MUINT index, T *array, size_t size, Type2Type<T> type) const
{
    return mIEntry->itemAt(index, array, size, type);
}

template <typename T>
int MiMetadata::MiEntry::indexOf(MiEntry &entry, const T &target)
{
    return NSCam::IMetadata::IEntry::indexOf(*entry.mIEntry, target);
}

#define TEMPLATE_INSTANTIATION(T)                                                           \
    template bool MiMetadata::getEntry(const MiMetadata *metadata, Tag_t const tag, T &val, \
                                       size_t index);                                       \
    template MERROR MiMetadata::setEntry(MiMetadata *metadata, Tag_t const tag, T const &val);
#define TEMPLATE_INSTANTIATION(T)                                                           \
    template bool MiMetadata::getEntry(const MiMetadata *metadata, Tag_t const tag, T &val, \
                                       size_t index);                                       \
    template MERROR MiMetadata::setEntry(MiMetadata *metadata, Tag_t const tag, T const &val);

TEMPLATE_INSTANTIATION(MUINT8);
TEMPLATE_INSTANTIATION(MINT32);
TEMPLATE_INSTANTIATION(MFLOAT);
TEMPLATE_INSTANTIATION(MINT64);
TEMPLATE_INSTANTIATION(MDOUBLE);
TEMPLATE_INSTANTIATION(MRational);
TEMPLATE_INSTANTIATION(MPoint);
TEMPLATE_INSTANTIATION(MSize);
TEMPLATE_INSTANTIATION(MRect);
TEMPLATE_INSTANTIATION(NSCam::IMetadata::Memory);
TEMPLATE_INSTANTIATION(NSCam::IMetadata);

#define TEMPLATE_INSTANTIATION0(T, U) \
    template MVOID MiMetadata::MiEntry::push_back(T const &item, Type2Type<U> type);

#define TEMPLATE_INSTANTIATION1(T)                                                                 \
    template MVOID MiMetadata::MiEntry::push_back(T const &item, Type2Type<T> type);               \
    template MVOID MiMetadata::MiEntry::push_back(T const *array, size_t size, Type2Type<T> type); \
    template MVOID MiMetadata::MiEntry::replaceItemAt(MUINT index, T const &item,                  \
                                                      Type2Type<T> type);                          \
    template MVOID MiMetadata::MiEntry::replaceItemAt(MUINT index, T const *array, size_t size,    \
                                                      Type2Type<T> type);                          \
    template T MiMetadata::MiEntry::itemAt(MUINT index, Type2Type<T> type) const;                  \
    template MBOOL MiMetadata::MiEntry::itemAt(MUINT index, T *array, size_t size,                 \
                                               Type2Type<T> type) const;

#define TEMPLATE_INSTANTIATION2(T) \
    template int MiMetadata::MiEntry::indexOf(MiEntry &entry, const T &target);

// FIXME: those special case should be resolved
TEMPLATE_INSTANTIATION0(char, MUINT8);
TEMPLATE_INSTANTIATION0(bool, MUINT8);
TEMPLATE_INSTANTIATION0(MINT32, MUINT8);
TEMPLATE_INSTANTIATION0(MINT32, MINT64);
TEMPLATE_INSTANTIATION0(MINT32, MFLOAT);
TEMPLATE_INSTANTIATION0(MDOUBLE, MFLOAT);
TEMPLATE_INSTANTIATION0(bool, MINT32);
TEMPLATE_INSTANTIATION0(uint32_t, MINT32);
TEMPLATE_INSTANTIATION0(uint16_t, MINT32);
TEMPLATE_INSTANTIATION0(int64_t, MINT32);
TEMPLATE_INSTANTIATION0(uint64_t, MINT32);
TEMPLATE_INSTANTIATION0(uint64_t, MINT64);
TEMPLATE_INSTANTIATION0(unsigned int, MUINT8);
TEMPLATE_INSTANTIATION0(unsigned int, MINT64);
TEMPLATE_INSTANTIATION0(unsigned int, MFLOAT);
TEMPLATE_INSTANTIATION0(unsigned int, MDOUBLE);

//
TEMPLATE_INSTANTIATION1(MUINT8);
TEMPLATE_INSTANTIATION1(MINT32);
TEMPLATE_INSTANTIATION1(MFLOAT);
TEMPLATE_INSTANTIATION1(MINT64);
TEMPLATE_INSTANTIATION1(MDOUBLE);
TEMPLATE_INSTANTIATION1(MRational);
TEMPLATE_INSTANTIATION1(MPoint);
TEMPLATE_INSTANTIATION1(MSize);
TEMPLATE_INSTANTIATION1(MRect);
TEMPLATE_INSTANTIATION1(IMetadata::Memory);
TEMPLATE_INSTANTIATION1(IMetadata);

//
TEMPLATE_INSTANTIATION2(MUINT8);
TEMPLATE_INSTANTIATION2(MINT32);
TEMPLATE_INSTANTIATION2(MFLOAT);
TEMPLATE_INSTANTIATION2(MINT64);
TEMPLATE_INSTANTIATION2(MDOUBLE);

TEMPLATE_INSTANTIATION2(MPoint);
TEMPLATE_INSTANTIATION2(MSize);
TEMPLATE_INSTANTIATION2(MRect);
TEMPLATE_INSTANTIATION2(IMetadata::Memory);

}; // namespace mizone
