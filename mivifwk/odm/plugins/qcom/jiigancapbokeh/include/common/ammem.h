// clang-format off
#ifndef __AMMEM_H__
#define __AMMEM_H__

#include "amcomdef.h"


#ifdef __cplusplus
extern "C" {
#endif


MVoid*  MMemAlloc(MHandle hContext, MLong lSize);
MVoid*  MMemRealloc(MHandle hContext, MVoid* pMem, MLong lSize);
MVoid   MMemFree(MHandle hContext, MVoid* pMem);
MVoid   MMemSet(MVoid* pMem, MByte byVal, MLong lSize);
MVoid   MMemCpy(MVoid* pDst, const MVoid* pSrc, MLong lSize);
MVoid   MMemMove(MVoid* pDst, MVoid* pSrc, MLong lSize);
MLong   MMemCmp(MVoid* pBuf1, MVoid* pBuf2, MLong lSize);

MHandle MMemMgrCreate(MVoid* pMem, MLong lMemSize);
MVoid   MMemMgrDestroy(MHandle hMemMgr);

MLong   GetMaxAllocMemSize(MHandle hContext);


typedef struct
{
    MLong lCodebase;                // Codebase version number
    MLong lMajor;                   // major version number
    MLong lMinor;                   // minor version number
    MLong lBuild;                   // Build version number, increasable only
    const MChar *Version;           // version in string form
    const MChar *BuildDate;         // latest build Date
    const MChar *CopyRight;         // copyright
}MPBASE_Version;

const MPBASE_Version* Mpbase_GetVersion();

#ifdef __cplusplus
}
#endif

#endif
// clang-format on
