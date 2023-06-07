#ifndef _ALTEKDEPURPLE_H_
#define _ALTEKDEPURPLE_H_

/*
PROPRIETARY STATEMENT:
THIS FILE (OR DOCUMENT) CONTAINS INFORMATION CONFIDENTIAL AND
PROPRIETARY TO ALTEK CORPORATION AND SHALL NOT BE REPRODUCED
OR TRANSFERRED TO OTHER FORMATS OR DISCLOSED TO OTHERS.
ALTEK DOES NOT EXPRESSLY OR IMPLIEDLY GRANT THE RECEIVER ANY
RIGHT INCLUDING INTELLECTUAL PROPERTY RIGHT TO USE FOR ANY
SPECIFIC PURPOSE, AND THIS FILE (OR DOCUMENT) DOES NOT
CONSTITUTE A FEEDBACK TO ANY KIND.
*/

#include <cutils/log.h>

#include <condition_variable>
#include <mutex>
#include <thread>

#include "MiaPluginUtils.h"
#include "alCFR.h"
namespace mialgo2 {

#define INVALID_SENSORID 0xFF

#define BACK_WIDE               0
#define BACK_TELE               1
#define BACK_UWIDE              3
#define BACK_MACRO              4
#define BACK_TELE4X             5
#define DEPURPLE_ZOOMRATIO_FOUR 4

const char *SensorIdString(int id);

class QAltekDepurple
{
public:
    QAltekDepurple() = delete;
    QAltekDepurple(uint32_t width, uint32_t height);
    virtual ~QAltekDepurple();
    int incInit(CFR_MODEINFO_T modeInfo, bool incIfNoInit = false);
    void decInit();
#if defined(NUWA_CAM) || defined(FUXI_CAM) || defined(SOCRATES_CAM)
    int process(struct MiImageBuffer *input, alCFR_FACE_T faceInfo, uint32_t exposure,
                alCFR_RECT zoomInfo, CFR_MODEINFO_T &modeInfo);
#else
    int process(struct MiImageBuffer *input, alCFR_FACE_T faceInfo, uint32_t exposure,
                alGE_RECT zoomInfo, CFR_MODEINFO_T &modeInfo);
#endif
    int loadResource(CFR_MODEINFO_T &modeInfo);
    int initCFR();
    void deinitCFR();

private:
    std::thread *m_CFRInitThread;
    std::mutex m_initMutex;
    std::condition_variable m_initCondition;
    std::mutex m_refCountMutex;
    int mInitRefCount;
    CFR_MODEINFO_T m_modeInfo;
    DepurpleInitState mInitState;
    unsigned char *mBuffer;
    void *mInOutHandle;
    int mOriWidth; // original image size
    int mOriHeight;
    bool misHighDefinitionSize;
#if defined(NUWA_CAM) || defined(FUXI_CAM) || defined(SOCRATES_CAM)
    unsigned int mInput_tuning_data_size;
#endif
};

} // namespace mialgo2
#endif //__ALCFR_LIBRARY_HEADER_
