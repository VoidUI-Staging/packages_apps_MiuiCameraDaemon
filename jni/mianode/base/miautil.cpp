#include "miautil.h"

#undef  LOG_TAG
#define LOG_TAG "MIA_NODE_UTIL"

namespace mialgo {

static void MiDumpPlane(FILE *fp, unsigned char *plane, int width, int height, int pitch)
{
    for (int i = 0; i < height; i++) {
        fwrite(plane, width, 1, fp);
        plane += pitch;
    }
}

void MiaUtil::MiDumpBuffer(MiImageList_p buffer, const char *file)
{
    FILE *p = fopen(file, "wb+");
    if (p != NULL) {
        int width, height;
        if (buffer->format.format == CAM_FORMAT_YUV_420_NV21 || buffer->format.format == CAM_FORMAT_YUV_420_NV12) {
            width = buffer->format.width;
            height = buffer->format.height;
            MiDumpPlane(p, buffer->pImageList[0].pAddr[0], width, height, buffer->planeStride);
            MiDumpPlane(p, buffer->pImageList[0].pAddr[1], width, height / 2, buffer->planeStride);
            ALOGD("MiDump %s width:height = %d * %d\n", file, width, height);
        }
        fclose(p);
    }
}

void MiaUtil::MiDumpBuffer(struct MiImageBuffer *buffer, const char *file)
{
    FILE *p = fopen(file, "wb+");
    if (p != NULL) {
        int width, height;
        if (buffer->PixelArrayFormat == CAM_FORMAT_YUV_420_NV21 || buffer->PixelArrayFormat == CAM_FORMAT_YUV_420_NV12) {
            width = buffer->Width;
            height = buffer->Height;
            MiDumpPlane(p, buffer->Plane[0], width, height, buffer->Pitch[0]);
            MiDumpPlane(p, buffer->Plane[1], width, height / 2, buffer->Pitch[1]);
            ALOGD("MiDump %s width:height = %d * %d\n", file, width, height);
        }
        fclose(p);
    }
}


bool MiaUtil::DumpToFile(struct MiImageBuffer *buffer, const char*  file_name)
{
    if (buffer == NULL)
        return MIAS_INVALID_PARAM;
    char buf[128];
    char timeBuf[128];
    time_t current_time;
    struct tm * timeinfo;
    static String8 lastFileName("");
    static int k = 0;

    time(&current_time);
    timeinfo = localtime(&current_time);
    memset(buf, 0, sizeof(buf));
    strftime(timeBuf, sizeof(timeBuf), "IMG_%Y%m%d%H%M%S", timeinfo);
    snprintf(buf, sizeof(buf), "%s%s_%s", DUMP_FILE_PATH, timeBuf, file_name);
    String8 filePath(buf);
    const char* suffix = "yuv";
    int written_len = 0;
    if (!filePath.compare(lastFileName)) {
        snprintf(buf, sizeof(buf), "_%d.%s", k++, suffix);
        ALOGD("%s%s", filePath.string(), buf);
    } else {
        ALOGD("diff %s %s", filePath.string(), lastFileName.string());
        k = 0;
        lastFileName = filePath;
        snprintf(buf, sizeof(buf), ".%s", suffix);
    }

    filePath.append(buf);
	FILE *p = fopen(filePath.string(), "wb+");
    if (p != NULL) {
        int width, height;
        if (buffer->PixelArrayFormat == CAM_FORMAT_YUV_420_NV21 || buffer->PixelArrayFormat == CAM_FORMAT_YUV_420_NV12) {
            width = buffer->Width;
            height = buffer->Height;
            MiDumpPlane(p, buffer->Plane[0], width, height, buffer->Pitch[0]);
            MiDumpPlane(p, buffer->Plane[1], width, height / 2, buffer->Pitch[1]);
            ALOGD("DumpToFile %s width:height = %d * %d\n", file_name, width, height);
        }
        fclose(p);
    } else {
        ALOGE("Fail t open file for image dumping %s: %s\n", filePath.string(), strerror(errno));
    }
    return MIAS_OK;
}

} //end namespace mialgo
