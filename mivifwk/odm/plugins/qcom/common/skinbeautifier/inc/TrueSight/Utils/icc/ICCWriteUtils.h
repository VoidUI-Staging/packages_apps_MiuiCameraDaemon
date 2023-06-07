#pragma once

#include <string>
#include <TrueSight/TrueSightDefine.hpp>

namespace TrueSight {

class ProfileInfo;
class TrueSight_PUBLIC ICCWriteUtils {
public:
    static void AddProfile(const std::string &strImagePath, ProfileInfo *pProfileInfo);
    static int CopyJPGImageDisplayP3(std::string &src_name, std::string &dst_name);
};

}
