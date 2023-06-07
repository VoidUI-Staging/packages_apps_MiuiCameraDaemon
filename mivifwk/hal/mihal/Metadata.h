#ifndef __METADATA_H__
#define __METADATA_H__

#include <system/camera_metadata.h>
#include <system/camera_vendor_tags.h>

#include <memory>
#include <set>
#include <string>
#include <vector>

#include "XiaomiTags.h"
#include "XiaomiVendorTagDataDefines.h"

namespace android {
namespace hardware {
namespace camera {
namespace common {
namespace V1_0 {
namespace helper {

class CameraMetadata;

}
} // namespace V1_0
} // namespace common
} // namespace camera
} // namespace hardware
} // namespace android

namespace mihal {

// TODO: type and return error number
// introduce a new file including some basic types and return error number
using status_t = int;

// WHY we introduced this wrapper class Metadata?
// 1. to eliminate the dependency to the boring include and namespace when accessing CameraMetadata
// 2. more important reason is we need to add some helper functions but we do not want to
//    modify CameraMetadata implementation nor to derive from it
//
// note: go thouth CameraMetadata in Framework or Hardware to get the implementation detail
class Metadata final
{
public:
    enum OwnerShip {
        TakeOver, // in case you owned a raw meta buffer at first, you just want to manager it
                  // inside a Metadata object

        CopyOnWrite, // in case you probably just want to read, such as dump/find. but ocasionally
                     // need to write, make sure the source buffer is still alive before write

        CopyNow, // in case you want to copy/store a source buffer which is not yours
    };

    Metadata();
    Metadata(size_t entryCapacity, size_t dataCapacity = 10);

    ~Metadata();

    // create a Metadata object with 3 ownership options:
    // option 1: TakeOver: Takes ownership of passed-in buffer; then we must free it in dctor
    // option 2: CopyOnWrite: borrow and share the raw camera_metadata, copy before we write on
    //           it
    // option 3: CopyNow: copy a raw metadata rightnow, mostly because you may use it later after
    //           the original raw buffer is not alive, such as store a result metadata from the
    //           vendor hal after the process_result() callback from vendor hal returned
    Metadata(const camera_metadata *buffer, OwnerShip own = CopyOnWrite);

    // Clones the metadata
    Metadata(const Metadata &other);

    // TODO: Move constructor

    // Assignment clones metadata buffer.
    Metadata &operator=(const Metadata &other);
    Metadata &operator=(const camera_metadata *buffer);

    // TODO: Move assignment

    // Normally the global vendor tags ops is set by framework after it get the ops
    // from HAL. But now there are MiHAL and Vendor HAL, at the initialization phase
    // of MiHAL there is a period before framework setting the global that we cannot
    // map the name and tag. To solve this issue, we set the "global" ops firstly, then
    // framework will clear and set a final one
    static status_t setEarlyVendorTagOps(const vendor_tag_ops *ops);

    // Find tag id for a given tag name, also checking vendor tags if available.
    // On success, returns OK and writes the tag id into tag.
    //
    // This is a slow method.
    static status_t getTagFromName(const char *name, uint32_t *tag);

    // Get reference to the underlying metadata buffer. Ownership remains with
    // the Metadata object, but non-const Metadata methods will not
    // work until unlock() is called. Note that the lock has nothing to do with
    // thread-safety, it simply prevents the camera_metadata pointer returned
    // here from being accidentally invalidated by Metadata operations.
    const camera_metadata *getAndLock() const;

    // Unlock the Metadata for use again. After this unlock, the pointer
    // given from getAndLock() may no longer be used. The pointer passed out
    // from getAndLock must be provided to guarantee that the right object is
    // being unlocked.
    status_t unlock(const camera_metadata *buffer) const;

    // Release a raw metadata buffer to the caller. After this call,
    // Metadata no longer references the buffer, and the caller takes
    // responsibility for freeing the raw metadata buffer (using
    // free_camera_metadata()), or for handing it to another Metadata
    // instance.
    camera_metadata *release();

    // Get the raw metadata buffer for a temperary read-only usage, the Metadata
    // object is still the owner. caller should take care of the validation of the
    // raw buffer, since the Metadata object may modify or even free it
    const camera_metadata *peek() const;

    // Clear the metadata buffer and free all storage used by it
    void clear();

    // Acquires raw buffer from other Metadata object. After the call, the argument
    // object no longer has any metadata.
    void acquire(Metadata &other);

    // For performance concern, Metadata objects maybe created with CopyOnWrite policy
    // but some time later the user find the raw metadata need to be accessed after the
    // original raw data is dead. Then with safeguard() the user can do a force copy before
    // the raw is dead if it's a CopyOnWrite one
    void safeguard();

    // Append metadata from another Metadata object.
    status_t append(const Metadata &other);

    // Append metadata from a raw camera_metadata buffer
    status_t append(const camera_metadata *other);

    // Number of metadata entries.
    size_t entryCount() const;

    // Is the buffer empty (no entires)
    bool isEmpty() const;

    bool exists(uint32_t tag) const;
    bool exists(const char *name) const;

    // Sort metadata buffer for faster find
    status_t sort();

    status_t erase(uint32_t tag);
    status_t erase(const char *tag);

    // Swap the underlying camera metadata between this and the other
    // metadata object.
    void swap(Metadata &other);

    // Update metadata entry. Will create entry if it doesn't exist already, and
    // will reallocate the buffer if insufficient space exists. Overloaded for
    // the various types of valid data.
    status_t update(uint32_t tag, const uint8_t *data, size_t data_count);
    status_t update(uint32_t tag, const int32_t *data, size_t data_count);
    status_t update(uint32_t tag, const float *data, size_t data_count);
    status_t update(uint32_t tag, const int64_t *data, size_t data_count);
    status_t update(uint32_t tag, const double *data, size_t data_count);
    status_t update(uint32_t tag, const camera_metadata_rational *data, size_t data_count);

    template <typename T>
    status_t update(const char *name, const T *data, size_t data_count)
    {
        uint32_t tag;
        status_t ret;

        if ((ret = getTagFromName(name, &tag))) {
            return ret;
        }

        return update(tag, data, data_count);
    }

    status_t update(uint32_t tag, const char *str);
    status_t update(const char *name, const char *str);

    // TODO: A convenience helper function for some user structs base on predefined tag-function map
    // status_t update(uint32_t tag, const void *data);
    // status_t update(const char *name, const void *data);

    template <typename T>
    status_t update(uint32_t tag, const std::vector<T> &data)
    {
        return update(tag, data.data(), data.size());
    }

    template <typename T>
    status_t update(const char *name, const std::vector<T> &data)
    {
        uint32_t tag;
        status_t ret;

        if ((ret = getTagFromName(name, &tag))) {
            return ret;
        }

        return update(tag, data.data(), data.size());
    }

    status_t update(const camera_metadata_ro_entry &entry);

    // Update metadata from another Metadata object.
    status_t update(const Metadata &other);

    // Update metadata from a raw camera_metadata buffer
    status_t update(const camera_metadata *other);

    status_t update(const Metadata &other, uint32_t tag);
    status_t update(const Metadata &other, const char *tag);

    camera_metadata_entry find(uint32_t tag);
    camera_metadata_entry find(const char *name);

    camera_metadata_ro_entry find(uint32_t tag) const;
    camera_metadata_ro_entry find(const char *name) const;

#if 0
    // Dump contents into FD for debugging. The verbosity levels are
    // 0: Tag entry information only, no data values
    // 1: Level 0 plus at most 16 data values per entry
    // 2: All information
    //
    // The indentation parameter sets the number of spaces to add to the start
    // each line of output.
    void dump(int fd, int verbosity = 1, int indentation = 0) const;

    void dump(int fd, const std::vector<uint32_t> &sections, int verbosity = 1,
              int indentation = 0) const;
    void dump(int fd, const std::vector<const char *> &sections, int verbosity = 1,
              int indentation = 0) const;

    void dump(int fd, const std::vector<uint32_t> &tags, int verbosity = 1,
              int indentation = 0) const;
    void dump(int fd, const std::vector<const char *> &tags, int verbosity = 1,
              int indentation = 0) const;
#endif
    // Just dump the summary and what entries are inside the meta
    std::string toStringSummary() const;

    // toStringSummary() plus the value of the entries within the tags set/vector,
    // NOTE: if the tags set/vector is empty, then all the entries' value will be dumpped
    std::string toStringByTags(const std::set<uint32_t> &tags,
                               const std::vector<const char *> *names = nullptr) const;
    std::string toString() const;

private:
    // pronounce: [a'ton], not 'at one'
    void atone();
    void copyOnWrite();

    // Acquire a raw metadata buffer from the caller. After this call,
    // the caller no longer owns the raw buffer, and must not free or manipulate it.
    // If Metadata already contains metadata, it is freed.
    void acquire(camera_metadata *buffer);

    // Multiple operate entrance about each entry of another Metadata object.
    status_t forEachEntry(std::function<status_t(const camera_metadata_ro_entry &)>) const;

    std::unique_ptr<android::hardware::camera::common::V1_0::helper::CameraMetadata> mImpl;
    bool mOwnerOfRaw;
};

} // namespace mihal

#endif // __METADATA_H__
