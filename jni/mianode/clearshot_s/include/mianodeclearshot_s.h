////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _H_MIANODECLEARSHOT_S_H_
#define _H_MIANODECLEARSHOT_S_H_

#include "visidonclearshot_s.h"
#include "mianode.h"

namespace mialgo {
class MiaNodeClearShot_S : public MiaNode {
public:
    virtual CDKResult Initialize(void);
    virtual CDKResult DeInit(void);
    virtual CDKResult PrePareMetaData(void *meta);
    virtual CDKResult FreeMetaData(void);
    virtual CDKResult ProcessRequest(NodeProcessRequestInfo *pProcessRequestInfo);

    MiaNodeClearShot_S();
    virtual ~MiaNodeClearShot_S();

private:
    MiaNodeClearShot_S(const MiaNodeClearShot_S &) = delete;    ///< Disallow the copy constructor
    MiaNodeClearShot_S &operator = (const MiaNodeClearShot_S &) = delete; ///< Disallow assignment operator

    void UpdateMetaData(uint64_t requestId);

    uint64_t m_processedFrame;   ///< The count for processed frame
    QVisidonClearshot_S *m_pNodeInstance;
    void *m_pRequestSetting; // camera_metadata_t* type cast as void*
};
}
#endif // _H_MIANODECLEARSHOT_S_H_
