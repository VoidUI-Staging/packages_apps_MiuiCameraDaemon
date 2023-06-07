#ifndef _MI_JSONUTILS_H_
#define _MI_JSONUTILS_H_

#include "MiaUtil.h"

namespace mialgo2 {

enum LinkType { SourceLink, InternalLink, SinkLink };

enum OutputType { OneBuffer, FollowInput, Custom };

struct NodeItem
{
    std::string nodeName;
    std::string nodeInstance;
    int nodeMask;
    bool outputBufferNeedCheck;
    bool isEnable;

    /* There are three types of output port buffer numbers:
     * OneBuffer: By default,one output port can only transmit one buffer.
     * FollowInput: The number of buffers that can be transmitted by output port follows the
     * number of buffers transmitted by input port of the node.
     * Custom: Node decides how many buffers can be transmitted for each output port.
     */
    OutputType outputType;
};

struct LinkItem
{
    int srcNodePortId;
    std::string srcNodeInstance;
    MiaPixelFormat srcPortFormat; // Must specify a format
    int dstNodePortId;
    std::string dstNodeInstance;
    std::vector<MiaPixelFormat> dstPortFormat; // You can specify input multiple formats
    LinkType type;
};

struct PipelineInfo
{
    std::string pipelineName;
    std::map<std::string, NodeItem> nodes;
    std::vector<LinkItem> links;
};

class MJsonUtils
{
public:
    static std::string readFile(const char *fileName);

    static int parseJson2Struct(const char *jsonStr, PipelineInfo *pipeline);
};

} // namespace mialgo2
#endif //_MI_JSONUTILS_H_