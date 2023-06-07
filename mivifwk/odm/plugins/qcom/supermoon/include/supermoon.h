#ifndef __SUPER_MOON__
#define __SUPER_MOON__

#include <stdlib.h>

#define MOON_SR_BUILD_TIMESTAMP "2021_0712_1448"

namespace mialgo {

#define MiFormatsMaxPlanes 3
struct MiImageBuffer
{
    int PixelArrayFormat;
    int Width;
    int Height;
    int Pitch[MiFormatsMaxPlanes];
    int Scanline[MiFormatsMaxPlanes];
    unsigned char *Plane[MiFormatsMaxPlanes];
    int fd[MiFormatsMaxPlanes];
};

/** @brief create resource before run sr.
@param storage_path A directory to save "mace_cl_compiled_program.bin" which makes MACE initialize
more fast. (We need to have read and write permissions for the directory)
@param moon_detection_model_path A file path of moon detection model.
*/
int moon_init(const char *storage_path, const char *moon_detection_model_path);

/** @brief release resource.
 */
void moon_release();

/** @brief give a moon image and return a supermoon image.

@param input NV21 input image buffer.
@param output NV21 output image buffer.
@param zoom_ratio Camera zoom ratio (at least 5x zoom).
@return  0 means succeed in processing a supermoon,
        -1 means fail to locate a moon,
        -2 means zoom ratio not supportted,
*/
int moon_sr(struct MiImageBuffer *input, struct MiImageBuffer *output, float zoom_ratio);

/** @brief a function to read a image and convert it into a MiImageBuffer for test.
 */

MiImageBuffer *load(const char *img_path, const char *tmp_folder = NULL);
/** @brief a function to save image from MiImageBuffer for test.
 */
void save_mibuffer(MiImageBuffer *output, const char *out_path);
} // namespace mialgo

#endif // END define __SUPER_MOON__
