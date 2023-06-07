////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2017 Qualcomm Technologies, Inc.
// All Rights Reserved.
// Confidential and Proprietary - Qualcomm Technologies, Inc.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef _H_MIANODEFACTORY_H_
#define _H_MIANODEFACTORY_H_

#include "mianode.h"

namespace mialgo {

class MiaNodeFactory {
public:
    /// Return the singleton instance of MiaNodeFactory
    static MiaNodeFactory *GetInstance();

    CDKResult Initialize(mia_node_type_t node_type);

    CDKResult DeInit(mia_node_type_t node_type);

    MiaNode * GetMiaNode(mia_node_type_t node_type);

    MiaNodeFactory();

    virtual ~MiaNodeFactory();

private:
    MiaNodeFactory(const MiaNodeFactory &) = delete;    ///< Disallow the copy constructor
    MiaNodeFactory &operator = (const MiaNodeFactory &) = delete; ///< Disallow assignment operator

    MiaNode *m_mianode[NDTP_MAX];
};

}
#endif // _H_MIANODEFACTORY_H_
