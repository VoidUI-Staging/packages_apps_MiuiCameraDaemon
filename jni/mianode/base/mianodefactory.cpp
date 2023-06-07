////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////
/// @file  mianodeclearshot.cpp
/// @brief Chi node for clearshot
////////////////////////////////////////////////////////////////////////////////////////////////////

#include <inttypes.h>
#include <utils/Timers.h>

#include "mianodeclearshot.h"
#include "mianodeclearshot_s.h"
#include "mianodefactory.h"

// NOWHINE FILE CP040: Keyword new not allowed. Use the CAMX_NEW/CAMX_DELETE functions insteads
// NOWHINE FILE NC008: Warning: - Var names should be lower camel case

#undef LOG_TAG
#define LOG_TAG "MIA_NODE_FACTORY"

namespace mialgo {

MiaNodeFactory* MiaNodeFactory::GetInstance()
{
    static MiaNodeFactory s_mianodefactorysingleton;

    return &s_mianodefactorysingleton;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// MiaNodeFactory::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////
CDKResult MiaNodeFactory::Initialize(mia_node_type_t node_type)
{
    CDKResult result = MIAS_OK;
    MiaNode *mianode = NULL;

    ALOGE("mia node:%d", node_type);

    switch (node_type) {
    case NDTP_CLEARSHOT:
        mianode = new MiaNodeClearShot;
        break;
    case NDTP_CLEARSHOT_S:
        // This is for single camera scenario
        mianode = new MiaNodeClearShot_S;
        break;
    case NDTP_HHT:
        //mianode = new MiaNodeHHT;
        break;
    default:
        ALOGE("unsupport mia node:%d", node_type);
        return MIAS_INVALID_PARAM;
    }

    XM_CHECK_NULL_POINTER(mianode, MIAS_INVALID_CALL);

    /* Init */
    mianode->Initialize();

    m_mianode[node_type] = mianode;

    ALOGI("OK!");
    return result;
}

CDKResult MiaNodeFactory::DeInit(mia_node_type_t node_type)
{
    CDKResult result = MIAS_OK;
    MiaNode *mianode = m_mianode[node_type];

    XM_CHECK_NULL_POINTER(mianode, MIAS_INVALID_CALL);

    mianode->DeInit();
    delete mianode;
    m_mianode[node_type] = NULL;

    return result;
}

MiaNode * MiaNodeFactory::GetMiaNode(mia_node_type_t node_type)
{
    MiaNode *mianode = m_mianode[node_type];

    XM_CHECK_NULL_POINTER(mianode, NULL);

    return m_mianode[node_type];
}

MiaNodeFactory::MiaNodeFactory()
{
    ALOGD("now construct -->> MiaNodeFactory\n");
    memset(m_mianode, 0, sizeof(m_mianode));
}

MiaNodeFactory::~MiaNodeFactory()
{
    ALOGD("now distruct -->> MiaNodeFactory\n");
}

}
// CAMX_NAMESPACE_END
