
#include <MiCamJsonUtils.h>
#include <gtest/gtest.h>

#include <fstream>
#include <iostream>

#include "rapidjson/Document.h"
#include "rapidjson/FileWriteStream.h"
#include "rapidjson/StringBuffer.h"
#include "rapidjson/Writer.h"
#include "rapidjson/error/En.h"

using namespace rapidjson;
using namespace std;

namespace mizone {

class MiCamJsonUtilsTest : public testing::Test
{
};

TEST_F(MiCamJsonUtilsTest, MiCamJsonUtilsFunctionTest)
{
    // MiCamJsonUtils::init();

    // NOTE: Read int
    std::string key1 = {"InternalPreviewBufferQueueSize"};
    int ret1;
    bool IsOK = MiCamJsonUtils::getVal(key1, ret1, 8);
    if (IsOK)
        cout << "get value: " << ret1 << endl;
    else
        cout << "get value fail" << endl;

    // NOTE: Read double
    // You can also search in this format:
    std::string key2 = {"InternalSnapshotStreamProperty.YuvStreamResolutionOptions.Tolerance"};
    double ret2;
    IsOK = MiCamJsonUtils::getVal(key2, ret2, 0.5);
    if (IsOK)
        cout << "get value: " << ret2 << endl;
    else
        cout << "get value fail" << endl;

    // NOTE: Read array
    // You can also search in this format:
    std::string key3 = {
        "InternalSnapshotStreamProperty.RawStreamResolutionOptions.AspectRatioList"};
    std::vector<double> ret3;
    IsOK = MiCamJsonUtils::getVal(key3, ret3, std::vector<double>{1, 2, 3, 4});
    if (IsOK) {
        for (auto ite = ret3.begin(); ite != ret3.end(); ++ite)
            cout << "get value: " << *ite << endl;
    } else
        cout << "get value fail" << endl;

    // NOTE: Read string
    // You can also search in this format:
    std::string key4 = {"ModeList.SatMode.InternalYuvStreamProperty.DefaultFormat"};
    std::string ret4;
    IsOK = MiCamJsonUtils::getVal(key4, ret4, "YUV");
    if (IsOK)
        cout << "get value: " << ret4 << endl;
    else
        cout << "get value fail" << endl;
}

} // namespace mizone
