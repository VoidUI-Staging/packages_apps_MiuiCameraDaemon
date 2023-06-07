#ifndef SKINBEAUTIFIER_H_
#define SKINBEAUTIFIER_H_

#include <stdint.h>

#include "../../common/skinbeautifier/inc/BeautyShot_Image_Algorithm.h"
#include "../../common/skinbeautifier/inc/BeautyShot_Video_Algorithm.h"
#include "../../common/skinbeautifier/inc/skinbeautifierdef.h"
#include "merror.h"
#define BEAUTY_STYLE_FILE "/vendor/etc/camera/F1_pink.cng"
#include <MiaMetadataUtils.h>
#include <MiaPluginUtils.h>
#include <MiaPluginWraper.h>

#include <fstream>

class SkinBeautifier
{
public:
    SkinBeautifier();
    virtual ~SkinBeautifier();
    void SetFeatureParams(BeautyFeatureParams *beautyFeatureParams);
    int Process(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, int faceAngle, int region, int mode,
                unsigned int faceNum);
    void IsFullFeature(bool flag) { this->mBeautyFullFeatureFlag = flag; };
    void IsFrontCamera(uint32_t frameworkCameraId);
    void ResetPreviewOutput();
    void isBokehMode(int isBokeh) { this->mIsBokeh = isBokeh; };

private:
    int ProcessCapture(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, int faceAngle, int region,
                       unsigned int faceNum);
    int ProcessPreview(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, int faceAngle,
                       unsigned int faceNum);
    double GetCurrentSecondValue();
    int allocBufAndReadFromFile(const char *name, void **ppBuf, int *pSize);

    int InitCapture(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, int faceAngle, int region);
    int InitPreview(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, int faceAngle,
                    unsigned int faceNum);
    int InitLib(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, int faceAngle, int region, int mode,
                unsigned int faceNum);

    BeautyShot_Image_Algorithm *pBeautyShotImageAlgorithm = NULL;

    int mWidth;
    int mHeight;
    int mInitReady = false;
    bool mBeautyFullFeatureFlag;
    bool mIsFrontCamera;
    BeautyShot_Video_Algorithm *mBeautyShotVideoAlgorithm;
    BeautyFeatureInfo *mFeatureParams;
    int mfeatureCounts;
    bool mIsBokeh;
    BeautyFeatureParams *mbeautyFeatureParams;
    int BeautyLastType;
};

#endif /* SKINBEAUTIFIER_H_ */
