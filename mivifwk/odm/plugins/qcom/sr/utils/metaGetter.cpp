#include "metaGetter.h"

#include <utils/Log.h>
#include <utils/String8.h>

#include "VendorMetadataParser.h"

using namespace std;

#define getMetadatListInitializer(buffer)                                     \
    {                                                                         \
        getPhysicalMetaDataPointer(buffer), getLogicalMetaDataPointer(buffer) \
    }

#define initBufferData(buffer, tag)                                                           \
    do {                                                                                      \
        camera_metadata_t *const metadataList[] = getMetadatListInitializer(buffer);          \
        for (unsigned int i = 0; i < (sizeof(metadataList) / sizeof(metadataList[0])); i++) { \
            const void *pData = getMetaData(metadataList[i], tag);                            \
            if (pData)                                                                        \
                return pData;                                                                 \
        }                                                                                     \
        return nullptr;                                                                       \
    } while (0)

#define initpData(buffer, metaSize, tag)                      \
    do {                                                      \
        for (unsigned char i = 0; i < metaSize; i++) {        \
            const void *pData = getMetaData(buffer + i, tag); \
            if (pData)                                        \
                return pData;                                 \
        }                                                     \
        return nullptr;                                       \
    } while (0)

MetaData getMetaData(const ImageParams *buffer, unsigned char metaSize, const char *tag)
{
    initpData(buffer, metaSize, tag);
}

MetaData getMetaData(const ImageParams *buffer, unsigned char metaSize, uint32_t tag)
{
    initpData(buffer, metaSize, tag);
}

MetaData getMetaData(const ImageParams *buffer, uint32_t tag)
{
    initBufferData(buffer, tag);
}

MetaData getMetaData(const ImageParams *buffer, const char *tag)
{
    initBufferData(buffer, tag);
}

MetaData getMetaData(camera_metadata *metaData, const char *tag)
{
    void *pData = nullptr;
    VendorMetadataParser::getVTagValue(metaData, tag, &pData);
    return pData;
}

MetaData getMetaData(camera_metadata *metaData, uint32_t tag)
{
    void *pData = nullptr;
    VendorMetadataParser::getTagValue(metaData, tag, &pData);
    return pData;
}

MetaData getMetaData(uint32_t staticMetadataIndex, uint32_t tag)
{
    return getMetaData(StaticMetadataWraper::getInstance()->getStaticMetadata(staticMetadataIndex),
                       tag);
}

MetaData getMetaData(uint32_t staticMetadataIndex, const char *tag)
{
    return getMetaData(StaticMetadataWraper::getInstance()->getStaticMetadata(staticMetadataIndex),
                       tag);
}

void getCropRegion(const ChiRect *cropRegion, const ChiRect *ref, uint32_t targetWidth,
                   uint32_t targetHeight, function<void(int, int, int, int)> setRect)
{
    if (cropRegion == NULL || ref == NULL) {
        MLOGW(Mia2LogGroupPlugin, "cropRegion is NULL, return");
        return;
    }

    int refWidth = ref->width, refHeight = ref->height;
    if (refWidth == 0) {
        // fallback to assume it's center-crop, and get ref size using cropRegion.
        refWidth = (cropRegion->left * 2) + cropRegion->width;
        refHeight = (cropRegion->top * 2) + cropRegion->height;
    }

    // clang-format off
    /************************************************************************************************
    +-----------------------------+
    |                             |
    |   +-------------------+     |    +-----------------------------+
    |   |                   |     |    |                             |
    |   |                   |     |    |   +-------------------+     |    +------------------+
    |   |                   |     |    |   |                   |     |    |  +-----------+   |
    |   |                   |     +--->+   |                   |     +--->+  |           |   |
    |   |                   |     |    |   |                   |     |    |  +-----------+   |
    |   |                   |     |    |   +-------------------+     |    +------------------+
    |   +-------------------+     |    |                             |
    |                             |    +-----------------------------+
    +-----------------------------+     16:9 for example:                 preZoom applied:
    sensorDomain(4:3)                   tmpCrop(after align aspectRatio)  (input buf domain)
    refWidth=6016,refHeight=4512        refWidth=6016,refHeight=3384      width=1600,height=900
    cropRegion=[1504,1128,3008,2256]    tmpCrop=[1504,846,3008,1692]      crop=[400,225,800,450]
    *************************************************************************************************/
    // clang-format on
    // align cropRegion's aspect ratio with input buffer
    float tmpLeft = cropRegion->left, tmpTop = cropRegion->top;
    float tmpWidth = cropRegion->width, tmpHeight = cropRegion->height;
    float xRatio = tmpWidth / targetWidth;
    float yRatio = tmpHeight / targetHeight;
    constexpr float tolerance = 0.0001f;

    if (yRatio > xRatio + tolerance) {
        tmpHeight = static_cast<float>(targetHeight) * xRatio;
        float tmpRefHeight = static_cast<float>(targetHeight) * refWidth / targetWidth;
        float delta = (refHeight - tmpRefHeight) / 2.0;
        tmpTop = cropRegion->top + (cropRegion->height - tmpHeight) / 2.0 - delta;
    } else if (xRatio > yRatio + tolerance) {
        tmpWidth = static_cast<float>(targetWidth) * yRatio;
        float tmpRefWidth = static_cast<float>(targetWidth) * refHeight / targetHeight;
        float delta = (refWidth - tmpRefWidth) / 2.0;
        tmpLeft = cropRegion->left + (cropRegion->width - tmpWidth) / 2.0 - delta;
    }

    // adjust crop according to preZoom into buffer domain
    unsigned int rectLeft, rectTop, rectWidth, rectHeight;
    float preZoomRatio_width = static_cast<float>(targetWidth) / refWidth;
    float preZoomRatio_height = static_cast<float>(targetHeight) / refHeight;
    float preZoomRatio = max(preZoomRatio_width, preZoomRatio_height);
    MLOGW(Mia2LogGroupPlugin, "preZoomRatio_width %f, preZoomRatio_height %f, preZoomRatio %f",
          preZoomRatio_width, preZoomRatio_height, preZoomRatio);
    rectWidth = tmpWidth * preZoomRatio;
    rectLeft = tmpLeft * preZoomRatio;
    rectHeight = tmpHeight * preZoomRatio;
    rectTop = tmpTop * preZoomRatio;

    // now we validate the rect, and ajust it if out-of-bondary found.
    if (rectLeft + rectWidth > targetWidth) {
        if (rectWidth <= targetWidth) {
            rectLeft = targetWidth - rectWidth;
        } else {
            rectLeft = 0;
            rectWidth = targetWidth;
        }
        MLOGW(Mia2LogGroupPlugin, "crop left or width is wrong, ajusted it manually!!");
    }
    if (rectTop + rectHeight > targetHeight) {
        if (rectHeight <= targetHeight) {
            rectTop = targetHeight - rectHeight;
        } else {
            rectTop = 0;
            rectHeight = targetHeight;
        }
        MLOGW(Mia2LogGroupPlugin, "crop top or height is wrong, ajusted it manually!!");
    }
    setRect(rectLeft, rectTop, rectWidth, rectHeight);

    MLOGI(Mia2LogGroupPlugin,
          "rect[%d,%d,%d,%d]->[%.2f,%.2f,%.2f,%.2f]->[%d,%d,%d,%d],"
          "target=%dx%d,ref=%dx%d",
          cropRegion->left, cropRegion->top, cropRegion->width, cropRegion->height, tmpLeft, tmpTop,
          tmpWidth, tmpHeight, rectLeft, rectTop, rectWidth, rectHeight, targetWidth, targetHeight,
          refWidth, refHeight);
}

bool dumpToFileWithDir(const char *fileName, struct MiImageBuffer *imageBuffer,
                       const char *srDirName)
{
    if (imageBuffer == NULL)
        return -1;
    char buf[256];
    char timeBuf[256];
    time_t currentTime;
    struct tm *timeInfo;
    static android::String8 lastFileName("");
    static int k = 0;
    time(&currentTime);
    timeInfo = localtime(&currentTime);
    memset(buf, 0, sizeof(buf));
    strftime(timeBuf, sizeof(timeBuf), "IMG_%Y%m%d%H%M%S", timeInfo);
    snprintf(buf, sizeof(buf), "%s%s_%s", srDirName, timeBuf, fileName);
    android::String8 filePath(buf);
    const char *suffix = "yuv";
    if (!filePath.compare(lastFileName)) {
        snprintf(buf, sizeof(buf), "_%d.%s", k++, suffix);
        MLOGD(Mia2LogGroupCore, "%s %s%s", __func__, filePath.string(), buf);
    } else {
        MLOGD(Mia2LogGroupCore, "%s diff %s %s", __func__, filePath.string(),
              lastFileName.string());
        k = 0;
        lastFileName = filePath;
        snprintf(buf, sizeof(buf), ".%s", suffix);
    }
    filePath.append(buf);
    PluginUtils::miDumpBuffer(imageBuffer, filePath);
    return 0;
}

void dump(const ImageParams *buffer, unsigned int count, const char *algoname,
          const char *srDirName, function<string(unsigned int)> getFileName)
{
    static const int32_t iIsDumpData = property_get_int32("persist.vendor.camera.srdump", 0);
    static const int32_t iIsDumpDataInDir =
        property_get_int32("persist.vendor.camera.srdumpmkdir", 0);
    if (iIsDumpData) {
        for (unsigned int i = 0; i < count; i++) {
            struct MiImageBuffer dumpBuffer = {getFormat(buffer + i),
                                               getWidth(buffer + i),
                                               getHeight(buffer + i),
                                               getStride(buffer + i),
                                               getScanline(buffer + i),
                                               0,
                                               {getPlane(buffer + i)[0], getPlane(buffer + i)[1]}};
            stringstream dumpNamestream;
            dumpNamestream << "sr_" << algoname << '_' << getWidth(buffer) << "x"
                           << getHeight(buffer) << '_' << getFileName(i) << '_' << i;
            if (iIsDumpDataInDir) {
                if (mkdir("data/vendor/camera/srdump/", 0777) != 0 && EEXIST != errno) {
                    MLOGE(Mia2LogGroupPlugin,
                          "Failed to create dir data/vendor/camera/srdump/ , Error: %d", errno);
                    return;
                }
                if (mkdir(srDirName, 0777) != 0 && EEXIST != errno) {
                    MLOGE(Mia2LogGroupPlugin, "Failed to create capture dumpfile , Error: %d",
                          errno);
                    return;
                }
                MLOGI(Mia2LogGroupPlugin, "sr create dump dir.");
                dumpToFileWithDir(dumpNamestream.str().c_str(), &dumpBuffer, srDirName);
            } else
                PluginUtils::dumpToFile(dumpNamestream.str().c_str(), &dumpBuffer);
        }
    }
}

void publishHDSRCropRegion(camera_metadata_t *metadata, int left, int top, int width, int height)
{
    static const char *const pSrHDRInfoTag = "xiaomi.ai.asd.SnapshotReqInfo";
    void *pSrHDRData = nullptr;
    size_t SrHDRTagCount = 0;
    VendorMetadataParser::getVTagValueCount(metadata, pSrHDRInfoTag, &pSrHDRData, &SrHDRTagCount);
    if (pSrHDRData) {
        SnapshotReqInfo srHDRSnapshotReqInfo;
        memcpy(&srHDRSnapshotReqInfo, pSrHDRData, sizeof(SnapshotReqInfo));
        srHDRSnapshotReqInfo.srCropRegion.left = left;
        srHDRSnapshotReqInfo.srCropRegion.top = top;
        srHDRSnapshotReqInfo.srCropRegion.width = width;
        srHDRSnapshotReqInfo.srCropRegion.height = height;

        if (!VendorMetadataParser::setVTagValue(metadata, pSrHDRInfoTag, &srHDRSnapshotReqInfo,
                                                SrHDRTagCount)) {
            MLOGI(Mia2LogGroupPlugin, "set crop region for sr-hdr: %d %d %d %d count %d",
                  srHDRSnapshotReqInfo.srCropRegion.left, srHDRSnapshotReqInfo.srCropRegion.top,
                  srHDRSnapshotReqInfo.srCropRegion.width, srHDRSnapshotReqInfo.srCropRegion.height,
                  SrHDRTagCount);
        } else {
            MLOGI(Mia2LogGroupPlugin, "set crop region tag error for sr-hdr");
        }
    }
}
