#ifndef __SESSION_INFO_H__
#define __SESSION_INFO_H__

#include <MiaPluginWraper.h>
class SessionInfo
{
public:
    void setCreatInfo(CreateInfo *createInfo)
    {
        if (createInfo)
            this->createInfo = *createInfo;
    }
    void setNodeInterface(MiaNodeInterface *nodeInterface)
    {
        if (nodeInterface)
            this->nodeInterface = *nodeInterface;
    }

    uint32_t getOperationMode() const { return createInfo.operationMode; }
    uint32_t getFrameworkCameraId() const { return createInfo.frameworkCameraId; }

    void setResultMetadata(uint32_t frameNum, int64_t timeStamp, std::string result) const
    {
        if (nodeInterface.pSetResultMetadata)
            nodeInterface.pSetResultMetadata(nodeInterface.owner, frameNum, timeStamp, result);
    }

    void setOutputFormat(ImageFormat imageFormat) const
    {
        if (nodeInterface.pSetOutputFormat)
            nodeInterface.pSetOutputFormat(nodeInterface.owner, imageFormat);
    }

    void setResultBuffer(uint32_t frameNum) const
    {
        if (nodeInterface.pSetResultBuffer)
            nodeInterface.pSetResultBuffer(nodeInterface.owner, frameNum);
    }

private:
    CreateInfo createInfo;
    MiaNodeInterface nodeInterface;
};

#endif
