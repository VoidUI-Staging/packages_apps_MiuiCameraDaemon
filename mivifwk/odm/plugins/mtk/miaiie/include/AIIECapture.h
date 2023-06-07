/*
 * AIIECapture.h
 *
 *  Created on: 2019-11-20
 *      Author: wangchenan
 */

#ifndef AIIECAPTURE_H_
#define AIIECAPTURE_H_

#include <MiaMetadataUtils.h>
#include <MiaPluginUtils.h>

#include "algo_image_enhancement_capture.h"

#define MI_AIIEALGO_PARAMS_PATH_1   "/data/data/com.android.camera/files/"
#define MI_AIIEALGO_PARAMS_PATH_2   "/sdcard/DCIM/Camera/.aiie"
#define MI_AIIEALGO_PARAMS_PATH_3   "/sdcard/DCIM/Camera/"
#define MI_AIIEALGO_PARAMS_DAL_PATH "vendor/etc/camera/ai_scene.bin"
//#define MI_AIIEALGO_PARAMS_PATH "/data/vendor/camera/"

using namespace mialgo2;

class AIIECapture
{
public:
    AIIECapture();
    virtual ~AIIECapture();

    const char *getVersion();

    int init(struct MiImageBuffer *inputBuffer);

    int process(struct MiImageBuffer *inputBuffer, struct MiImageBuffer *outputBuffer);

private:
    void destory();
    int CheckFiles(std::string &files);
    void *pHandle;
};

#endif /* AIIECapture_H_ */
