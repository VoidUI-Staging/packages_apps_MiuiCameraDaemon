#include "morpho.h"

#include <cutils/properties.h>
#include <stdio.h>
#include <utils/Log.h>

#undef LOG_TAG
#define LOG_TAG "MIA_N_MPSR"

#define DUMP_FILE_PATH "/data/vendor/camera/"

static inline uint32_t AlignGeneric32(uint32_t operand, uint32_t alignment)
{
    uint32_t remainder = (operand % alignment);

    return (0 == remainder) ? operand : operand - remainder + alignment;
}

using namespace std;

namespace mialgo {

const char *const Morpho::name = "morpho";

Morpho::Morpho()
{
    MLOGI(Mia2LogGroupPlugin, "Morpho() construction.");
}

Morpho::~Morpho() {}

const char *Morpho::getName()
{
    return name;
}

/*
struct MiImageBuffer
{
    uint32_t format;
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t scanline;
    uint32_t numberOfPlanes;
    uint8_t *plane[MaxPlanes];
    int fd[4];
    int planeSize[4];
};

*/
int Morpho::process(mialgo2::MiImageBuffer *in, mialgo2::MiImageBuffer *out, uint32_t input_num,
                    morpho_HDSR_InitParams init_params, morpho_RectInt *face_rect, uint32_t faceNum,
                    morpho_HDSR_MetaData morphometa[], int anchor)
{
    uint32_t i = 0;
    uint32_t ret = 0;
    morpho_HDSR engine;
    morpho_HDSR_MetaData meta_data;
    morpho_ImageDataEx input[input_num];
    morpho_ImageDataEx output = {0};
    const char *pVersion = NULL;

    memset(&engine, 0, sizeof(morpho_HDSR));
    memset(&meta_data, 0, sizeof(morpho_HDSR_MetaData));

    // 1 init
    ret = MorphoHDSR_init(&engine, &init_params);
    if (ret != MORPHO_OK) {
        MLOGE(Mia2LogGroupPlugin, "MorphoSR_initialize fails %x", ret);
        goto L_finalize;
    }
    MLOGI(Mia2LogGroupPlugin, "MorphoSR_initialize success %x", ret);
    // get version
    pVersion = MorphoHDSR_getVersion();
    if (pVersion)
        MLOGI(Mia2LogGroupPlugin, "MorphoSR_getVersion: %s", pVersion);

    // in moon mode: anchor is -1 and pass the first frame's iso
    meta_data.iso = morphometa[anchor >= 0 && anchor < input_num ? anchor : 0].iso;
    // 2 set param
    ret = MorphoHDSR_setParamsByCfgFile(&engine, &meta_data);
    if (ret != MORPHO_OK) {
        MLOGE(Mia2LogGroupPlugin, "MorphoSR_setParamsByCfgFile fails %x", ret);
        goto L_finalize;
    }
    MLOGI(Mia2LogGroupPlugin, "MorphoSR_setParamsByCfgFile success. anchor:%d, iso: %d", anchor,
          meta_data.iso);

    // 3 add face
    for (int i = 0; i < faceNum; i++) {
        MLOGI(Mia2LogGroupPlugin, "MorphoSR face_rect:[%d,%d,%d,%d]", (face_rect + i)->sx,
              (face_rect + i)->sy, (face_rect + i)->ex, (face_rect + i)->ey);
        ret = MorphoHDSR_addFaceRect(&engine, face_rect + i);
        if (ret != MORPHO_OK) {
            MLOGE(Mia2LogGroupPlugin, "MorphoSR_addFaceRect fails %x", ret);
        } else
            MLOGI(Mia2LogGroupPlugin, "MorphoSR_addFaceRect success %d", i);
    }

    // in -> input
    for (int i = 0; i < input_num; i++) {
        input[i].width = in[i].width;
        input[i].height = in[i].height;
        input[i].dat.semi_planar.y = in[i].plane[0];
        input[i].dat.semi_planar.uv = in[i].plane[1];
        input[i].pitch.semi_planar.y = in[i].stride;
        input[i].pitch.semi_planar.uv = in[i].stride;
        meta_data.iso = morphometa[i].iso;
        meta_data.exposure_time = morphometa[i].exposure_time;
        meta_data.is_base_image = morphometa[i].is_base_image;
        // 4 add image
        ret = MorphoHDSR_addImage(&engine, &input[i], &meta_data);
        MLOGI(Mia2LogGroupPlugin, "MorphoSR_addImage: iso = %d, exp = %d, is base image = %d",
              meta_data.iso, meta_data.exposure_time, meta_data.is_base_image);
        if (ret != MORPHO_OK) {
            MLOGE(Mia2LogGroupPlugin, "MorphoSR_addImage fails %x", ret);
            goto L_finalize;
        }
        MLOGI(Mia2LogGroupPlugin, "MorphoSR %d image is added.", i);
    }

    output.width = out->width;
    output.height = out->height;
    output.dat.semi_planar.y = out->plane[0];
    output.dat.semi_planar.uv = out->plane[1];
    output.pitch.semi_planar.y = out->stride;
    output.pitch.semi_planar.uv = out->stride;
    // 5 process
    ret = MorphoHDSR_process(&engine, &output, NULL);
    if (ret != MORPHO_OK) {
        MLOGE(Mia2LogGroupPlugin, "MorphoSR_process fails %x", ret);
        goto L_finalize;
    }
    MLOGI(Mia2LogGroupPlugin, "MorphoSR_process success %x", ret);

L_finalize:
    // 6 unint
    uint32_t ret_uninit = MorphoHDSR_uninit(&engine);
    if (ret_uninit != MORPHO_OK) {
        MLOGE(Mia2LogGroupPlugin, "MorphoSR_uninit fails %x", ret_uninit);
        return ret_uninit;
    }
    MLOGI(Mia2LogGroupPlugin, "MorphoSR_uninit success %x", ret_uninit);
    return ret;
}

} // namespace mialgo
