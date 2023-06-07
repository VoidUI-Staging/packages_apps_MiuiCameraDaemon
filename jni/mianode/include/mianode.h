#ifndef __H_MI_NODE_H__
#define __H_MI_NODE_H__
#include "miautil.h"

#define MAX_INPUT_NUM (10)
namespace mialgo {

// node type definition format: NDTP_MANUFACTOR_ALGONAME
typedef enum
{
    NDTP_INVALID = 0,
    NDTP_CLEARSHOT,
    NDTP_CLEARSHOT_S,
    NDTP_HHT,
    NDTP_MAX
} mia_node_type_t;

typedef struct _NodeProcessRequestInfo {
    uint64_t frameNum;                          //< Frame number for current request
    void *meta;                                 //< native CameraMetadata*
    uint32_t inputNum;                          //< Number of input buffer handles
    MiImageList_p phInputBuffer[MAX_INPUT_NUM]; //< Pointer to array of input buffer handles
    uint32_t outputNum;                         //< Number of output buffer
    MiImageList_p phOutputBuffer[1];            //< Pointer to array of output buffer handles
    int32_t baseFrameIndex;                     //< algo output
    int32_t cropBoundaries;                     //< Crop image boundaries and scale back to full size (=1), or keep the original FOV of the reference frame (=0)
} NodeProcessRequestInfo;

class MiaNode : public virtual RefBase {
public:
    MiaNode();

    virtual ~MiaNode();

    virtual CDKResult Initialize(void);
    virtual CDKResult DeInit(void);
    virtual CDKResult PrePareMetaData(void *meta);
    virtual CDKResult FreeMetaData(void);
    virtual CDKResult ProcessRequest(NodeProcessRequestInfo *pProcessRequestInfo);

    inline void setType(mia_node_type_t node_type) { mNodeType = node_type; }
    inline mia_node_type_t getType() { return  mNodeType; }
private:
    mia_node_type_t mNodeType;
};

}

#endif //END define __H_MI_NODE_H__
