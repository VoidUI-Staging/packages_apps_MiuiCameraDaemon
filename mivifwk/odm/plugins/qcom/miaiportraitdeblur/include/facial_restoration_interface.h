//
// Created by wangbencun on 20-11-6.
//

#ifndef AIFACE_FACIAL_RESTORATION_INTERFACE_H
#define AIFACE_FACIAL_RESTORATION_INTERFACE_H
#include <vector>
namespace aiface {

struct Exif_FaceInfo
{
    int id_;
    int stat_;
    int x_;
    int y_;
};
struct MIAIParams
{
    struct _config
    {
        int iso;
        int orientation;
        int camera_id;
        int process_count;
        int big_modle_count;
        int small_model_count;
    } config;
    struct _exif_info
    {
        int FaceSRTag;
        int FaceSRCount;
        std::vector<Exif_FaceInfo> FaceSRResults;
    } exif_info;
};

class FacialRestorationInterface
{
public:
    void init();

    void process(void *input, void *output, MIAIParams *params);

    void destroy();
};

} // namespace aiface
#endif // AIFACE_FACIAL_RESTORATION_INTERFACE_H
