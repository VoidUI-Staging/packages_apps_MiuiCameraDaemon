#ifndef _MORPHO_H_
#define _MORPHO_H_

#include "MiaPluginUtils.h"
#include "morpho_api.h"
#include "morpho_error.h"
#include "morpho_hdsr.h"
#include "morpho_hdsr_base.h"
#include "morpho_image_data.h"
#include "morpho_image_data_ex.h"
#include "morpho_image_format.h"
#include "morpho_log_dbg.h"
#include "morpho_primitive.h"
#include "morpho_rect_int.h"

typedef char CHAR;
typedef int INT;
typedef float FLOAT;
// typedef uint8_t  UINT;

#define CDKResultSuccess 0
#define CDKResultEFailed -1

namespace mialgo {

class Morpho
{
public:
    Morpho();
    ~Morpho();
    const char *getName();
    int process(mialgo2::MiImageBuffer *in, mialgo2::MiImageBuffer *out, uint32_t input_num,
                morpho_HDSR_InitParams init_params, morpho_RectInt *face_rect, uint32_t faceNum,
                morpho_HDSR_MetaData morphometa[], int anchor);

private:
    static const char *const name;
};

} // namespace mialgo
#endif
