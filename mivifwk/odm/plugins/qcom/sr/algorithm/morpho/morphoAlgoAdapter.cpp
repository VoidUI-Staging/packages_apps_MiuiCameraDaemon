#include <utils/Log.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "VendorMetadataParser.h"
#include "algoAdapter.h"
#include "device.h"
#include "metaGetter.h"
#include "morpho.h"

using namespace mialgo2;
using namespace std;

typedef struct
{
    int x;
    int y;
    int width;
    int height;
} sr_rect_t;

static uint32_t getFaceInfo(morpho_RectInt *faceRect, const Request *request,
                            const ChiRect *cropRegion, const ChiRect *ref, uint32_t targetWidth,
                            uint32_t targetHeight)
{
    const unsigned int inputCount = request->getInputCount();
    const ImageParams *const in = request->getInput();

    uint32_t faceNum = 0;
    const FaceResult *tmpData = getFaceRect(in, inputCount);
    // is 16:9 or 4:3 or 1:1?
    float xRatio = (float)cropRegion->width / (float)targetWidth;
    float yRatio = (float)cropRegion->height / (float)targetHeight;
    const float tolerance = 0.01f;
    MLOGI(Mia2LogGroupPlugin,
          "cropregion width:%u, target width:%u, xRatio:%f, cropregion height:%u, target "
          "height:%u, yRatio:%f",
          cropRegion->width, targetWidth, xRatio, cropRegion->height, targetHeight, yRatio);
    if (NULL != tmpData) {
        faceNum = (tmpData->faceNumber > MORPHO_HDSR_MAX_NUM_FACES ? MORPHO_HDSR_MAX_NUM_FACES
                                                                   : tmpData->faceNumber);
        for (int index = 0; index < faceNum; index++) {
            ChiRectEXT curFaceInfo = tmpData->chiFaceInfo[index];
            if (yRatio > xRatio + tolerance) {
                // 16:9 : height is cropped.
                int delta = (ref->height - targetHeight) / 2;
                faceRect[index] = {curFaceInfo.left, curFaceInfo.bottom - delta, curFaceInfo.right,
                                   curFaceInfo.top - delta};
                MLOGI(Mia2LogGroupPlugin,
                      "face 16:9 Index:%d final faceRect:[%d,%d,%d,%d] delta:%d face num:%u", index,
                      faceRect[index].sx, faceRect[index].sy, faceRect[index].ex,
                      faceRect[index].ey, delta, faceNum);
            } else if (xRatio > yRatio + tolerance) {
                // 1:1 : width is cropped
                int delta = (ref->width - targetWidth) / 2;
                faceRect[index] = {curFaceInfo.left - delta, curFaceInfo.bottom,
                                   curFaceInfo.right - delta, curFaceInfo.top};
                MLOGI(Mia2LogGroupPlugin,
                      "face 1:1 Index:%d final faceRect:[%d,%d,%d,%d] delta:%d face num:%u", index,
                      faceRect[index].sx, faceRect[index].sy, faceRect[index].ex,
                      faceRect[index].ey, delta, faceNum);
            } else {
                // 4:3 : "xiaomi.snapshot.faceRect" meta Top and Bottom need revert
                faceRect[index] = {curFaceInfo.left, curFaceInfo.bottom, curFaceInfo.right,
                                   curFaceInfo.top};
                MLOGI(Mia2LogGroupPlugin,
                      "face 4:3 Index:%d final faceRect:[%d,%d,%d,%d] face num:%u", index,
                      faceRect[index].sx, faceRect[index].sy, faceRect[index].ex,
                      faceRect[index].ey, faceNum);
            }
        }
    } else {
        MLOGI(Mia2LogGroupPlugin, "face is not found");
    }
    return faceNum;
}

static CameraRoleId getRole(const SessionInfo *sessionInfo, uint32_t fwkCameraId)
{
    if (sessionInfo->getOperationMode() == 0x8003)
        if (sessionInfo->getFrameworkCameraId() == 0x5)
            return RoleIdRearTele;
    return getRoleSAT(fwkCameraId);
}

static int getAnchor(const ImageParams *buffer, unsigned char metaSize)
{
#if HDSR
    bool hdsrEnable = getHDSREnable(buffer, metaSize);
    if (hdsrEnable)
        return metaSize - 1;
#endif
    return getAnchorIndex(buffer, metaSize).getDataWithDefault<int>(-1);
}

static void process(const AlgorithmAdapter *me, const SessionInfo *sessionInfo,
                    const Request *request)
{
    void *handle = nullptr;
    const unsigned int inputCount = request->getInputCount();
    const ImageParams *const in = request->getInput();
    const ImageParams *const out = request->getOutput();
    int anchor = getAnchor(in, inputCount);
    mialgo::Morpho *mMor;
    mMor = new mialgo::Morpho();
    char *cameraRoleName;
    struct MiImageBuffer inputFrame[inputCount], outputFrame;
    vector<int32_t> af = getLensPosition(in, inputCount, int32_t);

    // MiImageBuffer *output;
    for (int i = 0; i < inputCount; i++) {
        inputFrame[i].format = in->format.format;
        inputFrame[i].width = static_cast<uint32_t>(getWidth(in + i));
        inputFrame[i].height = static_cast<uint32_t>(getHeight(in + i));
        inputFrame[i].plane[0] = getPlane(in + i)[0];
        inputFrame[i].plane[1] = getPlane(in + i)[1];
        inputFrame[i].stride = static_cast<uint32_t>(getStride(in + i));
        inputFrame[i].scanline = static_cast<uint32_t>(getScanline(in + i));
        inputFrame[i].numberOfPlanes = 2;
    }
    outputFrame.format = out->format.format;
    outputFrame.width = static_cast<uint32_t>(getWidth(out));
    outputFrame.height = static_cast<uint32_t>(getHeight(out));
    outputFrame.plane[0] = getPlane(out)[0];
    outputFrame.plane[1] = getPlane(out)[1];
    outputFrame.stride = static_cast<uint32_t>(getStride(out));
    outputFrame.scanline = static_cast<uint32_t>(getScanline(out));

    // 1 morpho_HDSR_InitParams init_params
    morpho_HDSR_InitParams init_params = {0};
    sr_rect_t pRect[1] = {{0, 0, static_cast<int>(getWidth(in)), static_cast<int>(getHeight(in))}};
    uint32_t fwkCameraId = getFwkCameraId(in, inputCount);

    uint32_t insensorzoom = 0;
    insensorzoom = getInSensorZoom(in, inputCount);
    CameraRoleId cameraRole = getRole(sessionInfo, fwkCameraId);
    const ChiRect *cropRegion = getCropRegionSAT(in, inputCount);
    const ChiRect *ref = getRefSize(fwkCameraId);
    getCropRegion(cropRegion, ref, inputFrame[0].width, inputFrame[0].height,
                  [&pRect](int left, int top, int width, int height) mutable {
                      pRect->x = left;
                      pRect->y = top;
                      pRect->width = width;
                      pRect->height = height;
                  });

    MLOGI(Mia2LogGroupPlugin, "cameraRole:%d, insensorzoom:%d", cameraRole, insensorzoom);

    if (cameraRole == RoleIdRearWide) {
        if (2 == insensorzoom) {
            init_params.camera_id = 1;
            init_params.params_cfg_file =
                "/vendor/etc/camera/morpho_hdsr_tuning_params_XiaomiL1_WideRemosaic.xml";
            cameraRoleName = "Wide-Remosaic";
        } else {
            init_params.camera_id = 0;
            init_params.params_cfg_file =
                "/vendor/etc/camera/morpho_hdsr_tuning_params_XiaomiL1_WideBinning.xml";
            cameraRoleName = "Wide";
        }
    } else if (cameraRole == RoleIdRearTele4x) {
        if (2 == insensorzoom) {
            init_params.camera_id = 3;
            init_params.params_cfg_file =
                "/vendor/etc/camera/morpho_hdsr_tuning_params_XiaomiL1_TeleRemosaic.xml";
            cameraRoleName = "Tele-Remosaic";
        } else {
            init_params.camera_id = 2;
            init_params.params_cfg_file =
                "/vendor/etc/camera/morpho_hdsr_tuning_params_XiaomiL1_TeleBinning.xml";
            cameraRoleName = "Tele";
        }
    } else {
        init_params.camera_id = 0;
        init_params.params_cfg_file =
            "/vendor/etc/camera/morpho_hdsr_tuning_params_XiaomiL1_WideBinning.xml";
        cameraRoleName = "Wide";
    }

    init_params.width = inputFrame[0].width;
    init_params.height = inputFrame[0].height;
    init_params.stride = inputFrame[0].stride;
    init_params.output_width = outputFrame.width;
    init_params.output_height = outputFrame.height;
    init_params.num_input = inputCount;
    init_params.zoom_ratio = 0;

    morpho_RectInt morpho_rect;
    morpho_rect.sx = pRect->x;                 // left
    morpho_rect.sy = pRect->y;                 // top
    morpho_rect.ex = pRect->x + pRect->width;  // rect.right();
    morpho_rect.ey = pRect->y + pRect->height; // rect.bottom();
    init_params.zoom_rect = morpho_rect;
    init_params.format = MORPHO_IMAGE_FORMAT_YUV420_SEMIPLANAR;

    // 2a face rect
    morpho_RectInt faceRect[MORPHO_HDSR_MAX_NUM_FACES] = {{0}};
    uint32_t faceNum =
        getFaceInfo(faceRect, request, cropRegion, ref, init_params.width, init_params.height);

    // 2b zoomratio
    // after getcropregion(), target's crop regtion
    uint32_t tc_width = pRect->width;
    float zoomRatio = 1.0f;
    if (tc_width > 0) {
        zoomRatio = (float)init_params.width / tc_width;
        MLOGI(Mia2LogGroupPlugin, "zoomRatio:%f input_width:%d, rect_width:%d, cameraRoleName:%s",
              zoomRatio, init_params.width, tc_width, cameraRoleName);
    } else
        MLOGI(Mia2LogGroupPlugin, "get zoomRatio error! %f", zoomRatio);

    // 3 morpho_HDSR_MetaData morphometa
    morpho_HDSR_MetaData morphoMeta[inputCount];
    vector<float> iso = getIso(in, inputCount);
    vector<int> exp_time = getExposureTime(in, inputCount, int);

    for (int i = 0; i < inputCount; i++) {
        morphoMeta[i].iso = (int)iso[i];
        morphoMeta[i].exposure_time = exp_time[i];
        MLOGI(Mia2LogGroupPlugin, "iso[%d]:%d, exp[%d]:%d, anchor%d", i, morphoMeta[i].iso, i,
              morphoMeta[i].exposure_time, anchor);
        if (i == anchor)
            morphoMeta[i].is_base_image = 1;
        else
            morphoMeta[i].is_base_image = 0;
    }

    dump(in, inputCount, me->getName(), [=](unsigned int index) mutable -> string {
        stringstream filename;
        filename << cameraRoleName << "_crop_" << pRect->x << '_' << pRect->y << '_' << pRect->width
                 << '_' << pRect->height << "_iso_" << static_cast<int>(iso[index]) << "_exp_time_"
                 << exp_time[index] << "_anchor_" << anchor << "_input";
        return filename.str();
    });

    int res = mMor->process(inputFrame, &outputFrame, inputCount, init_params, faceRect, faceNum,
                            morphoMeta, anchor);
    if (0 != res) {
        MLOGE(Mia2LogGroupPlugin, "Sr process returned error! Going memcopy.");
        PluginUtils::miCopyBuffer(&outputFrame, &inputFrame[0]);
    }

    dump(out, 1, me->getName(), [=](unsigned int index) mutable -> string {
        stringstream filename;
        index = ((anchor >= 0 && anchor < inputCount) ? anchor : 0);
        filename << cameraRoleName << "_crop_" << pRect->x << '_' << pRect->y << '_' << pRect->width
                 << '_' << pRect->height << "_iso_" << static_cast<int>(iso[index]) << "_exp_time_"
                 << exp_time[index] << "_anchor_" << anchor << "_output";
        return filename.str();
    });

    sessionInfo->setResultMetadata(
        inputCount, request->getTimeStamp(),
        (stringstream() << "sr:{algo:" << me->getName() << " inNum:" << inputCount << ' '
                        << "info:" << ((cameraRole == RoleIdRearTele4x) ? "tele" : "wide") << ' '
                        << vector2exifstr<int32_t>("af", af).c_str() << ' '
                        << vector2exifstr<float>("iso", iso).c_str() << ' '
                        << vector2exifstr<int>("exp", exp_time).c_str() << ' '
                        << "ISZ remosaic:" << (int)((2 == insensorzoom) ? 1 : 0) << "}")
            .str());

    delete mMor;
}

static SingletonAlgorithmAdapter adapter = SingletonAlgorithmAdapter("morpho", process);
extern Entrance morpho = &adapter;
