#ifndef __METAGETTER_H__
#define __METAGETTER_H__

#include <cutils/properties.h>
#include <system/camera_metadata.h>

#include <functional>
#include <vector>

#include "MiaPluginWraper.h"
#include "pointerDataAdapter.h"
#include "request.h"

#define LSC_TOTAL_CHANNELS    4
#define LSC_MESH_PT_H_V40     17
#define LSC_MESH_PT_V_V40     13
#define LSC_MESH_ROLLOFF_SIZE (LSC_MESH_PT_H_V40 * LSC_MESH_PT_V_V40)
#define AWB_ALGO_DECISION_MAX 3

#define initpDataList(buffer, metaSize, tag, type)     \
    do {                                               \
        std::vector<type> list(metaSize);              \
        for (unsigned char i = 0; i < metaSize; i++) { \
            list[i] = getMetaData(buffer + i, tag);    \
        }                                              \
        return list;                                   \
    } while (0)

typedef char CHAR;
typedef int INT;
typedef float FLOAT;
typedef int32_t CDKResult;
typedef int8_t INT8;
typedef uint8_t UINT8;
typedef int16_t INT16;
typedef uint16_t UINT16;
typedef int32_t INT32;
typedef uint32_t UINT32;
typedef int64_t INT64;
typedef uint64_t UINT64;
typedef INT32 BOOL;
#include "chioemvendortag.h"

/// @brief This structures encapsulate IFE /BPS Gamma G Channel Output
struct GammaInfo2
{
    BOOL isGammaValid; ///< Is Valid Gamm Entries
    UINT32 gammaG[65]; ///< Gamm Green Entries
};

class MetaData : public PointerDataAdapter
{
public:
    MetaData(const void *pData = nullptr) : PointerDataAdapter(pData) {}

    operator const ChiRect *() { return reinterpret_cast<const ChiRect *>(getPointer()); }

    operator const float *() { return reinterpret_cast<const float *>(getPointer()); }

    operator const AWBFrameControl *()
    {
        return reinterpret_cast<const AWBFrameControl *>(getPointer());
    }

    operator const GammaInfo2 *() { return reinterpret_cast<const GammaInfo2 *>(getPointer()); }

    operator enum CameraRoleId() { return getDataWithDefault<enum CameraRoleId>(RoleIdRearWide); }

    operator const FaceResult *() { return reinterpret_cast<const FaceResult *>(getPointer()); }
};

template <typename T, typename U>
std::string vector2exifstr(const char *name, std::vector<T> &vec)
{
    unsigned int i = 0;
    std::stringstream exifstr;
    exifstr << name << ':' << static_cast<U>(vec[i++]);
    while (i < vec.size())
        exifstr << ',' << static_cast<U>(vec[i++]);
    return exifstr.str();
}

template <typename T>
std::string vector2exifstr(const char *name, std::vector<T> &vec)
{
    unsigned int i = 0;
    std::stringstream exifstr;
    exifstr << name << ':' << vec[i++];
    while (i < vec.size())
        exifstr << ',' << vec[i++];
    return exifstr.str();
}
MetaData getMetaData(camera_metadata *, uint32_t);
MetaData getMetaData(camera_metadata *, const char *);
MetaData getMetaData(const ImageParams *buffer, uint32_t tag);
MetaData getMetaData(const ImageParams *buffer, const char *tag);
MetaData getMetaData(const ImageParams *buffer, unsigned char metaSize, const char *tag);
MetaData getMetaData(const ImageParams *buffer, unsigned char metaSize, uint32_t tag);
MetaData getMetaData(uint32_t staticMetadataIndex, uint32_t tag);
MetaData getMetaData(uint32_t staticMetadataIndex, const char *tag);

template <typename T = MetaData>
std::vector<T> getMetaDataList(const ImageParams *buffer, unsigned char metaSize, const char *tag)
{
    initpDataList(buffer, metaSize, tag, T);
}

template <typename T = MetaData>
std::vector<T> getMetaDataList(const ImageParams *buffer, unsigned char metaSize, uint32_t tag)
{
    initpDataList(buffer, metaSize, tag, T);
}

#define getSrEnable(metaData) getMetaData(metaData, "xiaomi.superResolution.enabled")
#define getColorCorrection(metaData, metaDataSize) \
    getMetaData(metaData, metaDataSize, "com.xiaomi.ccm.ColorCorrection")
#define getGreenGamma(metaData, metaDataSize) \
    getMetaData(metaData, metaDataSize, "org.quic.camera.gammainfo.GammaInfo")
#define getLensShadingMap(metaData, metaDataSize) \
    getMetaData(metaData, metaDataSize, ANDROID_STATISTICS_LENS_SHADING_MAP)
#define getAWBFrameControl(metaData, metaDataSize) \
    getMetaData(metaData, metaDataSize, "org.quic.camera2.statsconfigs.AWBFrameControl")
#define getBlackLevel(metaData, metaDataSize) \
    getMetaData(metaData, metaDataSize, ANDROID_SENSOR_DYNAMIC_BLACK_LEVEL)
#define getMode(metaData, metaDataSize) \
    getMetaData(metaData, metaDataSize, "xiaomi.ai.asd.sceneApplied")
#define getCropRegionSAT(metaData, metaDataSize) \
    getMetaData(metaData, metaDataSize, "xiaomi.snapshot.residualCropRegion")
#define getAnchorIndex(metaData, metaDataSize) \
    getMetaData(metaData, metaDataSize, "xiaomi.superResolution.pickAnchorIndex")
#define getHDSREnable(metaData, metaDataSize) \
    getMetaData(metaData, metaDataSize, "xiaomi.hdr.sr.enabled")
#define getISZState(metaData, metaDataSize) \
    getMetaData(metaData, metaDataSize, "xiaomi.superResolution.inSensorZoomState")
#define getFaceRect(metaData, metaDataSize) \
    getMetaData(metaData, metaDataSize, "xiaomi.snapshot.faceRect")
#define getFwkCameraId(buffer, count) getMetaData(buffer, count, "xiaomi.snapshot.fwkCameraId")
#define getInSensorZoom(metaData, metaDataSize) \
    getMetaData(metaData, metaDataSize, "xiaomi.superResolution.inSensorZoomState")

#define getRefSize(staticMetadataIndex) \
    getMetaData(staticMetadataIndex, ANDROID_SENSOR_INFO_ACTIVE_ARRAY_SIZE)
#define getRoleSAT(staticMetadataIndex) \
    getMetaData(staticMetadataIndex, "com.xiaomi.cameraid.role.cameraId")
#define getStyleType(metaData, metaDataSize) \
    getMetaData(metaData, metaDataSize, "com.xiaomi.sessionparams.stylizationType")

#define getJpegOrientation(metaData, metaDataSize) \
    getMetaData(metaData, metaDataSize, ANDROID_JPEG_ORIENTATION)

#define getExposureTime(metaData, metaDataSize, type) \
    getMetaDataList<type>(metaData, metaDataSize, ANDROID_SENSOR_EXPOSURE_TIME)
#define getSensitivity(metaData, metaDataSize) \
    getMetaDataList<int32_t>(metaData, metaDataSize, ANDROID_SENSOR_SENSITIVITY)
#define getIspGain(metaData, metaDataSize) \
    getMetaDataList(metaData, metaDataSize, "com.qti.sensorbps.gain")
#define getLensPosition(metaData, metaDataSize, type) \
    getMetaDataList<type>(metaData, metaDataSize, "xiaomi.exifInfo.lenstargetposition")
#define getIQEnabledMap(metaData, metaDataSize, type) \
    getMetaDataList<type>(metaData, metaDataSize, "xiaomi.superResolution.IPEIQEnabledMask")
#define getSensorModeindex(metaData, metaDataSize) \
    getMetaDataList<int32_t>(metaData, metaDataSize, "com.qti.sensorbps.mode_index")
#define getSensorModeCaps(metaData, metaDataSize) \
    getMetaDataList<int32_t>(metaData, metaDataSize, "com.qti.sensorbps.capability")
#define getFakeSat(metaData, metaDataSize) \
    getMetaDataList<int32_t>(metaData, metaDataSize, "xiaomi.FakeSat.enabled")
#define getZslSelected(metaData, metaDataSize) \
    getMetaData(metaData, metaDataSize, "xiaomi.superResolution.isZSLFrameSelected")

template <typename T = float>
std::vector<T> getIso(const ImageParams *buffer, unsigned char metaSize)
{
    unsigned char i = 0;
    std::vector<T> iso(metaSize);
    std::vector<int32_t> sensitivity = getSensitivity(buffer, metaSize);
    std::vector<MetaData> ispGain = getIspGain(buffer, metaSize);
    for (i = 0; i < metaSize; i++) {
        iso[i] = static_cast<T>(ispGain[i].getDataWithDefault<float>(1) * 2 * 100 * sensitivity[i] /
                                100);
    }
    return iso;
}
extern void getCropRegion(const ChiRect *cropRegion, const ChiRect *ref, uint32_t targetWidth,
                          uint32_t targetHeight, std::function<void(int, int, int, int)> setRect);

extern bool dumpToFileWithDir(const char *fileName, struct MiImageBuffer *imageBuffer,
                              const char *srDirName);

extern void dump(const ImageParams *buffer, unsigned int count, const char *algoName,
                 const char *srDirName, std::function<std::string(unsigned int)> getFileName);

extern void publishHDSRCropRegion(camera_metadata_t *metadata, int left, int top, int width,
                                  int height);

#endif
