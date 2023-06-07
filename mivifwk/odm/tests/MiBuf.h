#ifndef __MI_BUF_H__
#define __MI_BUF_H__

#include <MiaPluginUtils.h>
#include <MiaPluginWraper.h>
#include <MiaPostProcType.h>
#include <pluma.h>
#include <stdio.h>
#include <utils/Log.h>

#include <vector>

#include "MiDebugUtils.h"

using namespace mialgo2;
using namespace midebug;

const int gTestWidth = 6016;
const int gTestHeight = 4512;
const int gTestStride = 6016;
const int gTestSlice = 4544;
const int gTestFormat = CAM_FORMAT_YUV_420_NV12;

template <typename T>
int miAllocBuffer(T buffer, int stride, int slice, int format);
bool miReadWritePlane(FILE *fp, unsigned char *plane, int width, int height, int pitch, bool read);
ImageParams *initImageParams(const char *file, bool read);
MiaBatchedImage *initMiaBatchedImage(const char *file, bool read);
void releaseImageParams(ImageParams *ret);
void releaseMiaBatchedImage(MiaBatchedImage *image);

int writeNV12(ImageParams buf, const char *file);

#endif /*__MI_BUF_H__*/
