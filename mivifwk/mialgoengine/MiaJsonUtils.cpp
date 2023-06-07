/*
 * Copyright (c) 2020. Xiaomi Technology Co.
 *
 * All Rights Reserved.
 *
 * Confidential and Proprietary - Xiaomi Technology Co.
 */

#include "MiaJsonUtils.h"

#include "MiaUtil.h"
#include "rapidjson/Document.h"
#include "rapidjson/StringBuffer.h"
#include "rapidjson/Writer.h"
#include "rapidjson/error/En.h"

using namespace std;
using namespace rapidjson;

namespace mialgo2 {

std::map<std::string, MiaPixelFormat> NodeFormat = {
    {"YUV420_NV21", CAM_FORMAT_YUV_420_NV21},
    {"RAW16", CAM_FORMAT_RAW16},
    {"BLOB", CAM_FORMAT_BLOB},
    {"IMPLEMENTATION_DEFINED", CAM_FORMAT_IMPLEMENTATION_DEFINED},
    {"YUV420_NV12", CAM_FORMAT_YUV_420_NV12},
    {"RAW_OPAQUE", CAM_FORMAT_RAW_OPAQUE},
    {"RAW10", CAM_FORMAT_RAW10},
    {"RAW12", CAM_FORMAT_RAW12},
    {"Y16", CAM_FORMAT_Y16},
    {"YV12", CAM_FORMAT_YV12},
    {"P010", CAM_FORMAT_P010},
    {"UNDEFINED", CAM_FORMAT_UNDEFINED}};

string MJsonUtils::readFile(const char *fileName)
{
    FILE *fp = fopen(fileName, "rb");
    if (!fp) {
        MLOGE(Mia2LogGroupCore, "open failed! file: %s", fileName);
        return "";
    }

    fseek(fp, 0L, SEEK_END);
    long int length = ftell(fp);
    if (length < 0) {
        MLOGE(Mia2LogGroupCore, "ftell failed! file: %s", fileName);
        return "";
    }

    char *buf = (char *)malloc(length + 1);
    memset(buf, 0, sizeof(char) * (length + 1));

    fseek(fp, 0L, SEEK_SET);
    size_t size = fread(buf, 1, length, fp); // length must be >= 0
    buf[length] = '\0';
    fclose(fp);

    string result;
    result.append(buf, 0, length + 1);

    free(buf);
    return result;
}

int MJsonUtils::parseJson2Struct(const char *jsonStr, PipelineInfo *pipeline)
{
    Document doc;
    if (doc.Parse(jsonStr).HasParseError()) {
        MLOGE(Mia2LogGroupCore, "parse error: (%d:%lu)%s\n", doc.GetParseError(),
              doc.GetErrorOffset(), GetParseError_En(doc.GetParseError()));
        return -1;
    }

    if (!doc.IsObject()) {
        MLOGE(Mia2LogGroupCore, "should be an object!");
        return -1;
    }

    if (!doc.HasMember("PipelineName")) {
        MLOGE(Mia2LogGroupCore, "'errorCode' no found!");
        return -1;
    }
    pipeline->pipelineName = doc["PipelineName"].GetString();

    if (doc.HasMember("NodesList") && doc["NodesList"].IsObject() && !doc["NodesList"].IsNull()) {
        if (doc["NodesList"]["Node"].IsNull()) {
            MLOGE(Mia2LogGroupCore, "NodesList is null!");
            return -1;
        }
        const rapidjson::Value &object = doc["NodesList"]["Node"];
        MLOGI(Mia2LogGroupCore, "Get PipelineName: %s, node size: %d",
              pipeline->pipelineName.c_str(), object.Size());

        for (SizeType i = 0; i < object.Size(); ++i) {
            struct NodeItem node;
            node.isEnable = true;
            node.nodeMask = 0;
            node.outputType = OneBuffer;
            node.outputBufferNeedCheck = true;

            if (object[i].HasMember("NodeName") && object[i]["NodeName"].IsString()) {
                node.nodeName = object[i]["NodeName"].GetString();
            }
            if (object[i].HasMember("NodeInstance") && object[i]["NodeInstance"].IsString()) {
                node.nodeInstance = object[i]["NodeInstance"].GetString();
            }
            if (object[i].HasMember("OutputBufferNeedCheck") &&
                object[i]["OutputBufferNeedCheck"].IsBool()) {
                node.outputBufferNeedCheck = object[i]["OutputBufferNeedCheck"].GetBool();
            }

            const rapidjson::Value &propertyObject = object[i]["NodeProperty"];
            for (SizeType j = 0; j < propertyObject.Size(); ++j) {
                std::string nodePropertyName = "";
                int nodePropertyValue = 0;

                if (propertyObject[j].HasMember("NodePropertyName") &&
                    propertyObject[j]["NodePropertyName"].IsString()) {
                    nodePropertyName = propertyObject[j]["NodePropertyName"].GetString();
                }
                if (propertyObject[j].HasMember("NodePropertyValue") &&
                    propertyObject[j]["NodePropertyValue"].IsInt()) {
                    nodePropertyValue = propertyObject[j]["NodePropertyValue"].GetInt();
                }
                if (nodePropertyName == "NodeMask") {
                    node.nodeMask = nodePropertyValue;
                } else if (nodePropertyName == "OutputType") {
                    node.outputType = static_cast<OutputType>(nodePropertyValue);
                }
            }

            int type = node.outputType;
            MLOGI(Mia2LogGroupCore,
                  "get Node[%d] name: %s, nodeMask: %d, outputType: %d, instanceName: %s", i,
                  node.nodeName.c_str(), node.nodeMask, type, node.nodeInstance.c_str());
            pipeline->nodes[node.nodeInstance] = node;
        }
    }

    if (doc.HasMember("PortLinkages") && doc["PortLinkages"].IsObject() &&
        !doc["PortLinkages"].IsNull()) {
        if (doc["PortLinkages"]["Link"].IsNull()) {
            MLOGE(Mia2LogGroupCore, "PortLinkages is null!");
            return -1;
        }
        const rapidjson::Value &object = doc["PortLinkages"]["Link"];
        MLOGI(Mia2LogGroupCore, "get port link size: %d", object.Size());
        for (SizeType i = 0; i < object.Size(); ++i) {
            struct LinkItem link;
            link.srcNodePortId = object[i]["SrcPort"]["PortId"].GetInt();
            link.srcNodeInstance = object[i]["SrcPort"]["NodeInstance"].GetString();
            if (object[i]["SrcPort"].HasMember("PortFormat") &&
                object[i]["SrcPort"]["PortFormat"].IsString()) {
                link.srcPortFormat = NodeFormat.at(object[i]["SrcPort"]["PortFormat"].GetString());
            } else {
                link.srcPortFormat = CAM_FORMAT_UNDEFINED;
            }
            link.dstNodePortId = object[i]["DstPort"]["PortId"].GetInt();
            link.dstNodeInstance = object[i]["DstPort"]["NodeInstance"].GetString();
            if (object[i]["DstPort"].HasMember("PortFormat") &&
                object[i]["DstPort"]["PortFormat"].IsString()) {
                link.dstPortFormat.emplace_back(
                    NodeFormat.at(object[i]["DstPort"]["PortFormat"].GetString()));
            } else if (object[i]["DstPort"].HasMember("PortFormats") &&
                       object[i]["DstPort"]["PortFormats"].IsArray()) {
                const rapidjson::Value &PortFormatsObject = object[i]["DstPort"]["PortFormats"];
                for (int j = 0; j < PortFormatsObject.Size(); j++) {
                    link.dstPortFormat.emplace_back(
                        NodeFormat.at(PortFormatsObject[j].GetString()));
                }
            } else {
                link.dstPortFormat.emplace_back(CAM_FORMAT_UNDEFINED);
            }

            std::ostringstream dstFormat;
            for (auto j : link.dstPortFormat) {
                dstFormat << j << ' ';
            }
            MLOGI(Mia2LogGroupCore,
                  "Link[%d] [%s:outport:%d foramt:%d] -----> [%s:inport:%d format:%s]", i,
                  link.srcNodeInstance.c_str(), link.srcNodePortId, link.srcPortFormat,
                  link.dstNodeInstance.c_str(), link.dstNodePortId, dstFormat.str().c_str());

            pipeline->links.push_back(link);
        }
    }

    return 0;
}

} // namespace mialgo2
