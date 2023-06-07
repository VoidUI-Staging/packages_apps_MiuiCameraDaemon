#include <MiaPluginWraper.h>

#include "chioemvendortag.h"
#include "dualcam_mdbokeh_capture_proc.h"

class MiMDBokeh
{
public:
    void MDdestroy();
    int MDinit();
    ~MiMDBokeh();
    S32 InitMDBokeh(DEV_IMAGE_BUF *inputMain, camera_metadata_t *pMeta, S32 m_MDMode,
                    S32 m_LiteMode);
    S32 ProcessMDBokeh(DEV_IMAGE_BUF *outputMain, DEV_IMAGE_BUF *inputMain,
                       DEV_IMAGE_BUF *inputMain_extraEV, DEV_IMAGE_BUF *inputDepth,
                       camera_metadata_t *pMeta, camera_metadata_t *phyMeta);
    S32 m_MDBlurLevel;

private:
    S32 PrepareToMiImage(DEV_IMAGE_BUF *image, AlgoMultiCams::MiImage *stImage);
    S32 GetImageScale(U32 width, U32 height, AlgoMultiCams::ALGO_CAPTURE_FRAME &Scale);
    S64 GetPackData(char *fileName, void **ppData0, U64 *pSize0);
    S64 FreePackData(void **ppData0);
    void setFlushStatus(bool value) { m_flustStatus = value; }
    NodeMetaData *m_pNodeMetaData;
    void *m_hCaptureBokeh;
    S32 m_flustStatus;
    S32 m_LiteMode;
    MdbokehLaunchParams m_launchParams;
    S64 CaptureInitEachPic;
    void *mCaliMdBWT;
    void *mCaliMdNGT;
    void *mCaliMdSFT;
    void *mCaliMdPRT;
    U64 m_CaliMdBWTLength;
    U64 m_CaliMdNGTLength;
    U64 m_CaliMdSFTLength;
    U64 m_CaliMdPRTLength;
};
