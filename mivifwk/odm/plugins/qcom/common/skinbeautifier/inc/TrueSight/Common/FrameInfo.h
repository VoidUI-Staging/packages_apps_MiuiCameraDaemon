#pragma once

#include <TrueSight/TrueSightDefine.hpp>

namespace TrueSight {

// camera type
enum CameraType {
    kCameraInvalid = -1,    // invalid type
    kCameraFront   = 0,     // front camera
    kCameraRear    = 1,     // rear camera
};

struct FrameInfoInner;
class TrueSight_PUBLIC FrameInfo {

public:
    FrameInfo();
    FrameInfo(const FrameInfo &rhs);
    FrameInfo(FrameInfo &&rhs) noexcept;
    ~FrameInfo();

    FrameInfo& operator=(const FrameInfo &rhs);
    FrameInfo& operator=(FrameInfo &&rhs) noexcept;

public:
    FrameInfo& setCameraType(CameraType camera_type);
    CameraType getCameraType();

    // iso when image captured, use default = 150 when unknown;
    FrameInfo& setISO(int iso);
    int getISO();

    // 室内室外Lux index参数  默认值 170(室外)
    FrameInfo& setLuxIndex(int lux_index);
    int getLuxIndex();

    // 是否使用闪光灯, 0否, 1是, 2常亮
    FrameInfo& setFlashTag(int flash_tag);
    int getFlashTag();

    // 是否使用闪光灯, 0否, 1是, 2常亮
    FrameInfo& setSoftLightTag(int tag);
    int getSoftLightTag();

    // 曝光时间;  default = 0.0
    FrameInfo& setExposure(float exposure);
    float getExposure();

    // adrc数值 正常范围为0.0-3.0
    FrameInfo& setAdrc(float adrc);
    float getAdrc();

    // 色温, 正常数值范围0-10000
    FrameInfo& setColorTemperature(int color_temperature);
    int getColorTemperature();

    void logOutput(const char* tag);
private:
    FrameInfoInner *inner_ = nullptr;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct FrameInfoInner {

    friend class FrameInfo;
    friend class XRealityImageImpl;
    friend class XRealityPreviewImpl;
private:
    FrameInfoInner();
    ~FrameInfoInner();

private:
    CameraType camera_type = kCameraInvalid;
    int iso = 150;                // iso when image captured, use default = 150 when unknown;
    int nLuxIndex = 170;          // 室内室外Lux index参数  默认值 170(室外)
    int nFlashTag = 0;            // 是否使用闪光灯, 0否, 1是, 2常亮
    int nSoftLightTag = 0;        // 是否使用柔光环, 0否, 1是, 2常亮
    float fExposure = 0.0f;       // 曝光时间;  default = 0.0
    float fAdrc = -0.0f;          // adrc数值 正常范围为0.0-3.0
    int nColorTemperature = -0;   // 色温, 正常数值范围0-10000
};

}
