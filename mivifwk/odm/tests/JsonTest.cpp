#include <utils/Log.h>

#include <iostream>
#include <string>
#include <vector>

#include "MiaPluginUtils.h"
#include "rapidjson/Document.h"
#include "rapidjson/StringBuffer.h"
#include "rapidjson/Writer.h"

#undef LOG_TAG
#define LOG_TAG "JsonTest"

using namespace std;
using namespace rapidjson;
using namespace mialgo2;

struct NodeItem
{
    string nodeName;
    int nodeInstanceId;
};

struct LinkItem
{
    int srcPortId;
    int srcNodeInstanceId;
    int dstPortId;
    int dstNodeInstanceId;
};

struct PipelineInfo
{
    string pipelineName;
    // int PipelineId;
    vector<NodeItem> nodes;
    vector<LinkItem> links;
};

string readFile(const char *fileName)
{
    FILE *fp = fopen(fileName, "rb");
    if (!fp) {
        printf("open failed! file: %s", fileName);
        return "";
    }
    MLOGI(Mia2LogGroupCore, " file %s open succeed! ", __FUNCTION__, fileName);

    char *buf = new char[1024 * 16];
    int n = fread(buf, 1, 1024 * 16, fp);
    fclose(fp);

    string result;
    if (n >= 0) {
        result.append(buf, 0, n);
    }
    delete[] buf;
    return result;
}

int parseJSON(const char *jsonStr, PipelineInfo *pipeline)
{
    Document doc;
    if (doc.Parse(jsonStr).HasParseError()) {
        MLOGE(Mia2LogGroupCore, "parse error!");
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
    MLOGI(Mia2LogGroupCore, "getPipelineName: %s", pipeline->pipelineName.c_str());

    //    if (!doc.HasMember("PipelineId")) {
    //        MLOGE(Mia2LogGroupCore, "'errorCode' no found!");
    //        return -1;
    //    }

    //    pipeline->PipelineId = doc["PipelineId"].GetInt();
    //    MLOGE(Mia2LogGroupCore, " get PipelineId: %d", pipeline->PipelineId);

    if (doc.HasMember("NodesList") && doc["NodesList"].IsObject() && !doc["NodesList"].IsNull()) {
        if (doc["NodesList"]["Node"].IsNull()) {
            MLOGE(Mia2LogGroupCore, "NodesList is null!");
            return -1;
        }
        const rapidjson::Value &object = doc["NodesList"]["Node"];
        MLOGI(Mia2LogGroupCore, "get node size: %d", object.Size());
        for (SizeType i = 0; i < object.Size(); ++i) {
            struct NodeItem node;
            if (object[i].HasMember("NodeName") && object[i]["NodeName"].IsString()) {
                node.nodeName = object[i]["NodeName"].GetString();
                MLOGI(Mia2LogGroupCore, "get Node[%d] name: %s", i, node.nodeName.c_str());
            }
            if (object[i].HasMember("NodeInstanceId") && object[i]["NodeInstanceId"].IsInt()) {
                node.nodeInstanceId = object[i]["NodeInstanceId"].GetInt();
                MLOGI(Mia2LogGroupCore, "get Node[%d]  id: %d", i, node.nodeInstanceId);
            }
            pipeline->nodes.push_back(node);
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
            link.srcPortId = object[i]["SrcPort"]["PortId"].GetInt();
            link.srcNodeInstanceId = object[i]["SrcPort"]["NodeInstanceId"].GetInt();
            link.dstPortId = object[i]["DstPort"]["PortId"].GetInt();
            link.dstNodeInstanceId = object[i]["DstPort"]["NodeInstanceId"].GetInt();
            pipeline->links.push_back(link);
        }
    }

    return 0;
}

int main()
{
    MLOGI(Mia2LogGroupCore, "%s: %d", __func__, __LINE__);
    printf("JsonTest start!");

    string jsonStr = readFile("/vendor/etc/camera/xiaomi/testmemcpy.json");

    struct PipelineInfo pipeline;
    parseJSON(jsonStr.c_str(), &pipeline);

    MLOGI(Mia2LogGroupCore, "PipelineName is:  %s", pipeline.pipelineName.c_str());
    // MLOGE(Mia2LogGroupCore, "PipelineId is:  %d", pipeline.PipelineId);
    MLOGI(Mia2LogGroupCore, "node size  is:  %lu", pipeline.nodes.size());
    printf("JsonTest finished! node size  is:%lu!", pipeline.nodes.size());

    return 0;
}
