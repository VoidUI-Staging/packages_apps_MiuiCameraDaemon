/*
 * Copyright (c) 2021. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#ifndef _MIZONE_UTILS_H_
#define _MIZONE_UTILS_H_

// some common helper function, e.g., debug...

#include "MiZoneTypes.h"

namespace mizone::utils {

std::string toString(const Stream &stream);
std::string toString(const HalStream &stream);
std::string toString(const StreamConfiguration &conf);
std::string toString(const HalStreamConfiguration &conf);

std::string toString(const std::shared_ptr<MiImageBuffer> &buffer);
std::string toString(const StreamBuffer &buffer);
std::string toString(const NotifyMsg &msg);
std::string toString(const CaptureResult &result);
std::string toString(const SubRequest &subReq);
std::string toString(const CaptureRequest &request);

bool copyBuffer(std::shared_ptr<MiImageBuffer> src, std::shared_ptr<MiImageBuffer> dst,
                char const *callName);
bool dumpBuffer(const std::shared_ptr<MiImageBuffer> &buffer, const std::string &name);

template <typename T>
std::string toString(const std::vector<T> vec)
{
    std::stringstream ss;
    ss << "[ ";
    for (const auto &elem : vec) {
        ss << elem << ", ";
    }
    ss << "]";

    return ss.str();
};

std::string toString(const CameraOfflineSessionInfo &offlineSessionInfo);

} // namespace mizone::utils

#endif //_MIZONE_UTILS_H_
