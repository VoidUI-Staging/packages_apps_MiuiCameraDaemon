#ifndef MIMETADATA_H_
#define MIMETADATA_H_

#include <mtkcam-halif/utils/metadata/1.x/IMetadata.h>

#include <memory>

#include "mtkcam-halif/def/BuiltinTypes.h"
#include "mtkcam-interfaces/utils/metadata/mtk_metadata_types.h"
//#include "mtkcam-halif/def/BasicTypes.h"
//#include "mtkcam-halif/def/UITypes.h"
//#include "mtkcam-halif/def/TypeManip.h"
//#include "mtkcam-halif/def/Errors.h"

namespace mizone {

using namespace NSCam;

enum { // for MERROR return value
    MI_METADATA_OK = 0,
    MI_METADATA_FAIL = -1
};

class MiMetadata
{
public:
    class MiEntry;
    MiMetadata();
    MiMetadata(MiMetadata const &m);

    MiMetadata(IMetadata const &m);
    ~MiMetadata();

    IMetadata getIMetadata();
    typedef MUINT32 Tag_t;

    template <typename T>
    static bool getEntry(const MiMetadata *metadata, Tag_t const tag, T &val, size_t index = 0);

    template <typename T>
    static MERROR setEntry(MiMetadata *metadata, Tag_t const tag, T const &val);
    MERROR remove(Tag_t tag);
    MBOOL isEmpty() const;
    MUINT count() const;
    MVOID clear();
    MERROR update(Tag_t tag, MiEntry const &entry);
    MERROR update(MiEntry const &entry);
    MiEntry entryFor(Tag_t tag, MBOOL isTakeAway = MFALSE) const;
    MiEntry entryAt(MUINT index) const;
    MiEntry takeEntryFor(Tag_t tag);
    void dump(int layer = 0, bool forceOutput = false);
    MiMetadata &operator+=(MiMetadata const &other);
    MiMetadata &operator=(MiMetadata const &other);
    MiMetadata operator+(MiMetadata const &other);
    bool operator==(MiMetadata const &other) const;
    class MiEntry
    {
    public:
        enum { BAD_TAG = -1U };
        explicit MiEntry(Tag_t tag = BAD_TAG);
        MiEntry(MiEntry const &other);
        MiEntry &operator=(MiEntry const &other);
        ~MiEntry();
        Tag_t tag() const;
        MINT32 type() const;
        const void *data() const;
        MBOOL isEmpty() const;
        MUINT count() const;
        MVOID clear();
        MERROR removeAt(MUINT index);
        template <typename T, typename U>
        MVOID push_back(T const &item, Type2Type<U>);
        template <typename T, typename U>
        MVOID push_back(T const *array, size_t size, Type2Type<U>);
        template <typename T, typename U>
        MVOID replaceItemAt(MUINT index, T const &item, Type2Type<U>);
        template <typename T, typename U>
        MVOID replaceItemAt(MUINT index, T const *array, size_t size, Type2Type<U>);
        template <typename T>
        T itemAt(MUINT index, Type2Type<T>) const;
        template <typename T>
        MBOOL itemAt(MUINT index, T *array, size_t size, Type2Type<T>) const;
        template <typename T>
        static int indexOf(MiEntry &entry, const T &target);

    private:
        friend MiMetadata;
        std::shared_ptr<NSCam::IMetadata::IEntry> mIEntry = nullptr;
    };

private:
    friend class DecoupleUtil;
    std::shared_ptr<NSCam::IMetadata> mIMetadata = nullptr;

public:
    template <typename T>
    static inline bool tryGetMetadata(const MiMetadata *pMetadata, Tag_t const tag, T &rVal,
                                      int index = 0)
    {
        if (pMetadata == NULL) {
            return false;
        }
        //
        MiMetadata::MiEntry entry = pMetadata->entryFor(tag);
        int size = entry.count();
        if (size == 0 || index >= size) {
            return false;
        }
        rVal = entry.itemAt(index, Type2Type<T>());

        return true;
    }

    //* - TYPE_MUINT8
    //* - TYPE_MINT32
    //* - TYPE_MFLOAT
    //* - TYPE_MINT64
    //* - TYPE_MDOUBLE
    //* - TYPE_MRational
    //* - TYPE_MPoint
    //* - TYPE_MSize
    //* - TYPE_MRect
    //* - TYPE_IMetadata    Not verified
    //* - TYPE_Memory       verified only can read one Memory object
    template <typename T>
    static inline bool tryGetMetadataPtr(const MiMetadata *pMetadata, Tag_t const tag, T *addr,
                                         int *size = nullptr)
    {
        if (pMetadata == NULL) {
            return false;
        }

        MiMetadata::MiEntry entry = pMetadata->entryFor(tag);
        if (size) {
            *size = entry.count();
        }
        if (!entry.isEmpty()) {
            if (entry.type() == TYPE_Memory) {
                auto temp = entry.itemAt(0, Type2Type<IMetadata::Memory>());
                ::memcpy(addr, temp.editArray(), sizeof(T));
                return true;
            }
            ::memcpy(addr, entry.data(), entry.count() * sizeof(T));
            return true;
        }
        return false;
    }
    template <typename T>
    static inline bool trySetMetadata(MiMetadata *pMetadata, Tag_t const tag, T const &val)
    {
        if (pMetadata == NULL) {
            return false;
        }
        //
        MiMetadata::MiEntry entry(tag);
        entry.push_back(val, Type2Type<T>());
        return pMetadata->update(tag, entry) == 0;
    }

    // T is the tpye for value and U is the type for the tag
    template <typename T, typename U>
    static inline int updateEntry(MiMetadata &metadata, Tag_t entry_tag, T value)
    {
        MiMetadata::MiEntry entry(entry_tag);
        entry.push_back(value, Type2Type<U>());
        return metadata.update(entry_tag, entry);
    }

    template <typename T, typename U>
    static inline int updateEntry(MiMetadata &metadata, Tag_t entry_tag, const T *array, int size)
    {
        MiMetadata::MiEntry entry(entry_tag);
        for (int i = 0; i < size; i++) {
            entry.push_back(array[i], Type2Type<U>());
        }
        return metadata.update(entry_tag, entry);
    }

    // if value type is same with tag type
    template <typename T>
    static inline int updateEntry(MiMetadata &metadata, Tag_t entry_tag, T value)
    {
        MiMetadata::MiEntry entry(entry_tag);
        entry.push_back(value, Type2Type<T>());
        return metadata.update(entry_tag, entry);
    }

    template <typename T>
    static inline int updateEntry(MiMetadata &metadata, Tag_t entry_tag, const T *array, int size)
    {
        MiMetadata::MiEntry entry(entry_tag);
        for (int i = 0; i < size; i++) {
            entry.push_back(array[i], Type2Type<T>());
        }
        return metadata.update(entry_tag, entry);
    }

    template <typename T>
    static inline int updateMemory(MiMetadata &metadata, Tag_t entry_tag, const T &data)
    {
        NSCam::IMetadata::Memory rTmp;
        rTmp.resize(sizeof(T));
        ::memcpy(rTmp.editArray(), &data, sizeof(T));
        return updateEntry(metadata, entry_tag, rTmp);
    }
};

}; // namespace mizone

#endif
