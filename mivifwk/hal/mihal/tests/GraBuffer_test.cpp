#include <GraBuffer.h>
#include <gtest/gtest.h>

#include <memory>
#include <string>

namespace mihal {

class GraBufferTest : public testing::Test
{
};

TEST_F(GraBufferTest, CreateBuffer)
{
    auto buffer = std::make_unique<GraBuffer>(1024, 1, 35, 0x30, "gb-mihal");
    ASSERT_EQ(0, buffer->initCheck());
    printf("buffer: stride=%d\n", buffer->getStride());
    printf("dump buffer:\n%s\n", buffer->toString().c_str());
}

} // namespace mihal
