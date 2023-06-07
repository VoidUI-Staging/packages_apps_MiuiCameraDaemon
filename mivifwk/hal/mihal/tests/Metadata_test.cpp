#include <Metadata.h>
#include <gtest/gtest.h>

#include <set>

namespace mihal {

class MetadataTest : public testing::Test
{
};

TEST_F(MetadataTest, CreateMeta)
{
    Metadata meta{};
    printf("entry count:%d\n", meta.entryCount());

    int32_t test_request = 3;
    meta.update(ANDROID_REQUEST_ID, &test_request, 1);

    int32_t pixSize[] = {12000, 9000};
    meta.update(ANDROID_SENSOR_INFO_PIXEL_ARRAY_SIZE, pixSize, 2);

    printf("entry count after update: %d\n", meta.entryCount());
    printf("dump meta: %s\n", meta.toString().c_str());

    Metadata meta_other{};

    int32_t orientation = 0;
    meta_other.update(ANDROID_SENSOR_ORIENTATION, &orientation, 1);
    int32_t maxOut[] = {0, 1, 1};
    meta_other.update(ANDROID_REQUEST_MAX_NUM_OUTPUT_STREAMS, maxOut, 3);
    std::vector<uint8_t> cap{
        // ANDROID_REQUEST_AVAILABLE_CAPABILITIES_BACKWARD_COMPATIBLE,
        ANDROID_REQUEST_AVAILABLE_CAPABILITIES_PRIVATE_REPROCESSING,
        ANDROID_REQUEST_AVAILABLE_CAPABILITIES_BURST_CAPTURE,
        ANDROID_REQUEST_AVAILABLE_CAPABILITIES_YUV_REPROCESSING,
        // ANDROID_REQUEST_AVAILABLE_CAPABILITIES_SYSTEM_CAMERA,
    };
    meta_other.update(ANDROID_REQUEST_AVAILABLE_CAPABILITIES, cap);
    printf("dump meta_other: %s\n", meta_other.toString().c_str());

    meta.append(meta_other);
    printf("after appending, dump meta: %s\n", meta.toString().c_str());
}

} // namespace mihal
