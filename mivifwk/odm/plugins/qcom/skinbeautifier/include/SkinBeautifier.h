#ifndef SKINBEAUTIFIER_H_
#define SKINBEAUTIFIER_H_

#include <MiaPluginWraper.h>
#include <stdint.h>

#include "BeautyShot_Image_Algorithm.h"
#include "BeautyShot_Video_Algorithm.h"
#include "SkinBeautifierDef.h"
#include "abstypes.h"
#include "merror.h"

class SkinBeautifier
{
public:
    SkinBeautifier();
    virtual ~SkinBeautifier();
    void SetFeatureParams(BeautyFeatureParams *beautyFeatureParams);
    int Process(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, int faceAngle, int region, int mode,
                unsigned int faceNum);
    int Process(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, int faceAngle, int region, int mode,
                unsigned int faceNum, unsigned int m_appModule);
    void IsFullFeature(bool flag) { this->mBeautyFullFeatureFlag = flag; };
    bool IsFrontCamera(uint32_t frameworkCameraId);
    void ResetPreviewOutput();

private:
    int ProcessCapture(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, int faceAngle, int region);
    int ProcessCapture(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, int faceAngle, int region,
                       unsigned int m_appModule);
    int ProcessPreview(ASVLOFFSCREEN *input, ASVLOFFSCREEN *output, int faceAngle,
                       unsigned int faceNum);
    double GetCurrentSecondValue();
    BeautyStyleFileData *GetBeautyStyleData(int beautyStyleType);

    int mWidth;
    int mHeight;
    int mInitReady;
    bool mBeautyFullFeatureFlag;
    bool mIsFrontCamera;
    BeautyShot_Video_Algorithm *mBeautyShotVideoAlgorithm;

    BeautyFeatureInfo *mFeatureParams;
    BeautyFeatureInfo *mFeatureBeautyMode;
    // int mLastBeautyMode;
    // bool mModeChange;
    int mfeatureCounts;

    BeautyFeatureParams *mbeautyFeatureParams;
    BeautyStyleInfo *mbeautyStyleInfo;
    BeautyStyleFileData mBeautyStyleFileData[Beauty_Style_max];
    int BeautyLastType;
    bool SkinGlossRatioIsLoad;
    bool mBeautyStyleEnable;
};

#endif /* SKINBEAUTIFIER_H_ */
