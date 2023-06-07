/**
 * @file       test_dev_image.cpp
 * @brief
 * @details
 * @author
 * @date       2021.06.21
 * @version    0.1
 * @note
 * @warning
 * @par        History:
 *
 */

#include "../device.h"

static S32 m_Result = RET_OK;

#define DEV_IMAGE_TEST_WIDTH    128
#define DEV_IMAGE_TEST_HEIGHT   128

#define DEV_IMAGE_TEST_WIDTH_STRIDE         64
#define DEV_IMAGE_TEST_HEIGHT_SLICEHEIGHT   64

static DEV_IMAGE_BUF devImageTestSrc;
static DEV_IMAGE_BUF devImageTestDts;
static DEV_IMAGE_BUF devImageTestClone;
static DEV_IMAGE_BUF devImageTestReSize;

S32 test_dev_image(void) {
    m_Result = RET_OK;
    m_Result |= Device->image->Alloc(&devImageTestSrc, DEV_IMAGE_TEST_WIDTH, DEV_IMAGE_TEST_WIDTH + DEV_IMAGE_TEST_WIDTH_STRIDE,
    DEV_IMAGE_TEST_HEIGHT, DEV_IMAGE_TEST_HEIGHT + DEV_IMAGE_TEST_HEIGHT_SLICEHEIGHT, 0, (DEV_IMAGE_FORMAT) DEV_IMAGE_FORMAT_YUV420NV12,
            MARK("devImageTestSrc"));
    m_Result |= Device->image->Alloc(&devImageTestDts, DEV_IMAGE_TEST_WIDTH, DEV_IMAGE_TEST_WIDTH + DEV_IMAGE_TEST_WIDTH_STRIDE,
    DEV_IMAGE_TEST_HEIGHT, DEV_IMAGE_TEST_HEIGHT + DEV_IMAGE_TEST_HEIGHT_SLICEHEIGHT, 0, (DEV_IMAGE_FORMAT) DEV_IMAGE_FORMAT_YUV420NV12,
            MARK("devImageTestDts"));
    m_Result |= Device->image->Alloc(&devImageTestReSize, DEV_IMAGE_TEST_WIDTH * 2, DEV_IMAGE_TEST_WIDTH * 2 + DEV_IMAGE_TEST_WIDTH_STRIDE,
    DEV_IMAGE_TEST_HEIGHT * 2, DEV_IMAGE_TEST_HEIGHT * 2 + DEV_IMAGE_TEST_HEIGHT_SLICEHEIGHT, 0, (DEV_IMAGE_FORMAT) DEV_IMAGE_FORMAT_YUV420NV12,
            MARK("devImageTestReSize"));

    DEV_IMAGE_RECT rect = {10, 10, 10, 10};
    DEV_IMAGE_POINT point={15,15};

    m_Result |= Device->image->DrawPoint(&devImageTestSrc, point, 2, 5);
    m_Result |= Device->image->DrawRect(&devImageTestSrc, rect, 5, 5);
    m_Result |= Device->image->DrawRect(&devImageTestReSize, rect, 5, 5);

    m_Result |= Device->image->Copy(&devImageTestDts, &devImageTestSrc);
//    m_Result |= Device->image->ReSize(&devImageTestReSize, &devImageTestSrc);
    m_Result |= Device->image->Clone(&devImageTestClone, &devImageTestDts, MARK("devImageTestClone"));
    m_Result |= Device->image->Report();
    m_Result |= Device->memoryPool->Report();

    m_Result |= Device->image->DumpImage(&devImageTestSrc, "TEST_DEV_IMAGE_NODE", 1, "devImageTestSrc.NV12");
    m_Result |= Device->image->DumpImage(&devImageTestDts, "TEST_DEV_IMAGE_NODE", 1, "devImageTestDts.NV12");
    m_Result |= Device->image->DumpImage(&devImageTestClone, "TEST_DEV_IMAGE_NODE", 1, "devImageTestClone.NV12");
    m_Result |= Device->image->DumpImage(&devImageTestReSize, "TEST_DEV_IMAGE_NODE", 1, "devImageTestReSize.NV12");

    m_Result |= Device->image->Free(&devImageTestSrc);
    m_Result |= Device->image->Free(&devImageTestDts);
    m_Result |= Device->image->Free(&devImageTestClone);
    m_Result |= Device->image->Free(&devImageTestReSize);

    return m_Result;
}
