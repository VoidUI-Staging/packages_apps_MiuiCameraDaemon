
#include "Metadata.h"

#include <CameraMetadata.h>
#include <LogUtil.h>
#include <VendorTagDescriptor.h>
#include <string.h>
#include <utils/StrongPointer.h>

#include <algorithm>
#include <functional>
#include <iomanip>
#include <map>
#include <set>
#include <shared_mutex>
#include <sstream>

namespace mihal {

using android::hardware::camera::common::V1_0::helper::CameraMetadata;
using android::hardware::camera::common::V1_0::helper::VendorTagDescriptor;

namespace {

#define OK                                   0
#define CAMERA_METADATA_ENUM_STRING_MAX_SIZE 29

// NOTE: here we peek the internal entry structures for dump. although the internal implementation
// rarely changes, we still need to keep following the source
typedef struct camera_metadata_buffer_entry
{
    uint32_t tag;
    uint32_t count;
    union {
        uint32_t offset;
        uint8_t value[4];
    } data;
    uint8_t type;
    uint8_t reserved[3];
} camera_metadata_buffer_entry;

extern "C" {
camera_metadata_buffer_entry *get_entries(const camera_metadata *metadata);
uint8_t *get_data(const camera_metadata *metadata);
}

const char *get_indentation(int indentation)
{
    static const char *indents[] = {"", "\t", "\t\t", "\t\t\t"};

    int index = (indentation + 3) / 4;
    index = index > 3 ? 3 : index;

    return indents[index];
}

void print_data_(std::ostringstream &str, const uint8_t *data_ptr, uint32_t tag, int type,
                 int count, int indentation)
{
    static int values_per_line[NUM_TYPES] = {
        [TYPE_BYTE] = 32, [TYPE_INT32] = 8,  [TYPE_FLOAT] = 16,
        [TYPE_INT64] = 4, [TYPE_DOUBLE] = 8, [TYPE_RATIONAL] = 4,
    };
    size_t type_size = camera_metadata_type_size[type];
    char value_string_tmp[CAMERA_METADATA_ENUM_STRING_MAX_SIZE];
    uint32_t value;

    int lines = count / values_per_line[type];
    if (count % values_per_line[type] != 0)
        lines++;

    int index = 0;
    int j, k;
    std::ios::fmtflags f(str.flags()); // save old format
    for (j = 0; j < lines; j++) {
        str << get_indentation(indentation) << "[ ";
        for (k = 0; k < values_per_line[type] && count > 0; k++, count--, index += type_size) {
            switch (type) {
            case TYPE_BYTE:
                value = *(data_ptr + index);
                if (camera_metadata_enum_snprint(tag, value, value_string_tmp,
                                                 sizeof(value_string_tmp)) == OK) {
                    str << value_string_tmp << ' ';
                } else {
                    uint8_t show = value;
                    str << std::hex << std::setfill('0') << std::setw(2) << (uint32_t)show
                        << std::dec << ' ';
                }
                break;
            case TYPE_INT32:
                value = *(int32_t *)(data_ptr + index);
                if (camera_metadata_enum_snprint(tag, value, value_string_tmp,
                                                 sizeof(value_string_tmp)) == OK) {
                    str << value_string_tmp << ' ';
                } else {
                    str << std::hex << value << ' ';
                }
                break;
            case TYPE_FLOAT: {
                float show = *(float *)(data_ptr + index);
                str << show << ' ';
            } break;
            case TYPE_INT64: {
                int64_t show = *(int64_t *)(data_ptr + index);
                str << std::hex << show << ' ';
            } break;
            case TYPE_DOUBLE: {
                double show = *(double *)(data_ptr + index);
                str << show << ' ';
            } break;
            case TYPE_RATIONAL: {
                int32_t numerator = *(int32_t *)(data_ptr + index);
                int32_t denominator = *(int32_t *)(data_ptr + index + 4);
                str << "(" << numerator << " / " << denominator << ") ";
                break;
            }
            default:
                str << "??? ";
                break;
            }
        }
        str << "]\n";
    }
    str.flags(f); // format restored
}

void string_dump_indented_camera_metadata(const camera_metadata *metadata, std::ostringstream &str,
                                          int verbosity, int indentation,
                                          const std::set<uint32_t> &tags)
{
    if (metadata == NULL) {
        str << "Dumping camera metadata array: Not allocated" << std::endl;
        return;
    }

    unsigned int i;
    str << "Dumping camera metadata array: " << get_camera_metadata_entry_count(metadata) << " / "
        << get_camera_metadata_entry_capacity(metadata) << " entries, "
        << get_camera_metadata_data_count(metadata) << " / "
        << get_camera_metadata_data_capacity(metadata) << " bytes of data" << std::endl;

    size_t eCount = get_camera_metadata_entry_count(metadata);
    camera_metadata_buffer_entry *entry = get_entries(metadata);
    for (i = 0; i < eCount; i++, entry++) {
        if (tags.size() && !tags.count(entry->tag))
            continue;

        const char *tag_name, *tag_section;
        tag_section = get_local_camera_metadata_section_name(entry->tag, metadata);
        if (tag_section == NULL) {
            tag_section = "unknownSection";
        }
        tag_name = get_local_camera_metadata_tag_name(entry->tag, metadata);
        if (tag_name == NULL) {
            tag_name = "unknownTag";
        }
        const char *type_name;
        if (entry->type >= NUM_TYPES) {
            type_name = "unknown";
        } else {
            type_name = camera_metadata_type_names[entry->type];
        }

        str << get_indentation(indentation) << tag_section << "." << tag_name << " (" << std::hex
            << entry->tag << std::dec << "): " << type_name << "[" << entry->count << "]\n";

        if (verbosity < 1)
            continue;

        if (entry->type >= NUM_TYPES)
            continue;

        size_t type_size = camera_metadata_type_size[entry->type];
        uint8_t *data_ptr;
        if (type_size * entry->count > 4) {
            if (entry->data.offset >= get_camera_metadata_data_count(metadata)) {
                MLOGE(LogGroupHAL, "%s: Malformed entry data offset: %u (max %zu)", __FUNCTION__,
                      entry->data.offset, get_camera_metadata_data_count(metadata));
                continue;
            }
            data_ptr = get_data(metadata) + entry->data.offset;
        } else {
            data_ptr = entry->data.value;
        }
        int count = entry->count;
        if (verbosity < 2 && count > 16)
            count = 16;

        print_data_(str, data_ptr, entry->tag, entry->type, count, indentation);
    }
}

android::sp<VendorTagDescriptor> earlyTagDesc = nullptr;

// NOTE: global tag name to tag id cache, used to speed up tag id search
std::map<std::string, uint32_t> gNameTagIdCache;
// NOTE: read-write mutex to protect gNameTagIdCache
std::shared_mutex gNameTagIdCacheMutex;
} // namespace

Metadata::Metadata() : mImpl{std::make_unique<CameraMetadata>()}, mOwnerOfRaw{false} {}

Metadata::Metadata(size_t entryCapacity, size_t dataCapacity)
    : mImpl{std::make_unique<CameraMetadata>(entryCapacity, dataCapacity)}, mOwnerOfRaw{true}
{
}

Metadata::Metadata(const camera_metadata *buffer, OwnerShip own)
    : mImpl{std::make_unique<CameraMetadata>()}, mOwnerOfRaw{false}
{
    switch (own) {
    case TakeOver:
        // NOTE: avoid to get the error log in low-level implementation
        //       who complains the buffer is null, althought it is not
        //       serious actually
        if (buffer)
            mImpl->acquire(const_cast<camera_metadata *>(buffer));

        mOwnerOfRaw = true;
        break;
    case CopyOnWrite:
        if (buffer)
            mImpl->acquire(const_cast<camera_metadata *>(buffer));

        mOwnerOfRaw = false;
        break;
    case CopyNow:
        *mImpl = buffer;
        mOwnerOfRaw = true;
        break;
    default:
        break;
    }
}

Metadata::Metadata(const Metadata &other)
    : mImpl{std::make_unique<CameraMetadata>(*other.mImpl)}, mOwnerOfRaw{true}
{
}

Metadata::~Metadata()
{
    atone();
}

Metadata &Metadata::operator=(const Metadata &other)
{
    return operator=(other.peek());
}

Metadata &Metadata::operator=(const camera_metadata *buffer)
{
    atone();
    *mImpl = buffer;
    mOwnerOfRaw = true;

    return *this;
}

inline void Metadata::atone()
{
    if (!mOwnerOfRaw)
        release();
}

inline void Metadata::copyOnWrite()
{
    if (mOwnerOfRaw)
        return;

    const camera_metadata *raw = peek();
    operator=(raw);
}

status_t Metadata::setEarlyVendorTagOps(const vendor_tag_ops *ops)
{
    if (!earlyTagDesc) {
        VendorTagDescriptor::createDescriptorFromOps(ops, earlyTagDesc);
        if (!earlyTagDesc)
            return -ENOMEM;
    }

    VendorTagDescriptor::setAsGlobalVendorTagDescriptor(earlyTagDesc);

    return 0;
}

status_t Metadata::getTagFromName(const char *name, uint32_t *tag)
{
    std::string temp{name};
    // NOTE: find if this name is cached
    {
        std::shared_lock<std::shared_mutex> lg(gNameTagIdCacheMutex);
        auto itr = gNameTagIdCache.find(temp);
        if (itr != gNameTagIdCache.end()) {
            *tag = itr->second;
            return OK;
        }
    }

    android::sp<VendorTagDescriptor> vTagDesc = VendorTagDescriptor::getGlobalVendorTagDescriptor();
    if (vTagDesc != earlyTagDesc && earlyTagDesc)
        earlyTagDesc = nullptr;

    auto status = CameraMetadata::getTagFromName(name, vTagDesc.get(), tag);

    if (status == OK) {
        std::lock_guard<std::shared_mutex> lg(gNameTagIdCacheMutex);
        gNameTagIdCache[temp] = *tag;
    }

    return status;
}

const camera_metadata *Metadata::getAndLock() const
{
    return mImpl->getAndLock();
}

status_t Metadata::unlock(const camera_metadata *buffer) const
{
    return mImpl->unlock(buffer);
}

camera_metadata *Metadata::release()
{
    return mImpl->release();
}

const camera_metadata *Metadata::peek() const
{
    const camera_metadata *buffer = mImpl->getAndLock();
    mImpl->unlock(buffer);

    return buffer;
}

void Metadata::clear()
{
    atone();
    mImpl->clear();
}

void Metadata::acquire(camera_metadata *buffer)
{
    atone();
    mImpl->acquire(buffer);
}

void Metadata::acquire(Metadata &other)
{
    atone();
    mImpl->acquire(*other.mImpl);
    mOwnerOfRaw = other.mOwnerOfRaw;
    other.mOwnerOfRaw = false;
}

void Metadata::safeguard()
{
    copyOnWrite();
}

status_t Metadata::append(const Metadata &other)
{
    copyOnWrite();
    return mImpl->append(*other.mImpl);
}

status_t Metadata::append(const camera_metadata *other)
{
    copyOnWrite();
    return mImpl->append(other);
}

status_t Metadata::forEachEntry(
    std::function<status_t(const camera_metadata_ro_entry &)> func) const
{
    status_t ret;
    camera_metadata_ro_entry ro_entry;
    const camera_metadata *meta = peek();
    if (meta == nullptr)
        return OK;
    size_t eCount = get_camera_metadata_entry_count(meta);
    camera_metadata_buffer_entry *buffer_entry = get_entries(meta);

    for (uint32_t i = 0; i < eCount; i++, buffer_entry++) {
        ro_entry = find(buffer_entry->tag);
        ret = func(ro_entry);
        if (ret)
            return ret;
    }

    return OK;
}

status_t Metadata::update(const Metadata &other)
{
    return other.forEachEntry(
        [this](const camera_metadata_ro_entry &entry) { return update(entry); });
}

status_t Metadata::update(const camera_metadata *other)
{
    return update(Metadata(other));
}

size_t Metadata::entryCount() const
{
    return mImpl->entryCount();
}

bool Metadata::isEmpty() const
{
    return mImpl->isEmpty();
}

bool Metadata::exists(uint32_t tag) const
{
    return mImpl->exists(tag);
}

bool Metadata::exists(const char *name) const
{
    uint32_t tag;
    status_t ret;

    if ((ret = getTagFromName(name, &tag))) {
        // FIXME: notify or assert
        return false;
    }

    return mImpl->exists(tag);
}

status_t Metadata::sort()
{
    copyOnWrite();
    return mImpl->sort();
}

status_t Metadata::erase(uint32_t tag)
{
    copyOnWrite();
    return mImpl->erase(tag);
}

status_t Metadata::erase(const char *name)
{
    uint32_t tag;
    status_t ret;

    if ((ret = getTagFromName(name, &tag))) {
        return ret;
    }

    copyOnWrite();
    return mImpl->erase(tag);
}

void Metadata::swap(Metadata &other)
{
    mImpl->swap(*other.mImpl);
    std::swap(mOwnerOfRaw, other.mOwnerOfRaw);
}

status_t Metadata::update(uint32_t tag, const uint8_t *data, size_t data_count)
{
    copyOnWrite();
    return mImpl->update(tag, data, data_count);
}

status_t Metadata::update(uint32_t tag, const int32_t *data, size_t data_count)
{
    copyOnWrite();
    return mImpl->update(tag, data, data_count);
}

status_t Metadata::update(uint32_t tag, const float *data, size_t data_count)
{
    copyOnWrite();
    return mImpl->update(tag, data, data_count);
}

status_t Metadata::update(uint32_t tag, const int64_t *data, size_t data_count)
{
    copyOnWrite();
    return mImpl->update(tag, data, data_count);
}

status_t Metadata::update(uint32_t tag, const double *data, size_t data_count)
{
    copyOnWrite();
    return mImpl->update(tag, data, data_count);
}

status_t Metadata::update(uint32_t tag, const camera_metadata_rational *data, size_t data_count)
{
    copyOnWrite();
    return mImpl->update(tag, data, data_count);
}

status_t Metadata::update(uint32_t tag, const char *str)
{
    const void *vstr = str;
    camera_metadata_ro_entry entry = {
        .tag = tag,
        .type = TYPE_BYTE,
        .count = strlen(str) + 1,
        .data.u8 = static_cast<const uint8_t *>(vstr),
    };

    copyOnWrite();
    return mImpl->update(entry);
}

status_t Metadata::update(const char *name, const char *str)
{
    uint32_t tag;
    status_t ret;

    if ((ret = getTagFromName(name, &tag))) {
        return ret;
    }

    copyOnWrite();
    return update(tag, str);
}

status_t Metadata::update(const camera_metadata_ro_entry &entry)
{
    copyOnWrite();
    return mImpl->update(entry);
}

status_t Metadata::update(const Metadata &other, uint32_t tag)
{
    if (other.isEmpty()) {
        MLOGD(LogGroupHAL, "metadata is empty");
        return -ENOENT;
    }

    auto entry = other.find(tag);
    if (entry.count) {
        return update(entry);
    } else {
        MLOGD(LogGroupHAL, "entry count is 0");
        return -ENOENT;
    }
}

status_t Metadata::update(const Metadata &other, const char *tag)
{
    uint32_t tagId;
    status_t ret;
    if ((ret = getTagFromName(tag, &tagId))) {
        return ret;
    } else {
        return update(other, tagId);
    }
}

camera_metadata_entry Metadata::find(uint32_t tag)
{
    return mImpl->find(tag);
}

camera_metadata_entry Metadata::find(const char *name)
{
    uint32_t tag;
    status_t ret;

    if ((ret = getTagFromName(name, &tag))) {
        return camera_metadata_entry{.count = 0};
    }

    return mImpl->find(tag);
}

camera_metadata_ro_entry Metadata::find(uint32_t tag) const
{
    const CameraMetadata *meta = mImpl.get();
    return meta->find(tag);
}

camera_metadata_ro_entry Metadata::find(const char *name) const
{
    uint32_t tag;
    status_t ret;

    if ((ret = getTagFromName(name, &tag))) {
        return camera_metadata_ro_entry{.count = 0};
    }

    const CameraMetadata *meta = mImpl.get();
    return meta->find(tag);
}

std::string Metadata::toStringSummary() const
{
    std::ostringstream str;
    const camera_metadata *meta = peek();

    string_dump_indented_camera_metadata(meta, str, 0, 4, std::set<uint32_t>{});
    return str.str();
}

std::string Metadata::toStringByTags(const std::set<uint32_t> &tags,
                                     const std::vector<const char *> *names) const
{
    std::ostringstream str;
    const camera_metadata *meta = peek();

    std::set<uint32_t> tagSet{tags};
    if (names) {
        for (const char *tagStr : *names) {
            uint32_t tag;
            if (!getTagFromName(tagStr, &tag))
                tagSet.insert(tag);
        }
    }

    string_dump_indented_camera_metadata(meta, str, 2, 4, tagSet);
    return str.str();
}

std::string Metadata::toString() const
{
    return toStringByTags(std::set<uint32_t>{});
}

} // namespace mihal
