#ifndef __MIVISETTINGS_H__
#define __MIVISETTINGS_H__

#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "LogUtil.h"
#include "rapidjson/Document.h"

namespace mihal {

// used when assigning JSON value to user supplied data
template <bool isConstructable, bool isConvertable>
struct Dispatcher
{
};

typedef Dispatcher<true, true> Constructable_Convertable;
typedef Dispatcher<true, false> Constructable_NotConvertable;
typedef Dispatcher<false, true> NotConstructable_Convertable;
typedef Dispatcher<false, false> NotConstructable_NotConvertable;

#define INVALIDFORMAT 0x404

enum CameraUser {
    MIUI = 0,
    SDK,
};

enum BokehCamSig {
    masterYuv = 0,
    masterRaw,
    slaveYuv,
    slaveRaw,
};

struct InternalStreamInfo
{
    uint32_t cameraId;
    uint32_t roleId;
    uint32_t width;
    uint32_t height;
    uint32_t usage;
    uint64_t stream_use_case;
    uint32_t space;
    float ratio;
    int format;
    int streamType;
    bool isPhysicalStream;
    bool isFakeSat;
    bool isRemosaic;
    BokehCamSig bokehSig;
    InternalStreamInfo() { memset(this, 0, sizeof(InternalStreamInfo)); }
};

struct PhgStreamFormat
{
    uint32_t phgType;
    int format;
};

typedef uint32_t FeatureMask_t;

struct InternalStreamInfoMap
{
    // sessionOpMode is specific to Ecology
    uint32_t sessionOpMode;
    uint32_t vendorOpMode;
    uint32_t mialgoOpMode;
    FeatureMask_t featureMask;
    uint32_t bufferLimitCnt;
    uint32_t snapshotBufferQueueSize;
    int64_t flushWaitTime;
    bool isVendorLowCaps;
    bool isVideoMode;
    std::string signature;
    std::vector<uint32_t> allowStreamsMask;
    std::vector<uint32_t> fpsRange;
    std::vector<InternalStreamInfo> streamInfos;
    std::set<uint32_t> supportFovCropCameras;
    InternalStreamInfoMap()
    {
        sessionOpMode = -1;
        vendorOpMode = -1;
        mialgoOpMode = -1;
        featureMask = 0;
        bufferLimitCnt = 8;
        snapshotBufferQueueSize = 8;
        flushWaitTime = 4000;
        isVendorLowCaps = false;
        isVideoMode = false;
        signature = "";
    }
};

enum FakeSatType {
    YUVIN = 1,
    YUVOUT = 2,
    JPEG = 3,
};

struct FakeSatSize
{
    uint32_t width;
    uint32_t height;
};

struct FakeSatThreshold
{
    float zoomStart;
    float zoomStop;
    float shortGain;
    float luxIndex;
    float adrcGain;
};

struct FakeSatConfigInfo
{
    FakeSatThreshold fakeSatThreshold;
    std::map<float /*FrameRatio*/, std::map<uint32_t, FakeSatSize>> ratioSizeMap;
};

struct XCFASnapshotThreshold
{
    float shortGain = -1;
    float luxIndex = -1;
    float adrcGain = -1;
    float darkRatio = -1;
    int8_t flashModeSingle = -1;
    int8_t flashModeTorch = -1;
};

struct CameraRoleConfigInfo
{
    std::map<uint32_t, std::vector<int>> phgStreamFormatMap;
    bool supportFakeSat;
    FakeSatConfigInfo fakeSatInfo;
    float fovCropZoomRatio;
    XCFASnapshotThreshold xcfaSnapshotThreshold;
};
typedef std::map<uint32_t, CameraRoleConfigInfo> CameraRoleConfigInfoMap;

class MiviSettings
{
public:
    template <typename... T>
    static bool getVal(T &&... args)
    {
        return getInstance()->getValue(std::forward<T>(args)...);
    }

    static bool init() { return getInstance()->isInit() ? true : false; }

    // For Debug Purpose
    static void printSettings();

    static InternalStreamInfoMap getVendorStreamInfoMapFromJson(const char *fileName,
                                                                uint32_t opMode, int lensFacing);

    static CameraRoleConfigInfoMap getCameraRoleConfigInfoFromJson();

private:
    static MiviSettings *getInstance();
    MiviSettings();
    ~MiviSettings() = default;
    bool initialize();
    bool isInit() { return mInited; }

    template <typename T>
    std::string toString(const T &val)
    {
        std::ostringstream str;
        str << val;
        return str.str();
    }

    template <typename T>
    std::string toString(const std::vector<T> &val, std::string prefix = "[ ",
                         std::string delimiter = ", ", std::string suffix = "]")
    {
        std::ostringstream str;
        str << prefix;
        for (const auto &element : val) {
            str << element << delimiter;
        }
        str << suffix;
        return str.str();
    }

    std::string toString(const rapidjson::Value &val)
    {
        std::ostringstream str;
        toStringImpl(val, str);
        return str.str();
    }

    void toStringImpl(const rapidjson::Value &val, std::ostringstream &str)
    {
        switch (val.GetType()) {
        case rapidjson::kNullType: {
            str << "NULL";
            break;
        }
        case rapidjson::kFalseType:
        case rapidjson::kTrueType: {
            str << val.GetBool();
            break;
        }
        case rapidjson::kObjectType: {
            str << "[MiviSettings] shouldn't be an object type";
            break;
        }
        case rapidjson::kArrayType: {
            str << "[ ";
            for (const auto &entry : val.GetArray()) {
                toStringImpl(entry, str);
                str << ", ";
            }
            str << "]";
            break;
        }
        case rapidjson::kStringType: {
            str << std::string(val.GetString());
            break;
        }
        case rapidjson::kNumberType: {
            str << (val.IsInt64() ? val.GetInt64() : val.GetDouble());
            break;
        }
        }
    }

    template <typename... T>
    bool getValue(const char *key, T &&... args)
    {
        return getValue(std::string(key), std::forward<T>(args)...);
    }

    // NOTE: To see examples of how to use this function, please refer to MiviSettings_test.cpp
    // @param:
    // key: is used to search the JSON tree and should be splite by '.' for example:
    // "aaa.aaa.aaa.aaa"
    template <typename... T>
    bool getValue(const std::string &key, T &&... args)
    {
        std::istringstream ss(key);
        std::string splitedKey;
        std::vector<std::string> finalKey;
        while (std::getline(ss, splitedKey, '.')) {
            finalKey.push_back(splitedKey);
        }

        return getValue(finalKey, std::forward<T>(args)...);
    }

    template <typename UserType, typename DefaultValType>
    bool AssignDefaultValue(UserType &retVal, DefaultValType defaultValue,
                            const std::vector<std::string> &key)
    {
        if (ProcessAssignmentDispatch(
                retVal, defaultValue,
                Dispatcher<std::is_constructible<UserType, DefaultValType>::value,
                           std::is_convertible<DefaultValType, UserType>::value>())) {
            MLOGD(LogGroupRT, "[MiviSettings] search key: %s FAIL. Assign deault value: %s",
                  toString(key, "", ".", "").c_str(), toString(defaultValue).c_str());
            return true;
        } else {
            MLOGW(LogGroupRT,
                  "[MiviSettings] default value is incompatible to user supplied data!!");
            return false;
        }
    }

    // @ brief: the implementation of MiviSettings::getVal
    // @param:
    // retVal: store the value found by the key
    // defaultVal: if search failed, then defaultVal is assign to retVal
    // @return: a  bool which indicates whether the search and assignment is  succeed.
    template <typename UserType, typename DefaultValType>
    bool getValue(const std::vector<std::string> &key, UserType &retVal,
                  DefaultValType defaultValue)
    {
        if (!isInit()) {
            MLOGW(LogGroupRT,
                  "[MiviSettings] Didn't find setting fie in odm/etc/camera/xiaomi/, use "
                  "default value");
            return AssignDefaultValue(retVal, defaultValue, key);
        }

        // NOTE: first we try to get JSON value by using user supplied search key
        rapidjson::Value value;
        if (!getJsonValue(key, 0, FinalConfig, value)) {
            MLOGW(LogGroupRT, "[MiviSettings] search Key: %s is invalid",
                  toString(key, "", ".", "").c_str());
            // NOTE: if the key is invalid, then we just assign the default value to user data
            return AssignDefaultValue(retVal, defaultValue, key);
        }

        // NOTE: if we get the JSON value, then try to assign JSON value to user supplied data
        if (!ProcessAssignment(retVal, value)) {
            // NOTE: if JSON value is incompatible with user data, then we use default value
            return AssignDefaultValue(retVal, defaultValue, key);
        }

        MLOGD(LogGroupRT, "[MiviSettings] search key: %s SUCCESS, value: %s",
              toString(key, "", ".", "").c_str(), toString(value).c_str());
        return true;
    }

    template <typename T>
    bool getJsonValue(const std::vector<std::string> &key, std::size_t index, const T &objectTree,
                      rapidjson::Value &retValue)
    {
        if (index == key.size()) {
            retValue = rapidjson::Value(objectTree, FinalConfig.GetAllocator());
            return true;
        }
        // NOTE: only the leaf node can be non object type
        const char *str = key[index].c_str();
        if (!objectTree.IsObject() || !objectTree.HasMember(str)) {
            MLOGW(LogGroupHAL, "[MiviSettings] can't find key: %s in json file", str);
            return false;
        }

        return getJsonValue(key, index + 1, objectTree[str], retValue);
    }

    // Lhs means "left hand side", Rhs means "Right hand side"
    // NOTE: if JSON value can be directedly converted to user data, then choose this dispatch
    template <typename LhsValType, typename RhsValType, bool T, bool U>
    bool ProcessAssignmentDispatch(LhsValType &retVal, const RhsValType &value,
                                   Dispatcher<T, U> dispatcher)
    {
        retVal = value;
        return true;
    }

    // NOTE: if JSON value can't convert to but can construct to user data, then choose this
    // dispatch
    template <typename LhsValType, typename RhsValType>
    bool ProcessAssignmentDispatch(LhsValType &retVal, const RhsValType &value,
                                   Constructable_NotConvertable dispatcher)
    {
        retVal = LhsValType(value);
        return true;
    }

    // NOTE: if JSON value can't convert and can't construct to user data, then choose this dispatch
    // and we will use default value
    template <typename LhsValType, typename RhsValType>
    bool ProcessAssignmentDispatch(LhsValType &retVal, const RhsValType &value,
                                   NotConstructable_NotConvertable dispatcher)
    {
        MLOGW(LogGroupRT,
              "[MiviSettings] data type is not compatible with user sipplied data type");
        MASSERT(false);
        return false;
    }

    // NOTE: process scalar(single) data assignment
    template <typename UserType>
    bool ProcessAssignment(UserType &retVal, rapidjson::Value &value)
    {
        switch (value.GetType()) {
        case rapidjson::kNullType: {
            MLOGW(LogGroupHAL, "[MiviSettings] JSON value is null!! use default value");
            return false;
        }

        case rapidjson::kFalseType:
        case rapidjson::kTrueType: {
            bool boolVal = value.GetBool();
            return ProcessAssignmentDispatch(
                retVal, boolVal,
                Dispatcher<std::is_constructible<UserType, bool>::value,
                           std::is_convertible<bool, UserType>::value>());
        }

        case rapidjson::kObjectType: {
            MLOGW(LogGroupHAL, "[MiviSettings] doesn't support assign JSON object value type!!");
            MASSERT(false);
            return false;
        }

        case rapidjson::kArrayType: {
            MLOGW(LogGroupHAL, "[MiviSettings] Please use std::vector to store JSON array value");
            MASSERT(false);
            return false;
        }

        case rapidjson::kStringType: {
            std::string strVal = value.GetString();
            return ProcessAssignmentDispatch(
                retVal, strVal,
                Dispatcher<std::is_constructible<UserType, std::string>::value,
                           std::is_convertible<std::string, UserType>::value>());
        }

        case rapidjson::kNumberType: {
            if (value.IsInt64()) {
                long intValue = value.GetInt64();
                return ProcessAssignmentDispatch(
                    retVal, intValue,
                    Dispatcher<std::is_constructible<UserType, long>::value,
                               std::is_convertible<long, UserType>::value>());
            } else {
                double doubleValue = value.GetDouble();
                return ProcessAssignmentDispatch(
                    retVal, doubleValue,
                    Dispatcher<std::is_constructible<UserType, double>::value,
                               std::is_convertible<double, UserType>::value>());
            }
        }
        }

        return false;
    }

    // NOTE: process vector data assignment
    template <typename UserType>
    bool ProcessAssignment(std::vector<UserType> &retVal, rapidjson::Value &value)
    {
        if (!value.IsArray()) {
            MLOGW(LogGroupHAL,
                  "[MiviSettings] User supplied a std::vector but JSON value is not array");
            return false;
        }
        for (auto ite = value.Begin(); ite != value.End(); ++ite) {
            retVal.emplace_back();
            // NOTE: use scalar(single) data assignment function to process each element's
            // assignment
            if (!ProcessAssignment(retVal[retVal.size() - 1], *ite)) {
                retVal.clear();
                return false;
            }
        }
        return true;
    }

    std::string readFile(const char *fileName);
    bool parseJsonFile(std::string &info, rapidjson::Document &doc);

    // @ brief: this function merges two config files
    // see design doc: https://xiaomi.f.mioffice.cn/docs/dock4CvlHAkZA3wS2NEU6jYnd1b#
    template <typename T>
    bool MergeTwoConfig(T &base, T &override)
    {
        if (!base.IsObject() || !override.IsObject()) {
            return false;
        }

        for (rapidjson::Value::MemberIterator itr = override.MemberBegin();
             itr != override.MemberEnd(); ++itr) {
            if (base.HasMember(itr->name)) {
                if (!MergeTwoConfig(base[itr->name], itr->value)) {
                    base[itr->name] = itr->value;
                }
            } else {
                base.AddMember(itr->name, itr->value, FinalConfig.GetAllocator());
            }
        }

        return true;
    }

    bool mInited;

    // NOTE: FinalConfig consists of configs in BaseJsonFile and OverrideJsonFile
    rapidjson::Document FinalConfig;
    rapidjson::Document OverrideConfig;
};
} // namespace mihal
#endif
