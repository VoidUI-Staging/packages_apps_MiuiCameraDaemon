#include "MiaPipeline.h"
#ifndef __MIA_PIPELINEPRUNER__
#define __MIA_PIPELINEPRUNER__

namespace mialgo2 {
using namespace midebug;

typedef enum {
    NoneMask = 0x0,
    AllMask = 0x1,
    HdrMask = 0x1 << 1,
    DepurpleMask = 0x1 << 2,
    BeautyMask = 0x1 << 3,
    BodyslimMask = 0x1 << 4,
    SuperNightMask = 0x1 << 5,
    SuperMoonMask = 0x1 << 6,
    MiAiieMask = 0x1 << 7,
    GpuMask = 0x1 << 8,
    DeblurMask = 0x1 << 9,
    SrMask = 0x1 << 10,
    CaptureFusionMask = 0x1 << 11,
    LdcMask = 0x1 << 12,
    DCMask = 0x1 << 13,
    OfflineMask = 0x1 << 14,
    MiFlawMask = 0x1 << 15,
    MemcpyMask = 0x1 << 16,
    AIPortraitDeblur = 0x1 << 17,
    RearSuperNightMask = 0x1 << 18,
    CupYuvMask = 0x1 << 19,
    CupRawMask = 0x1 << 20,
    BokehMask = 0x1 << 21,
    ArcSoftsSLLMask = 0x1 << 22,
    PureViewMask = 0x1 << 23,
    XiaoMiFuisonMask = 0X1 << 24,
    MiaiRawHDR = 0X1 << 25,
    MiAlgoSNSCMask = 0X1 << 26,
    AnchorMask = 0x1 << 27,

    MtkHWProcMask = 0x1 << 30,
    MtkHWJpegMask = 0x1 << 31
} DisableNodeMask;

typedef enum { INPUT, OUTPUT } portType;
typedef struct disablePort
{
    int portId;
    portType type;
    bool operator<(const disablePort &other) const
    {
        // INPUT and OUTPUT shall be supported to exist in one structure
        // The weighted value is 100
        return ((type * 100) + portId) < ((other.type * 100) + other.portId);
    }
    std::string toString() const { return std::to_string(portId) + std::to_string(type); }
} disablePort;

typedef struct
{
    int inputPortId;
    int outputPortId;
    std::string toString() const
    {
        return std::to_string(inputPortId) + std::to_string(outputPortId);
    }
} throughPath;

class MiaCustomization;
class PipelinePruner : public virtual RefBase
{
public:
    PipelinePruner(std::map<std::string, sp<MiaNode>> &nodes, PipelineInfo &rawPipeline);
    bool rebuildLinks(MiaParams &settings, PipelineInfo &pipeline, uint32_t firstFrameInputFormat,
                      std::vector<MiaImageFormat> &sinkBufferFormat);
    PipelineInfo *getRawPipelineInfo() { return &mRawPipeline; }

private:
    bool needDisablePlugin(std::string &pluginName);
    void lookForDisableNodes(MiaParams &settings, std::vector<std::string> &disableNodes);
    void lookForDisableNodesForRawMoveUp(camera_metadata_t *pMeta, NodeItem &nodeItem);
    bool checkNodeEnable(std::string &nodeInstance, MiaParams &settings);
    bool pipelineReuse(std::string &reuseKey, PipelineInfo &result);
    void pipelinePortPrune(std::map<std::string, std::set<disablePort>> &disablePorts,
                           PipelineInfo &pipeline);
    void pipelineNodePrune(std::map<std::string, std::vector<throughPath>> &nodesBypassRule,
                           std::vector<std::string> normalDisableNodes, PipelineInfo &pipeline);
    // Check whether the link is correct and whether the format matches
    bool checkLinkInformation(std::map<std::string, std::vector<int>> &dstInstanceMap,
                              std::string &dstInstance, const std::vector<LinkItem> &pipelineLinks,
                              std::set<std::string> &sourcePortNumer);
    bool checkPipelineLink(PipelineInfo pipeline, uint32_t firstFrameInputFormat,
                           std::vector<MiaImageFormat> &sinkBufferFormat);
    void dumpLinkInformation(const std::string &pruneResult, const PipelineInfo &pipeline,
                             const MiaParams &settings);

private:
    PipelineInfo mRawPipeline;
    uint32_t mDisableNodeMask;
    std::map<std::string, sp<MiaNode>> mNodes;
    std::mutex mLock;
    std::map<std::string, PipelineInfo> mPrunedResult;
};

class MiaCustomization
{
public:
    static void pipelineSelect(const camera_metadata_t *metadata, const PipelineInfo &pipeInfo,
                               std::string &pipelineName, std::vector<LinkItem> &links);
    static void disablePortSelect(const std::string &pipelineName, camera_metadata_t *metadata,
                                  std::map<std::string, std::set<disablePort>> &disablePorts);
    static bool nodeBypassRuleSelect(
        const std::string &pipelineName, camera_metadata_t *metadata,
        const std::string &nodeInstance,
        std::map<std::string, std::vector<throughPath>> &nodesBypassRule,
        std::map<std::string, std::set<disablePort>> &disablePorts);
    static void resetNodeMask(camera_metadata_t *metadata, const std::string &instance,
                              PipelineInfo &pipeline);
    static void reMapSourcePortId(uint32_t &sourcePort, camera_metadata_t *metadata,
                                  camera_metadata_t *phyMetadata, const std::string &pipelineName,
                                  sp<MiaImage> inputImage);
};

class MiaCustomizationMTK
{
public:
    static void pipelineSelect(const camera_metadata_t *metadata, const PipelineInfo &pipeInfo,
                               std::string &pipelineName, std::vector<LinkItem> &links);
    static void disablePortSelect(const std::string &pipelineName, camera_metadata_t *metadata,
                                  std::map<std::string, std::set<disablePort>> &disablePorts);
    static bool nodeBypassRuleSelect(
        const std::string &pipelineName, camera_metadata_t *metadata,
        const std::string &nodeInstance,
        std::map<std::string, std::vector<throughPath>> &nodesBypassRule,
        std::map<std::string, std::set<disablePort>> &disablePorts);
    static void resetNodeMask(camera_metadata_t *metadata, const std::string &instance,
                              PipelineInfo &pipeline);
    static void reMapSourcePortId(uint32_t &sourcePort, camera_metadata_t *metadata,
                                  camera_metadata_t *phyMetadata, const std::string &pipelineName,
                                  sp<MiaImage> inputImage);
};

} // namespace mialgo2

#endif
