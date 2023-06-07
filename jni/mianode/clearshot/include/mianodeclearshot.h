////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _H_MIANODECLEARSHOT_H_
#define _H_MIANODECLEARSHOT_H_

#include "visidonclearshot.h"
#include "mianode.h"

namespace mialgo {
class MiaNodeClearShot : public MiaNode {
public:
    virtual CDKResult Initialize(void);
    virtual CDKResult DeInit(void);
    virtual CDKResult PrePareMetaData(void *meta);
    virtual CDKResult FreeMetaData(void);
    virtual CDKResult ProcessRequest(NodeProcessRequestInfo *pProcessRequestInfo);

    MiaNodeClearShot();
    virtual ~MiaNodeClearShot();

private:
    MiaNodeClearShot(const MiaNodeClearShot &) = delete;    ///< Disallow the copy constructor
    MiaNodeClearShot &operator = (const MiaNodeClearShot &) = delete; ///< Disallow assignment operator

    void UpdateMetaData(uint64_t requestId);

    uint64_t m_processedFrame;   ///< The count for processed frame
    QVisidonClearshot *m_pNodeInstance;
    void *m_pRequestSetting; // camera_metadata_t* type cast as void*
};
}
#endif // _H_MIANODECLEARSHOT_H_
