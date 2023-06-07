#ifndef _GRALLOCUTILS_H_
#define _GRALLOCUTILS_H_

#include <cutils/native_handle.h>
#include <log/log.h>
#include <signal.h>

#define MI_ASSERT(cond, ...)                                                        \
    do {                                                                            \
        if (!(cond)) {                                                              \
            ALOGE("\nASSERTION FAILED: %s:%d --> %s\n", __FILE__, __LINE__, #cond); \
            ALOGE(" " __VA_ARGS__);                                                 \
            raise(SIGTRAP);                                                         \
        }                                                                           \
    } while (0)

struct GrallocBufHandle
{
    buffer_handle_t nativeHandle;
    void *backend;
};

class GrallocUtils
{
public:
    static void allocateGrallocBuffer(uint32_t width, uint32_t height, int format, uint64_t usage,
                                      GrallocBufHandle *handle, uint32_t *stride);

    static void createGrallocBuffer(uint32_t width, uint32_t height, int format, uint64_t usage,
                                    uint32_t stride, const buffer_handle_t buffer,
                                    GrallocBufHandle *handle);

    static void freeGrallocBuffer(GrallocBufHandle *handle);

    static void lockGrallocBuffer(GrallocBufHandle *handle, uint64_t usage, void **vaddr);

    static void unlockGrallocBuffer(GrallocBufHandle *handle);

    static void getGrallocBufferSize(buffer_handle_t bufferHandle, uint64_t *outAllocationSize);
};

#endif /* _GRALLOCUTILS_H_ */
