#include "miBokehUp.h"

#include "MiaPluginUtils.h"
#include "device/device.h"

namespace mialgo2 {

#undef LOG_TAG
#define LOG_TAG "LDC-PLUGIN"

#pragma pack(1)
typedef struct __DispMap
{
    S32 form;
    S32 width;
    S32 height;
    S32 pitch;
    S32 reserve;
} DispMap;
#pragma pack(0)

MiBokehLdc::MiBokehLdc()
{
    m_postMiBokehInitThread = NULL;
    m_postMiBokeh = NULL;
    m_debug = 0;
    m_doBokehProcess = FALSE;
}

MiBokehLdc::~MiBokehLdc()
{
    if (m_postMiBokehInitThread && m_postMiBokehInitThread->joinable()) {
        m_postMiBokehInitThread->join();
    }
    if (NULL != m_postMiBokehInitThread) {
        delete m_postMiBokehInitThread;
        m_postMiBokehInitThread = NULL;
    }
    if (NULL != m_postMiBokeh) {
        delete m_postMiBokeh;
        m_postMiBokeh = NULL;
    }
}

void MiBokehLdc::Destroy()
{
    std::unique_lock<std::mutex> lock(m_postInitMutex);
    if (m_postMiBokehInitThread && m_postMiBokehInitThread->joinable()) {
        m_postMiBokehInitThread->join();
    }
    if (NULL != m_postMiBokehInitThread) {
        delete m_postMiBokehInitThread;
        m_postMiBokehInitThread = NULL;
    }
    if (NULL != m_postMiBokeh) {
        delete m_postMiBokeh;
        m_postMiBokeh = NULL;
    }
    m_doBokehProcess = FALSE;
}

S32 MiBokehLdc::Init()
{
    std::unique_lock<std::mutex> lock(m_postInitMutex);
    if (m_postMiBokeh) {
        return RET_OK;
    }
    m_postMiBokeh = bokeh::MiBokehFactory::NewBokeh();
    const bokeh::BokehConfLoader *conf_loader =
        bokeh::MiBokehFactory::LoadConf("/system/etc/camera/mibokeh-conf.txt");
    m_postMiBokehInitThread = new std::thread([this, conf_loader] {
        bokeh::BokehConf post_conf = conf_loader->ConfByName(bokeh::BokehConfName::BOKEH_MOVIE);
#ifdef FLAGS_MIBOKEH_855
        m_postMiBokeh->ChangeModel(bokeh::BokehModelShape::MODEL_512_512_BACK_DXO, post_conf);
#else
        m_postMiBokeh->ChangeModel(bokeh::BokehModelShape::MODEL_512_512_MN_V2, post_conf);
#endif
        if (0 == m_postMiBokeh->Init(post_conf, NULL)) {
            m_doBokehProcess = TRUE;
        } else {
            m_doBokehProcess = FALSE;
            DEV_LOGE("MIBOKEH Init fail!!");
        }
    });
    return RET_OK;
}

S32 MiBokehLdc::GetDepthMask(DEV_IMAGE_BUF *input, DEV_IMAGE_BUF *outputDepth, S32 rotate, S32 flip)
{
    std::unique_lock<std::mutex> lock(m_postInitMutex);
    DEV_IF_LOGE_RETURN_RET((input == NULL || outputDepth == NULL), RET_ERR_ARG, "input err");
    DEV_IF_LOGE_RETURN_RET((input->width < 100 || input->height < 100), RET_ERR_ARG,
                           "input size err");
    S32 ret = RET_ERR;
    // Wait mibokeh init thread,then check return value for determine whether bypass bokeh algo.
    if (m_postMiBokehInitThread && m_postMiBokehInitThread->joinable()) {
        m_postMiBokehInitThread->join();
    }
    DEV_IF_LOGE_RETURN_RET((!m_doBokehProcess), RET_ERR, "*ERROR* m_doBokehProcess FALSE.");
    DEV_IF_LOGE_RETURN_RET((!m_postMiBokeh), RET_ERR, "*ERROR* m_postMiBokeh is NULL.");

    bokeh::MIBOKEH_DEPTHINFO *miBokeh_img = new bokeh::MIBOKEH_DEPTHINFO;
    DEV_IF_LOGE_RETURN_RET((miBokeh_img == NULL), RET_ERR, "*ERROR* miBokeh_img is NULL.");
    m_postMiBokeh->UpdateSizeIn(input->width, input->height, rotate,
                                input->plane[1] - input->plane[0], input->stride[0], false);
    m_postMiBokeh->Prepare((U8 *)input->plane[0]);
    m_postMiBokeh->Segment();
    m_postMiBokeh->GetDepthImage(miBokeh_img, flip);

    if (miBokeh_img->pDepthData != NULL) {
        DispMap *pDispMap = (DispMap *)miBokeh_img->pDepthData;
        outputDepth->width = pDispMap->width;
        outputDepth->height = pDispMap->height;
        outputDepth->stride[0] = pDispMap->pitch;
        DEV_IMAGE_BUF inputMask;
        inputMask.width = pDispMap->width;
        inputMask.height = pDispMap->height;
        inputMask.stride[0] = pDispMap->pitch;
        inputMask.format = DEV_IMAGE_FORMAT_Y8;
        inputMask.plane[0] = (char *)((char *)miBokeh_img->pDepthData + sizeof(DispMap));
        ret = Device->image->ReSize(outputDepth, &inputMask);
        //        DEV_LOGE("pDispMap->form=%x    pDispMap->width=%d  ,pDispMap->height=%d
        //        ,pDispMap->pitch=%d ", pDispMap->form, pDispMap->width,
        //                pDispMap->height, pDispMap->pitch);
        if (miBokeh_img->pDepthData != NULL) {
            free(miBokeh_img->pDepthData); // Internal malloc
        }
    }
    if (miBokeh_img != NULL) {
        delete miBokeh_img;
    }
    return ret;
}

void MiBokehLdc::SetDebugLevel(U32 debug)
{
    m_debug = debug;
    return;
}

void MiBokehLdc::AddRef()
{
    m_refCountMutex.lock();
    m_bokehInitCount++;
    DEV_LOGI("MiBokehLdc: m_bokehInitCount = %d", m_bokehInitCount);
    if (m_bokehInitCount == 1) {
        MiBokehLdc::GetInstance()->Init();
        DEV_LOGI("MiBokehLdc: ***Init*** Done.");
    }
    m_refCountMutex.unlock();
}

void MiBokehLdc::AddRefIfZero()
{
    m_refCountMutex.lock();
    bool needInitBokeh = FALSE;
    if (m_bokehInitCount == 0) {
        m_bokehInitCount++;
        DEV_LOGI("MiBokehLdc: m_bokehInitCount = %d", m_bokehInitCount);
        needInitBokeh = m_bokehInitCount == 1 ? TRUE : FALSE;
    }
    if (needInitBokeh) {
        MiBokehLdc::GetInstance()->Init();
        DEV_LOGI("MiBokehLdc: ***Init*** Done.");
    }
    m_refCountMutex.unlock();
}

void MiBokehLdc::ReleaseRef()
{
    m_refCountMutex.lock();
    bool needDeinitBokeh = FALSE;
    if (m_bokehInitCount > 0) {
        m_bokehInitCount--;
        needDeinitBokeh = m_bokehInitCount == 0 ? TRUE : FALSE;
        DEV_LOGI("MiBokehLdc: needDeinit:%d m_bokehInitCount = %d", needDeinitBokeh,
                 m_bokehInitCount);
    }
    if (needDeinitBokeh) {
        MiBokehLdc::GetInstance()->Destroy();
        DEV_LOGI("MiBokehLdc: ***Destroy*** Done.");
    }
    m_refCountMutex.unlock();
}

} // namespace mialgo2
