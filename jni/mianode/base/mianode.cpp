#include "mianode.h"

#undef  LOG_TAG
#define LOG_TAG "MIA_NODE_DAEM"

namespace mialgo {

MiaNode::MiaNode()
{
    mNodeType = NDTP_INVALID;
    ALOGD("now construct -->> MiaNode\n");
}

MiaNode::~MiaNode() {
    ALOGD("now distruct  ~~~\n");
    return;
}


CDKResult MiaNode::Initialize(void)
{
    ALOGE("ERR! %s SHOULD NOT BE HERE!\n", __func__);
    return MIAS_OK;
}

CDKResult MiaNode::DeInit(void)
{
    ALOGE("ERR! %s SHOULD NOT BE HERE!\n", __func__);
    return MIAS_OK;
}

CDKResult MiaNode::PrePareMetaData(void *meta)
{
    ALOGE("ERR! %s SHOULD NOT BE HERE!\n", __func__);
    return MIAS_OK;
}

CDKResult MiaNode::FreeMetaData(void)
{
    ALOGE("ERR! %s SHOULD NOT BE HERE!\n", __func__);
    return MIAS_OK;
}

CDKResult MiaNode::ProcessRequest(NodeProcessRequestInfo *pProcessRequestInfo)
{
    ALOGE("ERR! %s SHOULD NOT BE HERE!\n", __func__);
    return MIAS_OK;
}

}
