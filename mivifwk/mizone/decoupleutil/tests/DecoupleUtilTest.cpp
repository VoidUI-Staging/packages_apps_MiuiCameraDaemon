#include <gtest/gtest.h>

#include <cstring>
#include <iostream>
#include <iterator>
#include <ostream>
#include <string>
#include <vector>

#include "DecoupleUtil.h"
#include "mtkcam-halif/utils/metadata_tag/1.x/custom/custom_metadata_tag.h"
#include "mtkcam-halif/utils/metadata_tag/1.x/mtk_metadata_tag.h"

void createMetadata(mizone::MiMetadata &data)
{
    mizone::MiMetadata::setEntry<MUINT8>(&data, MTK_COLOR_CORRECTION_MODE,
                                         MTK_COLOR_CORRECTION_MODE_FAST);
    mizone::MiMetadata::setEntry<MFLOAT>(&data, MTK_COLOR_CORRECTION_GAINS, 1.2);
    NSCam::MRational mr(1, 2);
    mizone::MiMetadata::setEntry<NSCam::MRational>(&data, MTK_COLOR_CORRECTION_TRANSFORM, mr);
    mizone::MiMetadata::setEntry<MINT64>(&data, MTK_FLASH_INFO_CHARGE_DURATION, 10);
    mizone::MiMetadata::setEntry<MUINT8>(&data, MTK_QUIRKS_USE_PARTIAL_RESULT, 32);
}

void createStream(std::vector<mizone::Stream> &src)
{
    for (int i = 0; i < 3; i++) {
        mizone::Stream s;
        s.id = 10;
        s.streamType = mizone::StreamType::OUTPUT;
        s.width = 2560;
        s.height = 1000;
        s.format = mizone::eImgFmt_NV12;
        s.usage = 21;
        s.dataSpace = mizone::Dataspace::BT2020_PQ;
        s.transform = 100;
        s.physicalCameraId = 1;
        s.bufferSize = 5;
        createMetadata(s.settings);
        s.bufPlanes.count = 1;
        s.bufPlanes.planes[0].sizeInBytes = 2560;
        src.push_back(s);
    }
}

void judgeStreams(std::vector<mizone::Stream> &src, std::vector<mizone::Stream> &dst)
{
    for (int i = 0; i < src.size(); i++) {
        auto &s1 = src[i];
        auto &s2 = dst[i];
        GTEST_ASSERT_EQ(s1.bufPlanes.count, s2.bufPlanes.count);
        GTEST_ASSERT_EQ(s1.bufferSize, s2.bufferSize);
        GTEST_ASSERT_EQ(s1.dataSpace, s2.dataSpace);
        GTEST_ASSERT_EQ(s1.format, s2.format);
        GTEST_ASSERT_EQ(s1.height, s2.height);
        GTEST_ASSERT_EQ(s1.id, s2.id);
        GTEST_ASSERT_EQ(s1.physicalCameraId, s2.physicalCameraId);
        GTEST_ASSERT_EQ(s1.settings, s2.settings);
        GTEST_ASSERT_EQ(s1.usage, s2.usage);
        GTEST_ASSERT_EQ(s1.transform, s2.transform);
        GTEST_ASSERT_EQ(s1.streamType, s2.streamType);
    }
}

////////////////////
// configureStream阶段的测试

TEST(DecoupleUtilTest, convertStreamConfiguration)
{
    mizone::MiMetadata data;
    createMetadata(data);
    mizone::StreamConfiguration src, dst1;
    src.operationMode = mizone::StreamConfigurationMode::VENDOR_MODE_1;
    src.streamConfigCounter = 10;
    src.sessionParams = data.getIMetadata();
    mcam::StreamConfiguration dst;
    createStream(src.streams);
    mizone::DecoupleUtil::convertStreamConfiguration(src, dst);
    mizone::DecoupleUtil::convertStreamConfiguration(dst, dst1);

    float val = 0;
    mizone::MiMetadata tmp(dst1.sessionParams);
    mizone::MiMetadata::getEntry<float>(&tmp, MTK_COLOR_CORRECTION_GAINS, val);
    std::cout << "the float val is " << val << std::endl;

    GTEST_ASSERT_EQ(src.streamConfigCounter, dst1.streamConfigCounter);
    GTEST_ASSERT_EQ(src.operationMode, dst1.operationMode);
    GTEST_ASSERT_EQ(src.sessionParams, dst1.sessionParams);
}

TEST(DecoupleUtilTest, convertHalStreamConfiguration)
{
    mizone::HalStreamConfiguration src, dst;
    mcam::HalStreamConfiguration mid;
    src.streams.resize(1);
    src.streams[0].consumerUsage = 10;
    src.streams[0].producerUsage = 11;
    src.streams[0].id = 11;
    src.streams[0].maxBuffers = 100;
    src.streams[0].overrideDataSpace = mizone::Dataspace::DEPTH_32F_STENCIL_8;
    src.streams[0].overrideFormat = mizone::eImgFmt_AFBC_NV21;
    src.streams[0].physicalCameraId = 111;
    createMetadata(src.streams[0].results);
    createMetadata(src.sessionResults);
    mizone::DecoupleUtil::convertHalStreamConfiguration(src, mid);
    mizone::DecoupleUtil::convertHalStreamConfiguration(mid, dst);
    GTEST_ASSERT_EQ(src.sessionResults, dst.sessionResults);
    GTEST_ASSERT_EQ(src.streams[0].consumerUsage, dst.streams[0].consumerUsage);
    GTEST_ASSERT_EQ(src.streams[0].producerUsage, dst.streams[0].producerUsage);
    GTEST_ASSERT_EQ(src.streams[0].id, dst.streams[0].id);
    GTEST_ASSERT_EQ(src.streams[0].maxBuffers, dst.streams[0].maxBuffers);
    GTEST_ASSERT_EQ(src.streams[0].overrideDataSpace, dst.streams[0].overrideDataSpace);
    GTEST_ASSERT_EQ(src.streams[0].overrideFormat, dst.streams[0].overrideFormat);
    GTEST_ASSERT_EQ(src.streams[0].physicalCameraId, dst.streams[0].physicalCameraId);
    GTEST_ASSERT_EQ(src.streams[0].results, dst.streams[0].results);
}

/////////////////////////////////
// request阶段测试
//

void createStreamBuffer(std::vector<mizone::StreamBuffer> &src)
{
    src.resize(6);

    for (int i = 0; i < 6; i++) {
        src[i].acquireFenceFd = 1;
        src[i].releaseFenceFd = 12;
        src[i].bufferId = 12;
        src[i].status = mizone::BufferStatus::ERROR;
        src[i].buffer = std::make_shared<mizone::MiImageBuffer>();
        createMetadata(src[i].bufferSettings);
    }
}

void judgeStreamBuffer(std::vector<mizone::StreamBuffer> &src,
                       std::vector<mizone::StreamBuffer> &dst)
{
    for (int i = 0; i < src.size(); i++) {
        GTEST_ASSERT_EQ(src[i].acquireFenceFd, dst[i].acquireFenceFd);
        GTEST_ASSERT_EQ(src[i].releaseFenceFd, dst[i].releaseFenceFd);
        GTEST_ASSERT_EQ(src[i].bufferId, dst[i].bufferId);
        GTEST_ASSERT_EQ(src[i].status, dst[i].status);
        GTEST_ASSERT_EQ(src[i].status, dst[i].status);
    }
}

TEST(DecoupleUtilTest, request)
{
    mizone::CaptureRequest src, dst;
    mcam::CaptureRequest mid;
    src.frameNumber = 5000;
    createStreamBuffer(src.inputBuffers);
    createStreamBuffer(src.outputBuffers);
    createMetadata(src.settings);
    createMetadata(src.halSettings);
    src.physicalCameraSettings.resize(1);
    src.physicalCameraSettings[0].physicalCameraId = 12;
    createMetadata(src.physicalCameraSettings[0].halSettings);
    createMetadata(src.physicalCameraSettings[0].settings);
    src.subRequests.resize(5);

    for (int i = 0; i < 5; i++) {
        src.subRequests[i].subFrameIndex = i;
        createMetadata(src.subRequests[i].halSettings);
        createMetadata(src.subRequests[i].settings);
    }
    mizone::DecoupleUtil::convertOneRequest(src, mid);
    mizone::DecoupleUtil::convertOneRequest(mid, dst);

    GTEST_ASSERT_EQ(src.frameNumber, dst.frameNumber);
    GTEST_ASSERT_EQ(src.settings, dst.settings);
    GTEST_ASSERT_EQ(src.halSettings, dst.halSettings);
    GTEST_ASSERT_EQ(src.physicalCameraSettings[0].halSettings,
                    dst.physicalCameraSettings[0].halSettings);
    GTEST_ASSERT_EQ(src.physicalCameraSettings[0].settings, dst.physicalCameraSettings[0].settings);
    GTEST_ASSERT_EQ(src.physicalCameraSettings[0].physicalCameraId,
                    dst.physicalCameraSettings[0].physicalCameraId);
    judgeStreamBuffer(src.inputBuffers, dst.inputBuffers);
    judgeStreamBuffer(src.outputBuffers, dst.outputBuffers);

    for (int i = 0; i < 5; i++) {
        GTEST_ASSERT_EQ(src.subRequests[i].halSettings, dst.subRequests[i].halSettings);
        GTEST_ASSERT_EQ(src.subRequests[i].settings, dst.subRequests[i].settings);
        GTEST_ASSERT_EQ(src.subRequests[i].subFrameIndex, dst.subRequests[i].subFrameIndex);
    }
}

/////////////////////////////////
// result阶段测试
//
TEST(DecoupleUtilTest, result)
{
    mizone::CaptureResult src, dst;
    mcam::CaptureResult mid;
    src.frameNumber = 11;
    src.bLastPartialResult = false;
    createMetadata(src.halResult);
    createMetadata(src.result);
    createStreamBuffer(src.inputBuffers);
    createStreamBuffer(src.outputBuffers);
    mizone::DecoupleUtil::convertCaptureResult(src, mid);
    mizone::DecoupleUtil::convertCaptureResult(mid, dst);
    GTEST_ASSERT_EQ(src.frameNumber, dst.frameNumber);
    GTEST_ASSERT_EQ(src.bLastPartialResult, dst.bLastPartialResult);
    GTEST_ASSERT_EQ(src.result, dst.result);
    GTEST_ASSERT_EQ(src.halResult, dst.halResult);
    judgeStreamBuffer(src.inputBuffers, dst.inputBuffers);
    judgeStreamBuffer(src.outputBuffers, dst.outputBuffers);
}

/////////////////////
// mtk与android之间的metadata的测试

TEST(DecoupleUtilTest, mtkAndAndroidMetadata)
{
    mizone::MiMetadata data;
    createMetadata(data);

    size_t size = 0;
    camera_metadata *res = nullptr;
    IMetadata mtkMetadata = data.getIMetadata();
    static ::android::sp<NSCam::IMetadataConverter> sMetadataConverter =
        NSCam::IMetadataConverter::createInstance(
            NSCam::IDefaultMetadataTagSet::singleton()->getTagSet());
    sMetadataConverter->convert(mtkMetadata, res, &size);

    IMetadata dst;

    sMetadataConverter->convert(res, dst);

    GTEST_ASSERT_EQ(mtkMetadata.count(), dst.count());

    mizone::MiMetadata M1(mtkMetadata);
    mizone::MiMetadata M2(dst);
    float val = 0;
    mizone::MiMetadata tmp(M2);
    mizone::MiMetadata::getEntry<float>(&tmp, MTK_COLOR_CORRECTION_GAINS, val);
    std::cout << "the M2 float val is " << val << std::endl;
    GTEST_ASSERT_EQ(M1, M2);
    // for(int i=0;i<dst.count();i++)
    // GTEST_ASSERT_EQ(mtkMetadata.entryAt(0).data(),dst.entryAt(0).data());
    // GTEST_ASSERT_EQ(mtkMetadata.entryAt(1).data(),dst.entryAt(1).data());
    // GTEST_ASSERT_EQ(mtkMetadata.entryAt(2).data(),dst.entryAt(2).data());
}

// Notify相关测试
TEST(DecoupleUtilTest, NotifyMsgConvertShutter)
{
    mizone::NotifyMsg miMsg{};
    miMsg.type = mizone::MsgType::SHUTTER;
    miMsg.msg.shutter = mizone::ShutterMsg{
        .frameNumber = 1234,
        .timestamp = 12345678,
    };

    mcam::NotifyMsg mtkMsg = mizone::DecoupleUtil::convertOneNotify(miMsg);
    GTEST_ASSERT_EQ(mtkMsg.type, mcam::MsgType::SHUTTER);
    GTEST_ASSERT_EQ(mtkMsg.msg.shutter.frameNumber, miMsg.msg.shutter.frameNumber);
    GTEST_ASSERT_EQ(mtkMsg.msg.shutter.timestamp, miMsg.msg.shutter.timestamp);

    mizone::NotifyMsg convertedMiMsg = mizone::DecoupleUtil::convertOneNotify(mtkMsg);
    GTEST_ASSERT_EQ(convertedMiMsg.type, miMsg.type);
    GTEST_ASSERT_EQ(convertedMiMsg.msg.shutter.frameNumber, miMsg.msg.shutter.frameNumber);
    GTEST_ASSERT_EQ(convertedMiMsg.msg.shutter.timestamp, miMsg.msg.shutter.timestamp);
}

TEST(DecoupleUtilTest, NotifyMsgConvertError)
{
    mizone::NotifyMsg miMsg{};
    miMsg.type = mizone::MsgType::ERROR;
    miMsg.msg.error = mizone::ErrorMsg{.errorCode = mizone::ErrorCode::ERROR_RESULT,
                                       .errorStreamId = 12345678,
                                       .frameNumber = 5000};

    mcam::NotifyMsg mtkMsg = mizone::DecoupleUtil::convertOneNotify(miMsg);
    mizone::NotifyMsg convertedMiMsg = mizone::DecoupleUtil::convertOneNotify(mtkMsg);
    GTEST_ASSERT_EQ(convertedMiMsg.type, miMsg.type);
    GTEST_ASSERT_EQ(convertedMiMsg.msg.error.errorCode, miMsg.msg.error.errorCode);
    GTEST_ASSERT_EQ(convertedMiMsg.msg.error.errorStreamId, miMsg.msg.error.errorStreamId);
    GTEST_ASSERT_EQ(convertedMiMsg.msg.error.frameNumber, miMsg.msg.error.frameNumber);
}

using namespace NSCam;
// MiMetadata 操作函数测试
TEST(DecoupleUtilTest, MiMetadataTest)
{
    mizone::MiMetadata data;
    if (mizone::MiMetadata::trySetMetadata<MUINT8>(&data, MTK_COLOR_CORRECTION_MODE,
                                                   MTK_COLOR_CORRECTION_MODE_FAST)) {
        std::cout << "ok" << std::endl;
    }
    MUINT8 value = 0;
    mizone::MiMetadata::tryGetMetadata(&data, MTK_COLOR_CORRECTION_MODE, value);
    std::cout << "the value is " << std::to_string(value) << std::endl;

    MDOUBLE test = 1.12345678;
    mizone::MiMetadata::trySetMetadata(&data, MTK_JPEG_GPS_COORDINATES, test);

    mizone::MiMetadata::tryGetMetadata(&data, MTK_JPEG_GPS_COORDINATES, test);
    std::cout << "the value is " << test << std::endl;

    // push 数组
    char str[10] = "hello";
    mizone::MiMetadata::updateEntry<char, MUINT8>(data, MTK_FLASH_INFO_AVAILABLE, str,
                                                  strlen(str) + 1);
    std::cout << "write seccusee" << std::endl;
    char dirName[10];
    mizone::MiMetadata::tryGetMetadataPtr(&data, MTK_FLASH_INFO_AVAILABLE, dirName);
    std::cout << dirName << std::endl;

    // push int 类型
    std::vector<int> region = {1, 1, 4000, 3000};
    mizone::MiMetadata::updateEntry(data, MTK_MULTI_CAM_CONFIG_SCALER_CROP_REGION, region.data(),
                                    region.size());
    std::vector<int> dirRegion(5);
    mizone::MiMetadata::tryGetMetadataPtr(&data, MTK_MULTI_CAM_CONFIG_SCALER_CROP_REGION,
                                          dirRegion.data());
    for (auto &x : dirRegion) {
        std::cout << x << " ";
    }
    std::cout << std::endl;

    int size = 0;
    dirRegion.erase(dirRegion.begin(), dirRegion.end());
    mizone::MiMetadata::tryGetMetadataPtr(&data, MTK_MULTI_CAM_CONFIG_SCALER_CROP_REGION,
                                          dirRegion.data(), &size);
    for (int i = 0; i < size; i++) {
        std::cout << dirRegion[i] << " ";
    }
    std::cout << std::endl;

    // push MRect
    std::vector<NSCam::MRect> srcP, dstP;
    srcP.emplace_back(MPoint(1, 2), MPoint(0, 0));
    srcP.emplace_back(MPoint(99, 99), MPoint(0, 0));
    srcP.emplace_back(MPoint(10, 10), MPoint(0, 0));
    mizone::MiMetadata::updateEntry(data, MTK_SENSOR_INFO_OUTPUT_REGION_ON_ACTIVE_ARRAY,
                                    srcP.data(), srcP.size());
    dstP.resize(5);
    size = 0;
    mizone::MiMetadata::tryGetMetadataPtr(&data, MTK_SENSOR_INFO_OUTPUT_REGION_ON_ACTIVE_ARRAY,
                                          dstP.data(), &size);
    for (int i = 0; i < size; i++) {
        std::cout << dstP[i].leftTop().x << " " << dstP[i].leftTop().y << " "
                  << dstP[i].rightBottom().x << " " << dstP[i].rightBottom().y << std::endl;
        ;
    }
    std::cout << std::endl;

    // push memory
    struct node
    {
        int a;
        int *paddr;
        bool b;
        double c;
        inline node(int _a, bool _b, double _c) : a(_a), b(_b), c(_c){};
        inline node(){};
    };
    std::vector<node> srcN, dstN;
    srcN.emplace_back(1, 0, 10.1);
    srcN.emplace_back(22, 1, 199.12345);
    size = 0;
    mizone::MiMetadata::updateMemory(data, MTK_PRIVATE_3A_INFO, srcN[0]);
    dstN.resize(2);
    mizone::MiMetadata::tryGetMetadataPtr(&data, MTK_PRIVATE_3A_INFO, &dstN[0]);
    std::cout << "a:" << dstN[0].a << ",b:" << dstN[0].b << ",c:" << dstN[0].c << std::endl;

    mizone::MiMetadata::updateMemory(data, MTK_PRIVATE_3A_INFO, srcN[1]);
    mizone::MiMetadata::tryGetMetadataPtr(&data, MTK_PRIVATE_3A_INFO, &dstN[1]);
    std::cout << "a:" << dstN[1].a << ",b:" << dstN[1].b << ",c:" << dstN[1].c << std::endl;

    //测试double
    double dou_src = 1.234, dou_dst;
    mizone::MiMetadata::trySetMetadata(&data, MTK_JPEG_GPS_COORDINATES, dou_src);
    mizone::MiMetadata::tryGetMetadata(&data, MTK_JPEG_GPS_COORDINATES, dou_dst);
    std::cout << dou_dst << std::endl;
    GTEST_ASSERT_EQ(dou_src, dou_dst);
    std::vector<double> dou_vec_src{1.2, 3.4, 6.99}, dou_vec_dst;
    dou_vec_dst.resize(3);
    mizone::MiMetadata::updateEntry(data, MTK_JPEG_GPS_COORDINATES, dou_vec_src.data(),
                                    dou_vec_src.size());
    mizone::MiMetadata::tryGetMetadataPtr(&data, MTK_JPEG_GPS_COORDINATES, dou_vec_dst.data());
    GTEST_ASSERT_EQ(dou_vec_src, dou_vec_dst);
}
