#include "MiaPipelinePruner.h"

static const int32_t sDisableMask =
    property_get_int32("persist.vendor.camera.algoengine.disablemask", 0);

namespace mialgo2 {
static std::map<std::string, DisableNodeMask> gPluginNameToNodeMask = {
    {"com.xiaomi.plugin.hdr", HdrMask},
    {"com.xiaomi.plugin.depurple", DepurpleMask},
    {"com.xiaomi.plugin.skinbeautifier", BeautyMask},
    {"com.xiaomi.plugin.bodyslim", BodyslimMask},
    {"com.xiaomi.plugin.sr", SrMask},
    {"com.xiaomi.plugin.miaiportraitsupernight", SuperNightMask},
    {"com.xiaomi.plugin.supermoon", SuperMoonMask},
    {"com.xiaomi.plugin.mialgoellc", SuperNightMask},
    {"com.xiaomi.plugin.miaiie", MiAiieMask},
    {"com.xiaomi.plugin.gpu", GpuMask},
    {"com.xiaomi.plugin.miaideblur", DeblurMask},
    {"com.xiaomi.plugin.miaiportraitdeblur", AIPortraitDeblur},
    {"com.xiaomi.plugin.mifragment", MiAiieMask},
    {"com.xiaomi.plugin.misegment", MiAiieMask},
    {"com.xiaomi.plugin.fusion", CaptureFusionMask},
    {"com.xiaomi.plugin.mifusion", XiaoMiFuisonMask},
    {"com.xiaomi.plugin.ldc", LdcMask},
    {"com.xiaomi.plugin.dc", DCMask},
    {"com.xiaomi.plugin.offlinepostproc", OfflineMask},
    {"com.xiaomi.plugin.miflaw", MiFlawMask},
    {"com.xiaomi.plugin.memcpy", MemcpyMask},
    {"com.xiaomi.plugin.rearsupernight", RearSuperNightMask},
    {"com.xiaomi.plugin.cupyuv", CupYuvMask},
    {"com.xiaomi.plugin.cupraw", CupRawMask},
    {"com.xiaomi.plugin.capbokeh", BokehMask},
    {"com.xiaomi.plugin.arcsoftsll", ArcSoftsSLLMask},
    {"com.xiaomi.plugin.miairawhdr", MiaiRawHDR},
    {"com.xiaomi.plugin.pureview", PureViewMask},
    {"com.xiaomi.plugin.mialgosnsc", MiAlgoSNSCMask},
    {"com.xiaomi.plugin.anchor", AnchorMask},
    {"com.xiaomi.plugin.hwproc", MtkHWProcMask},
    {"com.xiaomi.plugin.hwjpeg", MtkHWJpegMask}};

PipelinePruner::PipelinePruner(std::map<std::string, sp<MiaNode>> &nodes, PipelineInfo &rawPipeline)
{
    mNodes = nodes;
    mRawPipeline = rawPipeline;
    mDisableNodeMask = sDisableMask;
    MLOGI(Mia2LogGroupCore, "mDisableNodeMask 0x%x", mDisableNodeMask);
}

// if true, the plugin's init and process function not called
// Just for debugging memory usage
bool PipelinePruner::needDisablePlugin(std::string &pluginName)
{
    if (mDisableNodeMask == NoneMask) {
        return false;
    }

    if (mDisableNodeMask == AllMask) {
        return true;
    }

    DisableNodeMask mask = NoneMask;
    if (gPluginNameToNodeMask.find(pluginName) != gPluginNameToNodeMask.end()) {
        mask = gPluginNameToNodeMask[pluginName];
        if (mask & mDisableNodeMask)
            return true;
        else
            return false;
    }

    return false;
}

bool PipelinePruner::checkNodeEnable(std::string &nodeInstance, MiaParams &settings)
{
    bool result = false;
    auto it = mNodes.find(nodeInstance);
    if (it != mNodes.end()) {
        result = it->second->isEnable(settings);
    }

    return result;
}

void PipelinePruner::lookForDisableNodes(MiaParams &settings,
                                         std::vector<std::string> &disableNodes)
{
    if (settings.metadata == nullptr) {
        return;
    }

    for (auto it = mRawPipeline.nodes.begin(); it != mRawPipeline.nodes.end(); it++) {
        // bypass node base on metadata
        it->second.isEnable = checkNodeEnable(it->second.nodeInstance, settings);

        if (!it->second.isEnable) {
            disableNodes.push_back(it->second.nodeInstance);
            continue;
        }

        // bypass node base on "xiaomi.burst.captureHint"
        void *data = NULL;
        int32_t captureHint = 0;
        VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.burst.captureHint", &data);
        if (data != NULL) {
            captureHint = *static_cast<int32_t *>(data);
        }
        if (captureHint && 0 != strcmp(it->second.nodeName.c_str(), "com.xiaomi.plugin.ldc") &&
            0 != strcmp(it->second.nodeName.c_str(), "com.xiaomi.plugin.offlinepostproc") &&
            0 != strcmp(it->second.nodeName.c_str(), "com.xiaomi.plugin.swjpegencode")) {
            it->second.isEnable = false;
        }

        // prune offlinepostproc plugin for mivi raw sr
        if (MiaInternalProp::getInstance()->getInt(Soc) == midebug::SocType::QCOM) {
            lookForDisableNodesForRawMoveUp(settings.metadata, it->second);
        }

        if (!it->second.isEnable) {
            disableNodes.push_back(it->second.nodeInstance);
            continue;
        }

        // bypass node base on Mask
        if (needDisablePlugin(it->second.nodeName)) {
            it->second.isEnable = false;
        }

        if (!it->second.isEnable) {
            disableNodes.push_back(it->second.nodeInstance);
            continue;
        }
    }
}

// Just use for full scene raw move up to mivi
void PipelinePruner::lookForDisableNodesForRawMoveUp(camera_metadata_t *pMeta, NodeItem &nodeItem)
{
    void *data = NULL;
    uint8_t isMiviRawZsl = 0;
    VendorMetadataParser::getVTagValue(pMeta, "com.xiaomi.mivi2.rawzsl.enable", &data);
    if (NULL != data) {
        isMiviRawZsl = *static_cast<uint8_t *>(data);
    }

    data = NULL;
    uint8_t isMiviRawNonZsl = 0;
    VendorMetadataParser::getVTagValue(pMeta, "com.xiaomi.mivi2.rawnonzsl.enable", &data);
    if (NULL != data) {
        isMiviRawNonZsl = *static_cast<uint8_t *>(data);
    }

    bool isMiviFullSceneRawUp = isMiviRawZsl || isMiviRawNonZsl;
    if (isMiviFullSceneRawUp) {
        data = NULL;
        uint8_t isSrEnable = 0;
        VendorMetadataParser::getVTagValue(pMeta, "xiaomi.superResolution.enabled", &data);
        if (data != NULL) {
            isSrEnable = *static_cast<uint8_t *>(data);
        }

        data = NULL;
        uint8_t isHdrSrEnable = 0;
        VendorMetadataParser::getVTagValue(pMeta, "xiaomi.hdr.sr.enabled", &data);
        if (NULL != data) {
            isHdrSrEnable = *static_cast<uint8_t *>(data);
        }

        data = NULL;
        uint8_t superNightMode = 0;
        VendorMetadataParser::getVTagValue(pMeta, "xiaomi.mivi.supernight.mode", &data);
        if (NULL != data) {
            superNightMode = *static_cast<uint8_t *>(data);
        }

        bool isMiviRawSr = isSrEnable && isMiviRawZsl;
        bool isMiviRawHdrSr = isMiviRawSr && isHdrSrEnable;

        // prune i2y/mfnr/stats nodes when mivi raw zsl on
        if (isMiviRawZsl &&
            !strcmp(nodeItem.nodeName.c_str(), "com.xiaomi.plugin.offlinepostproc") &&
            (strcmp(nodeItem.nodeInstance.c_str(), "BayerRaw2YuvInstance2") &&
             strcmp(nodeItem.nodeInstance.c_str(), "BayerRaw2YuvInstance1") &&
             strcmp(nodeItem.nodeInstance.c_str(), "YuvReprocessInstance0") &&
             strcmp(nodeItem.nodeInstance.c_str(), "MfnrInstance0") &&
             strcmp(nodeItem.nodeInstance.c_str(), "BurstJpegInstance0"))) {
            nodeItem.isEnable = false;
        }
        // prune redundant b2y node when mivi raw hdrsr off
        if (!isMiviRawHdrSr &&
            !strcmp(nodeItem.nodeName.c_str(), "com.xiaomi.plugin.offlinepostproc") &&
            !strcmp(nodeItem.nodeInstance.c_str(), "BayerRaw2YuvInstance2")) {
            nodeItem.isEnable = false;
        }
        // prune sigframe b2y node when mivi raw zsl off
        if (!isMiviRawZsl &&
            !strcmp(nodeItem.nodeName.c_str(), "com.xiaomi.plugin.offlinepostproc") &&
            strstr(nodeItem.nodeInstance.c_str(), "BayerRaw2YuvInstance") &&
            (nodeItem.nodeMask & SIGFRAME_MODE)) {
            nodeItem.isEnable = false;
        }
    } else {
        // prune all sigframe chioffline node here when MiviFullSceneRawUp off
        if (!strcmp(nodeItem.nodeName.c_str(), "com.xiaomi.plugin.offlinepostproc") &&
            (nodeItem.nodeMask & SIGFRAME_MODE)) {
            nodeItem.isEnable = false;
        }
    }
}

bool PipelinePruner::pipelineReuse(std::string &reuseKey, PipelineInfo &pipeline)
{
    bool result = false;
    std::lock_guard<std::mutex> locker(mLock);
    auto it = mPrunedResult.find(reuseKey);
    if (it != mPrunedResult.end()) {
        pipeline = it->second;
        result = true;
    }

    return result;
}

bool PipelinePruner::rebuildLinks(MiaParams &settings, PipelineInfo &pipeline,
                                  uint32_t firstFrameInputFormat,
                                  std::vector<MiaImageFormat> &sinkBufferFormat)
{
    std::vector<std::string> disableNodes;
    bool rebuildResult = true;
    double startTime = MiaUtil::nowMSec();

    // The time-consuming part of pipeline prune process
    lookForDisableNodes(settings, disableNodes);

    char processTime[16] = {0};
    snprintf(processTime, sizeof(processTime), "%0.2fms", MiaUtil::nowMSec() - startTime);
    MLOGI(Mia2LogGroupCore, "lookForDisableNodes use %s", processTime);

    std::string pipelineName = mRawPipeline.pipelineName;
    // all json files are not merged into one, so pipelineSelect is not needed for the time being
    /* std::vector<LinkItem> links;
     * if(MiaInternalProp::getInstance()->getInt(Soc) == midebug::SocType::QCOM){
     *     MiaCustomization::pipelineSelect(settings.metadata, mRawPipeline, pipelineName, links);
     * }else{
     *     MiaCustomizationMTK::pipelineSelect(settings.metadata, mRawPipeline, pipelineName,
     * links);
     * }
     */

    std::map<std::string, std::set<disablePort>> disablePorts;
    std::map<std::string, std::vector<throughPath>> throughPaths;
    std::vector<std::string> normalDisableNodes; // nodes are disabled and have no customize info

    if (MiaInternalProp::getInstance()->getInt(Soc) == midebug::SocType::QCOM) {
        MiaCustomization::disablePortSelect(pipelineName, settings.metadata, disablePorts);
    } else {
        MiaCustomizationMTK::disablePortSelect(pipelineName, settings.metadata, disablePorts);
    }

    for (auto &nodeInstance : disableNodes) {
        bool haveMatch;
        if (MiaInternalProp::getInstance()->getInt(Soc) == midebug::SocType::QCOM) {
            haveMatch = MiaCustomization::nodeBypassRuleSelect(
                pipelineName, settings.metadata, nodeInstance, throughPaths, disablePorts);
        } else {
            haveMatch = MiaCustomizationMTK::nodeBypassRuleSelect(
                pipelineName, settings.metadata, nodeInstance, throughPaths, disablePorts);
        }

        if (!haveMatch) {
            normalDisableNodes.push_back(nodeInstance);
        }
    }

    if (MiaInternalProp::getInstance()->getInt(Soc) == midebug::SocType::QCOM) {
        if (pipelineName == "DualBokehSnapshot") { // work around
            void *data = NULL;
            int mdmode = 0;
            VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.bokeh.MDMode", &data);
            if (NULL != data) {
                mdmode = *reinterpret_cast<uint32_t *>(data);
            }
            int superNightEnabled = 0;
            VendorMetadataParser::getVTagValue(settings.metadata, "xiaomi.bokeh.superNightEnabled",
                                               &data);
            if (NULL != data) {
                superNightEnabled = *reinterpret_cast<uint8_t *>(data);
            }
            if (mdmode) {
                throughPaths["virtualSinkbuffer2"].push_back({2, 0});
                if (superNightEnabled) {
                    throughPaths["MemcpyInstance0"].push_back({0, 0});
                } else {
                    throughPaths["BayerRaw2YuvInstance1"].push_back({0, 0});
                }
                disablePorts["CapbokehInstance0"].insert({1, OUTPUT});
            } else {
                disablePorts["SourceBuffer2"].insert({2, OUTPUT});
            }
        }
    }

    // pipeline prune result reuse
    std::string pruneResultReuseKey = pipelineName;
    for (const auto &itMap : disablePorts) {
        pruneResultReuseKey += itMap.first;
        for (const disablePort &info : itMap.second) {
            pruneResultReuseKey += info.toString();
        }
    }
    for (const auto &itMap : throughPaths) {
        pruneResultReuseKey += itMap.first;
        for (const throughPath &info : itMap.second) {
            pruneResultReuseKey += info.toString();
        }
    }
    for (std::string &nodeInstance : normalDisableNodes) {
        pruneResultReuseKey += nodeInstance;
    }

    if (pipelineReuse(pruneResultReuseKey, pipeline)) {
        return true;
    }

    // no reusable results, start prune
    pipeline = mRawPipeline;
    pipelinePortPrune(disablePorts, pipeline);
    pipelineNodePrune(throughPaths, normalDisableNodes, pipeline);

    // set type of links available
    for (int i = 0; i < pipeline.links.size(); i++) {
        LinkItem &link = pipeline.links[i];
        if (link.srcNodeInstance.find("SourceBuffer") != std::string::npos) {
            link.type = SourceLink;
        } else if (link.dstNodeInstance.find("SinkBuffer") != std::string::npos) {
            link.type = SinkLink;
        } else {
            link.type = InternalLink;
        }

        MiaCustomization::resetNodeMask(settings.metadata, link.dstNodeInstance, pipeline);
    }

    // sourceBuffer -> sinkBuffer ===>  sourceBuffer -> memcpy -> sinkBuffer
    // virtual -> sinkBuffer     ===>  virtual -> memcpy -> sinkBuffer
    // virtual -> virtual        ===>  virtual -> memcpy -> virtual
    std::vector<LinkItem> neededBridgeLinks;
    std::vector<MiaPixelFormat> linkFormat = {CAM_FORMAT_UNDEFINED};
    for (int i = 0; i < pipeline.links.size(); i++) {
        LinkItem &link = pipeline.links[i];
        if (link.dstNodeInstance.find("SinkBuffer") != std::string::npos) {
            if ((link.srcNodeInstance.find("SourceBuffer") != std::string::npos) ||
                (pipeline.nodes.at(link.srcNodeInstance).nodeMask & VIRTUAL_MODE)) {
                LinkItem newlink = {0,
                                    "newMemcpyInstance",
                                    CAM_FORMAT_UNDEFINED,
                                    link.dstNodePortId,
                                    link.dstNodeInstance,
                                    linkFormat,
                                    SinkLink};
                neededBridgeLinks.push_back(newlink);

                link.dstNodeInstance = "newMemcpyInstance";
                link.dstNodePortId = 0;
            }
        } else if ((pipeline.nodes.at(link.dstNodeInstance).nodeMask & VIRTUAL_MODE) &&
                   (link.srcNodeInstance.find("SourceBuffer") == std::string::npos) &&
                   (pipeline.nodes.at(link.srcNodeInstance).nodeMask & VIRTUAL_MODE)) {
            LinkItem newlink = {0,
                                "newMemcpyInstance",
                                CAM_FORMAT_UNDEFINED,
                                link.dstNodePortId,
                                link.dstNodeInstance,
                                linkFormat,
                                link.type};
            neededBridgeLinks.push_back(newlink);

            link.dstNodeInstance = "newMemcpyInstance";
            link.dstNodePortId = 0;
        }
    }
    while (neededBridgeLinks.size() > 0) {
        pipeline.links.push_back(neededBridgeLinks.back());
        neededBridgeLinks.pop_back();
    }

    if (MiaInternalProp::getInstance()->getBool(OpenPrunerAbnormalDump)) {
        rebuildResult = checkPipelineLink(pipeline, firstFrameInputFormat, sinkBufferFormat);
    }

    // save prune result
    std::lock_guard<std::mutex> locker(mLock);
    if (rebuildResult) {
        mPrunedResult[pruneResultReuseKey] = pipeline;
    } else {
        dumpLinkInformation(pruneResultReuseKey, pipeline, settings);
    }

    return rebuildResult;
}

void PipelinePruner::pipelinePortPrune(std::map<std::string, std::set<disablePort>> &disablePorts,
                                       PipelineInfo &pipeline)
{
    if (disablePorts.size() == 0) {
        return;
    }

    std::vector<LinkItem> &links = pipeline.links;
    std::vector<bool> linkFlag(links.size(), true);
    std::map<std::string, std::vector<int>> inputLinkMap;
    std::map<std::string, std::vector<int>> outputLinkMap;
    for (int index = 0; index < links.size(); index++) {
        LinkItem &link = links[index];
        std::string &linkSrcInstance = link.srcNodeInstance;
        std::string &linkDstInstance = link.dstNodeInstance;
        // the current link is the inputLink of currentLink's DstNodeInstance
        inputLinkMap[linkDstInstance].push_back(index);
        // the current link is the outputLink of currentLink's SrcNodeInstance
        outputLinkMap[linkSrcInstance].push_back(index);
    }

    for (auto &itMap : disablePorts) {
        std::string nodeInstance = itMap.first;
        for (auto &itSet : itMap.second) {
            int portId = itSet.portId;
            portType type = itSet.type;
            if (type == INPUT) { // Delete the link related to the input port
                if (inputLinkMap.find(nodeInstance) == inputLinkMap.end()) {
                    MLOGI(Mia2LogGroupCore,
                          "Node %s with src port %d does not exist in %s pipeline",
                          nodeInstance.c_str(), portId, pipeline.pipelineName.c_str());
                    continue;
                }
                std::vector<int> &inputLinks = inputLinkMap[nodeInstance];
                int linkIndex = -1;
                for (int index : inputLinks) {
                    if (links[index].dstNodePortId == portId) {
                        linkIndex = index;
                        break;
                    }
                }
                if (linkIndex == -1) {
                    MLOGE(Mia2LogGroupCore,
                          "Please check whether the configuration information is correct");
                    continue;
                }
                std::queue<int> runQueue;
                runQueue.push(linkIndex);
                // Find the link to be deleted and delete it
                while (!runQueue.empty()) {
                    linkIndex = runQueue.front();
                    linkFlag[linkIndex] = false;
                    runQueue.pop();
                    std::string parentNode = links[linkIndex].srcNodeInstance;
                    std::vector<int> &parentOutputLinkIndexs = outputLinkMap[parentNode];
                    auto itVec = std::find(parentOutputLinkIndexs.begin(),
                                           parentOutputLinkIndexs.end(), linkIndex);
                    parentOutputLinkIndexs.erase(itVec);
                    if (parentOutputLinkIndexs.size() == 0) {
                        std::vector<int> &parentInputLinkIndexs = inputLinkMap[parentNode];
                        for (int parentInputLinkIndex : parentInputLinkIndexs) {
                            runQueue.push(parentInputLinkIndex);
                        }
                    }
                }
            } else { // Delete the link related to the output port
                if (outputLinkMap.find(nodeInstance) == outputLinkMap.end()) {
                    MLOGI(Mia2LogGroupCore,
                          "Node %s with dst port %d does not exist in %s pipeline",
                          nodeInstance.c_str(), portId, pipeline.pipelineName.c_str());
                    continue;
                }
                std::vector<int> &outputLinks = outputLinkMap[nodeInstance];
                int linkIndex = -1;
                for (int index : outputLinks) {
                    if (links[index].srcNodePortId == portId) {
                        linkIndex = index;
                        break;
                    }
                }
                if (linkIndex == -1) {
                    MLOGE(Mia2LogGroupCore,
                          "Please check whether the configuration information is correct");
                    continue;
                }
                std::queue<int> runQueue;
                runQueue.push(linkIndex);
                // Find the link to be deleted and delete it
                while (!runQueue.empty()) {
                    linkIndex = runQueue.front();
                    linkFlag[linkIndex] = false;
                    runQueue.pop();
                    std::string childNode = links[linkIndex].dstNodeInstance;
                    std::vector<int> &childInputLinkIndexs = inputLinkMap[childNode];
                    auto itVec = std::find(childInputLinkIndexs.begin(), childInputLinkIndexs.end(),
                                           linkIndex);
                    childInputLinkIndexs.erase(itVec);
                    if (childInputLinkIndexs.size() == 0) {
                        std::vector<int> &childOutputLinkIndexs = outputLinkMap[childNode];
                        for (int childOutputLinkIndex : childOutputLinkIndexs) {
                            runQueue.push(childOutputLinkIndex);
                        }
                    }
                }
            }
        }
    }

    std::vector<LinkItem> result;
    for (int index = 0; index < linkFlag.size(); ++index) {
        if (linkFlag[index]) {
            result.push_back(links[index]);
        }
    }
    pipeline.links = result;
}

void PipelinePruner::pipelineNodePrune(
    std::map<std::string, std::vector<throughPath>> &nodesBypassRule,
    std::vector<std::string> normalDisableNodes, PipelineInfo &pipeline)
{
    std::vector<LinkItem> &links = pipeline.links;
    int rawSize = links.size();
    std::vector<int> linkIndexTable;
    for (int index = 0; index < rawSize; ++index) {
        linkIndexTable.push_back(index);
    }

    auto getRealLinkIndex = [&linkIndexTable](int index) -> int {
        int realLinkIndex = linkIndexTable[index];
        while (realLinkIndex != index) {
            index = realLinkIndex;
            realLinkIndex = linkIndexTable[index];
        }
        return realLinkIndex;
    };

    std::map<std::string, std::map<int, int>> inputPortAndLink;  // instance<->{portId<->linkIndex}
    std::map<std::string, std::map<int, int>> outputPortAndLink; // instance<->{portId<->linkIndex}
    for (int linkIndex = 0; linkIndex < rawSize; linkIndex++) {
        LinkItem &link = links[linkIndex];
        std::string &linkSrcInstance = link.srcNodeInstance;
        int srcPortId = link.srcNodePortId;
        std::string &linkDstInstance = link.dstNodeInstance;
        int dstPortId = link.dstNodePortId;

        inputPortAndLink[linkDstInstance].insert({dstPortId, linkIndex});
        outputPortAndLink[linkSrcInstance].insert({srcPortId, linkIndex});
    }

    // Delete the disabled node without customization information
    for (std::string &instance : normalDisableNodes) {
        auto itMapFind = inputPortAndLink.find(instance);
        if (itMapFind == inputPortAndLink.end() || itMapFind->second.size() == 0) {
            MLOGI(Mia2LogGroupCore, "nodes prune tracking 1");
            continue;
        }
        itMapFind = outputPortAndLink.find(instance);
        if (itMapFind == outputPortAndLink.end() || itMapFind->second.size() == 0) {
            MLOGI(Mia2LogGroupCore, "nodes prune tracking 2");
            continue;
        }
        // By default,the first inputLink and the first outputLink are fused to generate new link
        auto &inputs = inputPortAndLink[instance];
        auto &outputs = outputPortAndLink[instance];
        int inputLinkIndex = inputs.begin()->second;
        int outputLinkIndex = outputs.begin()->second;
        inputLinkIndex = getRealLinkIndex(inputLinkIndex);
        outputLinkIndex = getRealLinkIndex(outputLinkIndex);

        std::string srcInstance = links[inputLinkIndex].srcNodeInstance;
        int srcPortId = links[inputLinkIndex].srcNodePortId;
        MiaPixelFormat srcPortFormat = links[inputLinkIndex].srcPortFormat;
        std::string dstInstance = links[outputLinkIndex].dstNodeInstance;
        int dstPortId = links[outputLinkIndex].dstNodePortId;
        std::vector<MiaPixelFormat> dstPortFormat(links[outputLinkIndex].dstPortFormat);
        links.push_back(
            {srcPortId, srcInstance, srcPortFormat, dstPortId, dstInstance, dstPortFormat});

        linkIndexTable.push_back(links.size() - 1);
        linkIndexTable[inputLinkIndex] = linkIndexTable[outputLinkIndex] = links.size() - 1;
    }

    // Delete the disabled node have customization information
    for (auto &itMap : nodesBypassRule) {
        std::string instance = itMap.first;
        if (inputPortAndLink.find(instance) == inputPortAndLink.end() ||
            outputPortAndLink.find(instance) == outputPortAndLink.end()) {
            MLOGI(Mia2LogGroupCore, "nodes prune tracking 3");
            continue;
        }

        // Delete node according to customized information
        for (auto &path : itMap.second) {
            int inputPortId = path.inputPortId;
            int outputPortId = path.outputPortId;
            auto &inputs = inputPortAndLink[instance];
            auto &outputs = outputPortAndLink[instance];

            int inputLinkIndex = -1;
            int outputLinkIndex = -1;
            auto itInputLinkIndex = inputs.find(inputPortId);
            if (itInputLinkIndex != inputs.end()) {
                inputLinkIndex = itInputLinkIndex->second;
            }
            auto itOutputLinkIndex = outputs.find(outputPortId);
            if (itOutputLinkIndex != outputs.end()) {
                outputLinkIndex = itOutputLinkIndex->second;
            }

            if (inputLinkIndex == -1 || outputLinkIndex == -1) {
                MLOGE(Mia2LogGroupCore,
                      "Please check whether the configuration information is correct");
                continue;
            }
            inputLinkIndex = getRealLinkIndex(inputLinkIndex);
            outputLinkIndex = getRealLinkIndex(outputLinkIndex);

            std::string srcInstance = links[inputLinkIndex].srcNodeInstance;
            int srcPortId = links[inputLinkIndex].srcNodePortId;
            MiaPixelFormat srcPortFormat = links[inputLinkIndex].srcPortFormat;
            std::string dstInstance = links[outputLinkIndex].dstNodeInstance;
            int dstPortId = links[outputLinkIndex].dstNodePortId;
            std::vector<MiaPixelFormat> dstPortFormat(links[outputLinkIndex].dstPortFormat);
            links.push_back(
                {srcPortId, srcInstance, srcPortFormat, dstPortId, dstInstance, dstPortFormat});

            linkIndexTable.push_back(links.size() - 1);
            linkIndexTable[inputLinkIndex] = linkIndexTable[outputLinkIndex] = links.size() - 1;
        }
    }

    std::vector<int> linkTable(linkIndexTable.size(), false);
    std::vector<int> activeLinks; // Delete duplicate values
    for (int index = 0; index < rawSize; ++index) {
        int value = getRealLinkIndex(index);
        if (!linkTable[value]) {
            activeLinks.push_back(value);
            linkTable[value] = true;
        }
    }

    std::vector<LinkItem> result;
    for (int index : activeLinks) {
        result.push_back(links[index]);
    }
    pipeline.links = result;
}

bool PipelinePruner::checkPipelineLink(PipelineInfo pipeline, uint32_t firstFrameInputFormat,
                                       std::vector<MiaImageFormat> &sinkBufferFormat)
{
    std::map<std::string, std::vector<int>> allInputLinkMap;
    std::map<std::string, int> sinkLinks;
    std::map<std::string, NodeItem> &nodeItem = pipeline.nodes;
    int sinkPortNumber = 0;
    int sourcePortNumber = 0;
    bool portConfigureFormat = false;

    for (int index = 0; index < pipeline.links.size(); index++) {
        std::string &dstNodeInstance = pipeline.links[index].dstNodeInstance;
        if (!portConfigureFormat) {
            if (pipeline.links[index].dstPortFormat[0] != CAM_FORMAT_UNDEFINED ||
                pipeline.links[index].srcPortFormat != CAM_FORMAT_UNDEFINED) {
                portConfigureFormat = true;
            }
        }
        if (pipeline.links[index].type == SourceLink) {
            // Calculate the number of sourceports
            sourcePortNumber++;
        }
        if (pipeline.links[index].type == SinkLink) {
            std::string &instance = pipeline.links[index].dstNodeInstance;
            sinkLinks[instance] = index;
            NodeItem item = {"sinkbuffer", instance, 0, false};
            nodeItem.insert(std::make_pair(instance, item));
            // Set format to sinkport，The format of the sinkport may be different
            pipeline.links[index].dstPortFormat[0] =
                MiaPixelFormat(sinkBufferFormat[sinkPortNumber++].format);
        }
        allInputLinkMap[dstNodeInstance].push_back(index);
    }

    for (auto it : sinkLinks) {
        std::string instance = it.first;
        std::set<std::string> sourceNodeVisitedSet;
        int processLinkIndex = allInputLinkMap[instance][0];
        std::string dstInstance =
            pipeline.links[processLinkIndex].dstNodeInstance; // dstInstance is Node[SinkBuffer]

        if (!checkLinkInformation(allInputLinkMap, dstInstance, pipeline.links,
                                  sourceNodeVisitedSet)) {
            return false;
        }

        // If all ports in the pipeline have no configuration format,
        // then check the format of sourceport and sinkport
        if (!portConfigureFormat) {
            if (firstFrameInputFormat != pipeline.links[processLinkIndex].dstPortFormat[0]) {
                MLOGE(Mia2LogGroupCore,
                      "all port doesn't configure a format. However srcport's format:%d doesn't "
                      "match [%s]'s port format:%d",
                      firstFrameInputFormat,
                      pipeline.links[processLinkIndex].dstNodeInstance.c_str(),
                      pipeline.links[processLinkIndex].dstPortFormat[0]);
                return false;
            }
        }
        if (sourceNodeVisitedSet.size() != sourcePortNumber) {
            std::ostringstream errorInfo;
            for (auto i : sourceNodeVisitedSet) {
                errorInfo << i << ' ';
            }
            MLOGE(Mia2LogGroupCore, "There is no walk through all sourceport. we traversed %s",
                  errorInfo.str().c_str());
            return false;
        }
    }

    return true;
}

bool PipelinePruner::checkLinkInformation(std::map<std::string, std::vector<int>> &allInputLinkMap,
                                          std::string &dstInstance,
                                          const std::vector<LinkItem> &pipelineLinks,
                                          std::set<std::string> &sourcePortVisited)
{
    bool findSourceLink = false;
    // all inputlinks for a node
    const std::vector<int> &processLinkVec = allInputLinkMap.at(dstInstance);

    for (int index = 0; index < processLinkVec.size(); index++) {
        int processLinkIndex = processLinkVec[index];
        const LinkItem &processLink = pipelineLinks[processLinkIndex];
        std::string nextDstInstance = processLink.srcNodeInstance;

        // When a sourcelink is detected, the counter of the sourceport is decremented by one, and
        // when all the source ports are reached, the link is considered to be correct.
        if (processLink.type == SourceLink) {
            // When traversing the link, there are two paths that can go to the same source port
            // through the virtual node.
            std::string sourcePortKey =
                processLink.srcNodeInstance + to_string(processLink.srcNodePortId);
            sourcePortVisited.insert(sourcePortKey);
            findSourceLink = true;
            continue;
        }

        // The formats of the two ports on the current link do not match
        if (find(processLink.dstPortFormat.begin(), processLink.dstPortFormat.end(),
                 processLink.srcPortFormat) == processLink.dstPortFormat.end()) {
            // If the format on a port is CAM_FORMAT_UNDEFINED, the format of this link is
            // considered correct, because CAM_FORMAT_UNDEFINED can be in any format
            if (processLink.srcPortFormat != CAM_FORMAT_UNDEFINED &&
                processLink.dstPortFormat[0] != CAM_FORMAT_UNDEFINED) {
                std::ostringstream dstFormat;
                for (auto i : processLink.dstPortFormat) {
                    dstFormat << i << ' ';
                }
                MLOGE(Mia2LogGroupCore,
                      "node[%s]'s port format:%s doesn't match node[%s]'s port format:%d",
                      processLink.dstNodeInstance.c_str(), dstFormat.str().c_str(),
                      processLink.srcNodeInstance.c_str(), processLink.srcPortFormat);
                return false;
            }
        }

        findSourceLink = checkLinkInformation(allInputLinkMap, nextDstInstance, pipelineLinks,
                                              sourcePortVisited);
    }

    return findSourceLink;
}

void PipelinePruner::dumpLinkInformation(const std::string &pruneResult,
                                         const PipelineInfo &pipeline, const MiaParams &settings)
{
    static const char *dumpFilePath =
        "data/vendor/camera/offlinelog/MiCam_Debug_PipelineErrorInfoDump.txt";
    static const off_t MAX_PipelineErrorDumpFile_SIZE = 512 * 1024;
    bool clearFile = false;
    bool isFirstOpenDumpFile = false;
    char errorLink[1024] = {0};
    char linkInformation[128] = {0};

    std::string pruneResults = "disablenode:" + pruneResult + '\n';
    strcpy(errorLink, pruneResults.c_str());
    for (auto i : pipeline.links) {
        std::ostringstream dstFormat;
        for (auto j : i.dstPortFormat) {
            dstFormat << j << ' ';
        }
        sprintf(linkInformation,
                "Link is [%s:outport %d format %d] -----> [%s:inport %d format %s]\n",
                i.srcNodeInstance.c_str(), i.srcNodePortId, i.srcPortFormat,
                i.dstNodeInstance.c_str(), i.dstNodePortId, dstFormat.str().c_str());
        strcat(errorLink, linkInformation);
    }

    struct stat myStat;
    if (!stat(dumpFilePath, &myStat)) {
        if (myStat.st_size > MAX_PipelineErrorDumpFile_SIZE) {
            clearFile = true;
        }
    } else {
        isFirstOpenDumpFile = true;
    }
    FILE *pf = nullptr;
    if (clearFile) {
        pf = fopen(dumpFilePath, "w+");
    } else {
        pf = fopen(dumpFilePath, "a+");
    }
    if (isFirstOpenDumpFile) {
        chmod(dumpFilePath, 0666);
    }
    if (pf) {
        fwrite(errorLink, strlen(errorLink) * sizeof(char), sizeof(char), pf);
        fclose(pf);
    }

    VendorMetadataParser::dumpMetadata(
        reinterpret_cast<const camera_metadata_t *>(settings.metadata), dumpFilePath, true);
}

void MiaCustomization::pipelineSelect(const camera_metadata_t *metadata /*no use now*/,
                                      const PipelineInfo &pipeInfo, std::string &pipelineName,
                                      std::vector<LinkItem> &links)
{
#if 0
    // PseudoCode example
    if (isSatMode) { // use metadata to determine
        pipelineName = "SatSnapShot";
        links = pipeInfo.links["SatSnapShot"];
    }
#endif
}

void MiaCustomization::disablePortSelect(const std::string &pipelineName,
                                         camera_metadata_t *metadata,
                                         std::map<std::string, std::set<disablePort>> &disablePorts)
{
    if (pipelineName == "SatSnapshot" || pipelineName == "SatSnapshotJpeg" ||
        pipelineName == "thirdpartyjpegsnapshot" ||
        pipelineName == "thirdpartysnapshot") { // satsnapshot.json & SDK snapshot json
        void *data = NULL;
        VendorMetadataParser::getVTagValue(metadata, "xiaomi.hdr.sr.enabled", &data);
        if ((NULL == data) || (NULL != data && 0 == *static_cast<uint8_t *>(data))) {
            disablePorts["HDRInstance0"].insert({1, INPUT}); // disable HDRInstance0's inputPort 1
        } else {
            int hdrSrEnable = *static_cast<int *>(data);
            MLOGI(Mia2LogGroupCore, "hdrSrEnable: %d", hdrSrEnable);
        }
    }
}

bool MiaCustomization::nodeBypassRuleSelect(
    const std::string &pipelineName, camera_metadata_t *metadata, const std::string &nodeInstance,
    std::map<std::string, std::vector<throughPath>> &nodesBypassRule,
    std::map<std::string, std::set<disablePort>> &disablePorts)
{
    bool haveMatch = false;
    if (pipelineName == "SatSnapshot" || pipelineName == "SatSnapshotJpeg" ||
        pipelineName == "thirdpartyjpegsnapshot" ||
        pipelineName == "thirdpartysnapshot") { // satsnapshot.json
        if (nodeInstance == "HDRInstance0") {
            void *data = NULL;
            VendorMetadataParser::getVTagValue(metadata, "xiaomi.hdr.sr.enabled", &data);
            if ((NULL == data) || (NULL != data && 0 == *static_cast<uint8_t *>(data))) {
                nodesBypassRule["HDRInstance0"].push_back({0, 0});
                disablePorts["HDRInstance0"].insert({1, INPUT});
                haveMatch = true;
            } else {
                int hdrSrEnable = *static_cast<int *>(data);
                MLOGI(Mia2LogGroupCore, "hdrSrEnable: %d", hdrSrEnable);
            }
        } else if (nodeInstance == "CaptureFusionInstance0") {
            void *data = NULL;
            int32_t captureHint = 0;
            VendorMetadataParser::getVTagValue(metadata, "xiaomi.burst.captureHint", &data);
            if (data != NULL) {
                captureHint = *static_cast<int32_t *>(data);
            }
            VendorMetadataParser::getVTagValue(metadata, "xiaomi.capturefusion.isFusionOn", &data);
            if ((NULL == data) || (NULL != data && 0 == *static_cast<uint8_t *>(data)) ||
                (1 == captureHint)) {
                nodesBypassRule["CaptureFusionInstance0"].push_back({0, 0});
                disablePorts["CaptureFusionInstance0"].insert({1, INPUT});
                haveMatch = true;
            } else {
                int captureFusionEnable = *static_cast<int *>(data);
                MLOGI(Mia2LogGroupCore, "captureFusionEnable: %d", captureFusionEnable);
            }
        } // else if(nodeInstance == "OTHER"){}
    }     // else if(pipelineName == "OTHER"){}

    return haveMatch;
}

void MiaCustomization::resetNodeMask(camera_metadata_t *metadata, const std::string &instance,
                                     PipelineInfo &pipeline)
{
    if (instance.find("SrInstance1") != std::string::npos) {
        void *data = NULL;
        VendorMetadataParser::getVTagValue(metadata, "xiaomi.capturefusion.isFusionOn", &data);
        if (NULL != data && 1 == *static_cast<uint8_t *>(data)) {
            auto itInstance = pipeline.nodes.find("SrInstance1");
            if (itInstance != pipeline.nodes.end()) {
                itInstance->second.nodeMask ^= (1 << 2); // SCALE_MODE = (1 << 2)
            }
        }
    }
}

void MiaCustomization::reMapSourcePortId(uint32_t &sourcePort, camera_metadata_t *metadata,
                                         camera_metadata_t *phyMetadata,
                                         const std::string &pipelineName, sp<MiaImage> inputImage)
{
    /*
     * Workaround: Since APP shares the same stream_index between the slave input of Fusion
     * and the no sr input of HDR, in order to adapt SR+Fusion+HDR in satsnapshot.json, we
     * distinguish the SourceBuffer port here.
     */
    if (metadata != NULL) {
        if (sourcePort == 0) {
            if (pipelineName == "DualBokehSnapshot") {
                void *data = NULL;
                int mdmode = 0;
                int curReqIndex = 0;
                int evValue = 0;
                int superNightEnabled = 0;
                VendorMetadataParser::getVTagValue(metadata, "xiaomi.bokeh.MDMode", &data);
                if (NULL != data) {
                    mdmode = *reinterpret_cast<uint8_t *>(data);
                }
                VendorMetadataParser::getVTagValue(metadata, "xiaomi.burst.curReqIndex", &data);
                if (NULL != data) {
                    curReqIndex = *reinterpret_cast<int32_t *>(data);
                }
                VendorMetadataParser::getVTagValue(metadata, "xiaomi.bokeh.superNightEnabled",
                                                   &data);
                if (NULL != data) {
                    superNightEnabled = *reinterpret_cast<uint8_t *>(data);
                }
                VendorMetadataParser::getTagValue(metadata,
                                                  ANDROID_CONTROL_AE_EXPOSURE_COMPENSATION, &data);
                if (NULL != data) {
                    evValue = *reinterpret_cast<int32_t *>(data);
                }

                if ((curReqIndex != inputImage->getMergeInputNumber()) && mdmode) {
                    uint32_t inputNum = inputImage->getMergeInputNumber() - 1;
                    inputImage->setMergeInputNumber(inputNum);
                    VendorMetadataParser::setVTagValue(metadata, "xiaomi.multiframe.inputNum",
                                                       &inputNum, 1);
                    if (phyMetadata != NULL) {
                        VendorMetadataParser::setVTagValue(
                            phyMetadata, "xiaomi.multiframe.inputNum", &inputNum, 1);
                    }
                    MLOGI(Mia2LogGroupCore,
                          "MDbokeh SE mode, multiframe inputNum change from %d to %d. "
                          "[MAIN]EV: %d MDMode: %d CurReqIndex: %d  superNightEnabled: %d",
                          inputNum + 1, inputNum, evValue, mdmode, curReqIndex, superNightEnabled);
                    return;
                }
                if ((evValue == -24) && mdmode &&
                    (curReqIndex == inputImage->getMergeInputNumber())) {
                    sourcePort = 2;
                    inputImage->setMergeInputNumber(1);
                    uint32_t inputNum = inputImage->getMergeInputNumber();
                    VendorMetadataParser::setVTagValue(metadata, "xiaomi.multiframe.inputNum",
                                                       &inputNum, 1);
                    if (phyMetadata != NULL) {
                        VendorMetadataParser::setVTagValue(
                            phyMetadata, "xiaomi.multiframe.inputNum", &inputNum, 1);
                    }
                    MLOGI(Mia2LogGroupCore,
                          "MDbokeh SE mode, set sourcePort from 0 to 2,multiframe inputNum "
                          "change from  7 to 1. [MAIN]EV: %d MDMode: %d CurReqIndex: %d  "
                          "superNightEnabled: %d",
                          evValue, mdmode, curReqIndex, superNightEnabled);
                    return;
                }
            }
        } else if (sourcePort == 1) {
            if (pipelineName == "SatSnapshot") {
                bool miFusionEnable = false;
                void *pData = NULL;
                const char *FusionEnabled = "xiaomi.capturefusion.isFusionOn";
                VendorMetadataParser::getVTagValue(metadata, FusionEnabled, &pData);
                if (NULL != pData) {
                    miFusionEnable = *static_cast<bool *>(pData);
                }
                if (miFusionEnable) {
                    sourcePort = 2;
                    MLOGI(Mia2LogGroupCore, "MiFusion isFusionOn:true set sourcePort from 1 to 2");
                    return;
                }
            }
        }
    } else {
        MLOGW(Mia2LogGroupCore, "parameter metadata's addr is NULL");
    }
}

} // namespace mialgo2
