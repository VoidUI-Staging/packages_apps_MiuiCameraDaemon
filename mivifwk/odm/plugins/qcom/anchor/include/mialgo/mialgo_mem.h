
/************************************************************************************

Filename  : mialgo_mem.h
Content   :
Created   : Nov. 14, 2019
Author    : guwei (guwei@xiaomi.com)
Copyright : Copyright 2019 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/

#ifndef MIALGO_MEM_H__
#define MIALGO_MEM_H__

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include "mialgo_type.h"
#include "mialgo_errorno.h"
#include "mialgo_basic.h"

#define MIALGO_MEM_UNCACHED                 (0)     /*!< uncached memory flag */
#define MIALGO_MEM_CACHED                   (1)     /*!< cached memory flag */

/**
* @brief Memory type.
*
* Memory usage description
*   MIALGO_MEM_HEAP:
*       Windows sysytem
*       Ubuntu sysytem
*       Android sytem : A small piece of memory on the CPU
*
*   MIALGO_MEM_ION: only used for Android sytem
*       A large block memory for storing images
*       memory used for HVX and GPU
*/
typedef enum
{
    MIALGO_MEM_NONE = 0,        /*!< invalid mem type */
    MIALGO_MEM_HEAP,            /*!< heap mem */
    MIALGO_MEM_ION,             /*!< ion mem */
    MIALGO_MEM_CL,              /*!< opencl svm */
    MIALGO_MEM_NUM,             /*!< mem type num */
} MialgoMemType;

/**
* @brief memory info.
*   For the implementation of HVX, We have to know the fd of ion buf, MialgoMemInfo is used to record this information.
*
* Description of type
*   @see MialgoMemType.
*
* Description of size
*   for alignment reason, the actual allocated memory is greater than the applied memory.
*   size is the actual allocated memory size.
*
* Description of phy_addr
*   the memory physical address, currently not in use.
*
* Description of fd
*   fd of ion memory, for heap memoy, fd is 0 and will not used.
*/
typedef struct
{
    MialgoMemType type;         /*!< memory type, @see MialgoMemType */
    MI_U64 size;                /*!< memory size */
    MI_U32 phy_addr;            /*!< physical address */
    MI_S32 fd;                  /*!< fd of ion memory */
} MialgoMemInfo;

/**
* @brief Inline function of assignment of MialgoMemInfo quickly.
*
* @return
*         --<em>mialgoMemInfo</em> , @see mialgoMemInfo.
*/
static inline MialgoMemInfo mialgoMemInfo(MialgoMemType type, MI_U64 size, MI_U32 phy_addr, MI_S32 fd)
{
    MialgoMemInfo mem_info;

    mem_info.type = type;
    mem_info.size = size;
    mem_info.phy_addr = phy_addr;
    mem_info.fd = fd;

    return mem_info;
}

/**
* @brief Inline function of assignment of a heap MialgoMemInfo quickly.
*
* @return
*         --<em>mialgoMemInfo</em> , @see mialgoMemInfo.
*/
static inline MialgoMemInfo mialgoHeapMemInfo(MI_U64 size)
{
    MialgoMemInfo mem_info;

    mem_info.type = MIALGO_MEM_HEAP;
    mem_info.size = size;
    mem_info.phy_addr = 0;
    mem_info.fd = 0;

    return mem_info;
}

/**
* @brief Inline function of assignment of a ion MialgoMemInfo quickly.
*
* @return
*         --<em>mialgoMemInfo</em> , @see mialgoMemInfo.
*/
static inline MialgoMemInfo mialgoIonMemInfo(MI_U64 size, MI_U32 phy_addr, MI_S32 fd)
{
    MialgoMemInfo mem_info;

    mem_info.type = MIALGO_MEM_ION;
    mem_info.size = size;
    mem_info.phy_addr = phy_addr;
    mem_info.fd = fd;

    return mem_info;
}

/**
* @brief used to record memory allocations, internal implementation use.
*/
typedef struct
{
    MI_U32 line;                /*!< the line of code that allocates memory */
    const MI_CHAR *func;        /*!< the function that allocates memory */
    const MI_CHAR *file;        /*!< the file that allocates memory */
} MialgoMemPos;

/**
* @brief memory allocation parameters, can specify the type of memory(@see MialgoMemType), caching(@see MIALGO_MEM_UNCACHED MIALGO_MEM_CACHED) and alignment.
*
* Default value description:
*   type == 0   : the memory type is determined by the system, for Android is ion, for others is heap
*   flag == 0   : uncached memory
*   align == 0  : for heap memory, align is 32, for ion memory, align is the page size(4096)
*/
typedef struct
{
    MI_S32 type;                /*!< memory type, must be MIALGO_MEM_HEAP/MIALGO_MEM_ION */
    MI_S32 flag;                /*!< memory caching, must be MIALGO_MEM_UNCACHED/MIALGO_MEM_CACHED */
    MI_S32 align;               /*!< memory alignment */
} MialgoAllocParam;

/**
* @brief system memory information.
*/
typedef struct MialgoMemSysInfo
{
    MI_U64 max_size;            /*!< the peak of memory */
    MI_U64 cur_size;            /*!< the current memory */
} MialgoMemSysInfo;

/**
* @brief inline function for create MialgoMemPos quickly.
* @param[in] line       @see MialgoMemPos
* @param[in] func       @see MialgoMemPos
* @param[in] func       @see MialgoMemPos
*
* @return
*        -<em>MialgoMemPos</em> record memory allocations @see MialgoMemPos.
*/
static inline MialgoMemPos mialgoMemPos(MI_U32 line, const MI_CHAR *func, const MI_CHAR *file)
{
    MialgoMemPos pos;

    pos.line = line;
    pos.func = func;
    pos.file = file;

    return pos;
}

/**
* @brief inline function for create MialgoAllocParam quickly.
* @param[in] type       @see MialgoAllocParam
* @param[in] flag       @see MialgoAllocParam
* @param[in] align      @see MialgoAllocParam
*
* @return
*        -<em>MialgoAllocParam</em> memory allocation parameters @see MialgoAllocParam.
*/
static inline MialgoAllocParam mialgoAllocParam(MI_U32 type, MI_U32 flag, MI_U32 align)
{
    MialgoAllocParam param;

    param.type = type;
    param.flag = flag;
    param.align = align;

    return param;
}

/**
* @brief allocate memory using default parameters.
* @param[in] s          memory size in bytes.
* 
* @return
*        -<em>void *</em> memory pointer
*
* Default behavior description:
*   for windows and ubunt system, allocate memory from the heap.
*   for android system, allocate memory from the ION.
*/
#define MialgoAllocate(s)               MialgoAllocInternal(mialgoMemPos(__LINE__, __FUNCTION__, __FILE__), mialgoAllocParam(0, MIALGO_MEM_CACHED, 0), s)

/**
* @brief allocate memory using parameters.
* @param[in] p          memory allocation parameters, @see MialgoAllocParam.
* @param[in] s          memory size in bytes.
* 
* @return
*        -<em>void *</em> memory pointer
*
* How to select memory allocation parameters, @see MialgoMemType
*
* example for allocate 1000 bytes for heap memory with uncanched and start address align to 128.
* @code
    MI_S32 size = 1000;
    MI_CHAR *buf = (MI_CHAR *)MialgoAllocateParam(mialgoAllocParam(MIALGO_MEM_HEAP, 0, 128), size);
    UtilsPrintMemInfo();
    MialgoDeallocate(buf);
 * @endcode
*/
#define MialgoAllocateParam(p, s)       MialgoAllocInternal(mialgoMemPos(__LINE__, __FUNCTION__, __FILE__), p, s)

/**
* @brief allocate memory from heap.
* @param[in] s          memory size in bytes.
* 
* @return
*        -<em>void *</em> memory pointer
*/
#define MialgoAllocateHeap(s)           MialgoAllocateParam(mialgoAllocParam(MIALGO_MEM_HEAP, MIALGO_MEM_CACHED, 0), s)

/**
* @brief allocate memory from ion.
* @param[in] s          memory size in bytes.
* 
* @return
*        -<em>void *</em> memory pointer
*/
#define MialgoAllocateIon(s)            MialgoAllocateParam(mialgoAllocParam(MIALGO_MEM_ION, MIALGO_MEM_CACHED, 0), s)

/**
* @brief allocate memory from opencl.
* @param[in] s          memory size in bytes.
* 
* @return
*        -<em>void *</em> memory pointer
*/
#define MialgoAllocateCl(s)             MialgoAllocateParam(mialgoAllocParam(MIALGO_MEM_CL, MIALGO_MEM_CACHED, 0), s)

/**
* @brief deallocate memory.
* @param[in] p          memory pointer.
*/
#define MialgoDeallocate(p)             MialgoDeallocInternal(mialgoMemPos(__LINE__, __FUNCTION__, __FILE__), p)

/**
* @brief Internal implementation use, do not recommend direct use.
*/
MI_VOID* MialgoAllocInternal(MialgoMemPos pos, MialgoAllocParam param, MI_U64 size);

/**
* @brief Internal implementation use, do not recommend direct use.
*/
MI_VOID MialgoDeallocInternal(MialgoMemPos pos, MI_VOID *ptr);

/**
* @brief Get the memory info.
* @param[in]  ptr               memory addr Allocated by @see MialgoAllocate.
* @param[out] mem_info          memory info @see MialgoMemInfo.
*
* @return
*        <em>MI_S32</em> @see mialgo_errorno.h.
*/
MI_S32 MialgoGetMemInfo(MI_VOID *ptr, MialgoMemInfo *mem_info);

/**
* @brief memory copy like memcpy.
*/
MI_S32 MialgoRawCopy(MI_VOID *dest, const MI_VOID *src, MI_S32 bytes);

/**
* @brief get the system memory info.
* @param[in] mem_info       @see MialgoMemSysInfo.
* 
* @return
*        -<em>MI_S32</em> @see mialgo_errorno.h
*
* check memory leak
*   use MialgoGetMemSysInfo can check memory leak, the peak memory of the algorithm can also be obtained.
*   when the algorithm process is complete, if cur_size is not equal to 0, there is a memory leak.
*   must use MialgoAllocate and MialgoDeallocate replace malloc and free.
*/
MI_S32 MialgoGetMemSysInfo(MialgoMemSysInfo *mem_info);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // MIALGO_MEM_H__