#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "almalenceAlogAdapter.h"
#undef LOG_TAG
#define LOG_TAG   "MIASR"
#define VERSION_M "tomato"
#define VERSION_S "2020-2-2_182325"

using namespace std;

namespace mialgo {

static const Updater *updater[] = {
    intMetaUpdater,     multiCameraIdsMetaUpdater, int64_tMetaUpdater,
    int32_tMetaUpdater, floatMetaUpdater,          miCropInfoUpdater,
    intMetaUpdater + 1, cropRegionUpdater,         fusionUpdater,
    qcfaUpdater,        combinationModeUpdater,    algoRectUpdater,
};

static const char *updateTuningParam()
{
    static string result;
    static unsigned int lastConfigIndex = -1u;
    static const char charset[] = {'\r', '\n'};
    unsigned int i = 0;
    int pos = -1;

    unsigned int configIndex = getAlgoConfigIndex();
    if (configIndex != lastConfigIndex) {
        result = getAlgoConfig(configIndex);
        for (i = 0; i < sizeof(charset) / sizeof(charset[0]); i++)
            result.erase(remove(result.begin(), result.end(), charset[i]), result.end());
        lastConfigIndex = configIndex;
    }
    return result.c_str();
}

static AlmalenceRectAdapter rectAdapter;

AlmalenceAdapter::AlmalenceAdapter() : AlgorithmAdapter("Almalence", &rectAdapter)
{
    initFilter("/vendor/etc/almalence.config");
    MLOGI("init AlmalenceAdapter");
}

const char *AlmalenceAdapter::getName()
{
    return name;
}

const int AlmalenceAdapter::getDumpMsg(string &dumpMSG)
{
    stringstream dmstream;
    dmstream << getName() << "_role_" << multiCameraIds.currentCameraRole << "_" << in->Pitch[0]
             << "x" << in->Height << "_crop_" << rect.left() << "-" << rect.top() << "-"
             << rect.width() << "-" << rect.height() << "_iso_" << iso << "_exp_" << exp_time;
    dumpMsg = sdmstream.str();
    return 0;
}

const Updater **AlmalenceAdapter::getUpdater()
{
    return updater;
}

unsigned int AlmalenceAdapter::getUpdaterCount()
{
    return sizeof(updater) / sizeof(updater[0]);
}

// This method supposed to be called after all input images (8 frames) collected.
// Full processing is done on current thread. It make take up to 500ms (should be tested on target
// platform).
int AlmalenceAdapter::process(const struct MiImageBuffer *in, struct MiImageBuffer *out,
                              uint32_t input_num)
{
    alma_config_t configProcess[] = {{"profileStr", .p = updateTuningParam()},
                                     {nullptr, .p = nullptr}};

    MLOGI("sr config version:%s", getConfigVersion());
    MLOGI("sr comment :%s", getConfigComment());

    YuvImage<Yuv888> input[input_num];
    for (uint32_t i = 0; i < input_num; i++) {
        input[i] = YuvImage<Yuv888>(
            Yuv888(Plane<std::uint8_t>(in[i].Plane[0], 1, in->Pitch[0]),
                   Plane<std::uint8_t>(in[i].Plane[0] + in->Pitch[0] * in->Scanline[0], 2,
                                       in->Pitch[0]),
                   Plane<std::uint8_t>(in[i].Plane[0] + in->Pitch[0] * in->Scanline[0] + 1, 2,
                                       in->Pitch[0])),
            Exposure(iso, expTime));
    }

    SuperSensorProcessor *stillProcessor = new SuperSensorProcessor(
        pConfigInfo->getCameraId(), Size(in->Width, in->Height), Size(out->Width, out->Height),
        out->Pitch[0], out->Scanline[0], input_num,
        configProcess + (sizeof(configProcess) / sizeof(configProcess[0])) - 1);

    MLOGI("finish new SuperSensorProcessor: %p", stillProcessor);
    if (is10Bit) {
        stillProcessor->process8to10((uint16_t *)out->Plane[0], input, input_num,
                                     *(rectAdapter.getRect()), configProcess);
    } else {
        stillProcessor->process(out->Plane[0], input, input_num, *(rectAdapter.getRect()),
                                configProcess);
    }
    delete stillProcessor;
}

} // namespace mialgo
