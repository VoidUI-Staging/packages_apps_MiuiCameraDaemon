/**
 * @file        dev_memoryPool.cpp
 * @brief
 * @details
 * @author
 * @date        2021.07.03
 * @version     0.1
 * @note
 * @warning
 * @par
 *
 */

//TODO LIST
//INO BUF的支持
//时间释放策略//有多余长时间不使用的BUF可以尝试释放
//自动生长问题
//Dump到文件功能
//提供对外读写接口是不是就可以检查内存块是不是安全了。

//DONE
//越界测试用例
//默认不申请，首次申请。
//有分段策略，对齐策略
//有强制释放功能。比如特定大小的BUF
//有申请日志输出
//退出有报告输出，包括申请位置，行号，名称，大小等内容。
//退出模块有强制释放功能。
//前标记+后标记 做数组越界检查，申请时检查，释放时检查

#include "device.h"
#include <memory.h>
#include "dev_memoryPool.h"

static U32 init_f = FALSE;
static S64 m_mutex_Id;

#define DEV_MEMORY_POOL_LIST_MAX        128
#define DEV_MEMORY_POOL_NODE_MAX        512

#define DEV_MEMORY_ALLOC_INFO_MAX_LEN   128

#define DEV_MEMORY_MARK_OFFSET          8

#define KB  (1024)
#define MB  (KB*1024)
#define GB  (MB*1024)

#define MS  (1000)
#define S   (1000*MS)

#define ALIGNSIZE32(n)  (n+(32-1))&(~(32-1)))   //4byte
#define ALIGNSIZE(n,align)  ((n+(align-1))&(~(align-1)))

static const U64 DEV_MEMORY_FRONT_MARK = 0xFA4FFFA5084CB5CF;
static const U64 DEV_MEMORY_REAR_MARK = 0xFB453A5FC78D3FDF;

typedef struct __DEV_MEMORYPOOL_NODE {
    void *addr;
    U32 userSzie;
    U32 used;
    S64 allocTimestamp;
    S64 unusedTimestamp;
    S64 waitTime;
    S64 allocTime;
    char fileInfo[DEV_MEMORY_ALLOC_INFO_MAX_LEN];
    char tagName[DEV_MEMORY_ALLOC_INFO_MAX_LEN];
} Dev_MemoryPool_node;

typedef struct __DEV_MEMORYPOOL_LIST {
    S32 allNum;
    S32 usedNum;
    U32 realSize;
    U32 unallocdNodeIndex[DEV_MEMORY_POOL_NODE_MAX];
    U32 AllocdButUnusedNodeIndex[DEV_MEMORY_POOL_NODE_MAX];
    Dev_MemoryPool_node *p_node[DEV_MEMORY_POOL_NODE_MAX];
} Dev_MemoryPool_list;

typedef struct __DEV_MEMORYPOOL_LIST_HEAD {
    S32 usedNum;
    U32 AllocdAndusedListIndex[DEV_MEMORY_POOL_LIST_MAX];
    U32 AllocdButUnusedListIndex[DEV_MEMORY_POOL_LIST_MAX];
    Dev_MemoryPool_list *p_list[DEV_MEMORY_POOL_LIST_MAX];
} Dev_MemoryPool_list_head;

typedef struct __DEV_MEMORYPOOL_MAP_NODE {
    U32 nodeIndex;
    U32 listIndex;
} Dev_MemoryPool_map_node;

typedef struct __DEV_MEMORYPOOL_MAP {
    S32 usedNum;
    Dev_MemoryPool_map_node node[DEV_MEMORY_POOL_LIST_MAX * DEV_MEMORY_POOL_LIST_MAX];
} Dev_MemoryPool_map;

typedef struct __DEV_MEMORYPOOL_FREE_THRESHOLD {
    U64 realSize;
    S32 countThreshold;
    U64 TimeThreshold;
} Dev_MemoryPool_free_threshold;

typedef struct __DEV_MEMORYPOOL_SIZE_ALIGN {
    U64 size;
    U64 align;
} Dev_MemoryPool_Size_Align;

static Dev_MemoryPool_list_head m_list = {0};
static Dev_MemoryPool_map m_addr_map = {0};
static Dev_MemoryPool_map m_realSize_map = {0};

static const Dev_MemoryPool_free_threshold m_free_threshold[] = {
        {1 * KB, 128, 60*S},   {128 * KB, 64, 40*S}, {256 * KB, 32, 30*S}, {384 * 16, 20, 20*S},
        {512 * KB, 10, 10*S},  {640 * KB, 9, 5*S},   {768 * KB, 8, 1*S},   {896 * KB, 7, 640*MS},
        {1 * MB, 6, 320*MS},   {10 * MB, 5, 160*MS}, {20 * MB, 4, 120*MS}, {30 * MB, 3, 80*MS},
        {40 * MB, 2, 40*MS},   {50 * MB, 1, 20*MS},  {60 * MB, 0, 20*MS},  {0XFFFFFFFFFFFFFFFF, 0, 0*MS}
};

static const Dev_MemoryPool_Size_Align m_size_align[] = {
        {512,       512     },   {1 * KB,   1 * KB  },  {4 * KB,     4 * KB },
        {8 * KB,    8 * KB  },   {32 * KB,  32 * KB },  {128 * KB,   64 * KB},
        {512 * KB,  128 * KB},   {1 * MB,   1 * MB  },  {8 * MB,     2 * MB },
        {16 * MB,   4 * MB  },   {32 * MB,  8 * MB  },  {40 * MB,    8 * MB },
};

static const char* DevMemoryPool_NoDirFileName(const char *pFilePath) {
    if (pFilePath == NULL) {
        return NULL;
    }
    const char *pFileName = strrchr(pFilePath, '/');
    if (NULL != pFileName) {
        pFileName += 1;
    } else {
        pFileName = pFilePath;
    }
    return pFileName;
}

S64 __DevMemoryPool_Report(void) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    U64 allocMemory = 0;
    U64 useMemory = 0;
    U32 realSize = 0;
    Dev_MemoryPool_list *p_MemoryPool_list = NULL;
    Dev_MemoryPool_node *p_MemoryPool_node = NULL;
    for (int i = 0; i < m_addr_map.usedNum; i++) {
        p_MemoryPool_list = m_list.p_list[m_addr_map.node[i].listIndex];
        DEV_IF_LOGE_RETURN_RET((p_MemoryPool_list == NULL), RET_ERR, "arg ERR");
        p_MemoryPool_node = p_MemoryPool_list->p_node[m_addr_map.node[i].nodeIndex];
        DEV_IF_LOGE_RETURN_RET((p_MemoryPool_node == NULL), RET_ERR, "arg ERR");
        useMemory += p_MemoryPool_node->userSzie;
    }
    for (int i = 0; i < DEV_MEMORY_POOL_LIST_MAX; i++) {
        if (m_list.p_list[i] != NULL) {
            allocMemory += (float)(((float)m_list.p_list[i]->realSize) * m_list.p_list[i]->allNum);
        }
    }
    DEV_LOGI("[--------------------------------------MEMORY POOL REPORT SATRT--------------------------------------]");
    DEV_LOGI("[-----------TOTAL USED MEM %8.4f:MB TOTAL ALLOCD MEM %8.4f:MB  RATE  %8.4f %%----------------]"
            , (float )((float )useMemory / MB)
            , (float )((float )allocMemory / MB)
            , (((float )useMemory / (float )allocMemory) * 100)
            );
    DEV_LOGI("[----------------------------------------------------------------------------------------------------]");
    for (int i = 0; i < DEV_MEMORY_POOL_LIST_MAX; i++) {
        if (m_list.p_list[i] != NULL) {
            DEV_LOGI("[--------------ALLOCD MEM %8.4f:MB BLOCK SIZE %8.4f:MB USED [%4d/%4d] MAX [%4d]  -----------]"
                    , (float )(((float )m_list.p_list[i]->realSize / MB) * m_list.p_list[i]->allNum)
                    , (float )((float )m_list.p_list[i]->realSize / MB)
                    , m_list.p_list[i]->usedNum
                    , m_list.p_list[i]->allNum
                    , DEV_MEMORY_POOL_NODE_MAX
                    );
        }
    }
    DEV_LOGI("[----------------------------------------------------------------------------------------------------]");
    DEV_LOGI("[----------------------------------------------------------------------------------------------------]");
    for (int i = m_realSize_map.usedNum - 1; i >= 0; i--) {
        p_MemoryPool_list = m_list.p_list[m_realSize_map.node[i].listIndex];
        DEV_IF_LOGE_RETURN_RET((p_MemoryPool_list == NULL), RET_ERR, "arg ERR");
        p_MemoryPool_node = p_MemoryPool_list->p_node[m_realSize_map.node[i].nodeIndex];
        DEV_IF_LOGE_RETURN_RET((p_MemoryPool_node == NULL), RET_ERR, "arg ERR");
        if (p_MemoryPool_node->used == TRUE) {
            if (realSize != p_MemoryPool_list->realSize) {
                realSize = p_MemoryPool_list->realSize;
                if (p_MemoryPool_list != NULL) {
                    DEV_LOGI("[-------------ALLOCD MEM %8.4f:MB BLOCK SIZE %8.4f:MB USED [%4d/%4d] MAX [%4d]  -----------]"
                            , (float )(((float )p_MemoryPool_list->realSize / MB) * p_MemoryPool_list->allNum)
                            , (float )((float )p_MemoryPool_list->realSize / MB)
                            , p_MemoryPool_list->usedNum
                            , p_MemoryPool_list->allNum
                            , DEV_MEMORY_POOL_NODE_MAX
                            );
                }
            }
            DEV_LOGI("[%d/%d][%s][%s][%p][%.4fMB=%d]TIMESTAMP[%ld]USED[%ldMS]WAIT[%ldMS]ALLOC[%ldMS]"
                    , i + 1
                    , m_realSize_map.usedNum
                    , p_MemoryPool_node->tagName
                    , p_MemoryPool_node->fileInfo
                    , (void*)((U8*)(p_MemoryPool_node->addr)+DEV_MEMORY_MARK_OFFSET)
                    , float((float )p_MemoryPool_node->userSzie / MB)
                    , p_MemoryPool_node->userSzie
                    , p_MemoryPool_node->allocTimestamp
                    , (Device->time->GetTimestampMs()-p_MemoryPool_node->allocTimestamp)
                    , p_MemoryPool_node->waitTime
                    , p_MemoryPool_node->allocTime
                    );
        }
    }
    DEV_LOGI("[--------------------------------------MEMORY POOL REPORT END  --------------------------------------]");
    return RET_OK;
}

S64 DevMemoryPool_Report(void) {
    S64 ret = RET_ERR;
    ret = Device->mutex->Lock(m_mutex_Id);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "ARG ERR");
    __DevMemoryPool_Report();
    ret = Device->mutex->Unlock(m_mutex_Id);
    return ret;
}

static U32 DevMemoryPool_GetRealSize(U64 userSzie) {
    U64 realSize = userSzie;
    S32 sizeAlignNum = sizeof(m_size_align) / sizeof(Dev_MemoryPool_Size_Align);
    S32 front = 0;
    S32 rear = sizeAlignNum - 1;
    S32 mid = 0;
    while (front <= rear) {
        mid = (front + rear) / 2;
        if (userSzie > m_size_align[mid].size) {
            front = mid + 1;
        } else {
            rear = mid - 1;
        }
    }
    S32 array_max = sizeof(m_size_align) / sizeof(Dev_MemoryPool_Size_Align);
    if (front >= array_max) {
        front = array_max - 1;
        DEV_LOGE("array_max ERR");
    }
    realSize = ALIGNSIZE(userSzie, m_size_align[front].align);
    if (realSize == userSzie) {
        realSize = realSize + (DEV_MEMORY_MARK_OFFSET * 2);
    }
    return realSize;
}

static S32 DevMemoryPool_GetList(U32 realSize, U32 *listIndex, Dev_MemoryPool_list **pp_memoryPool_list) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_IF_LOGE_RETURN_RET((listIndex == NULL), RET_ERR, "ARG ERR");
    DEV_IF_LOGE_RETURN_RET((pp_memoryPool_list == NULL), RET_ERR, "ARG ERR");
    S32 ret = RET_ERR;
    S32 usedNum = m_list.usedNum;
    S32 front = 0;
    S32 rear = usedNum - 1;
    S32 mid = 0;
    Dev_MemoryPool_list *p_MemoryPool_list = NULL;
    while ((front <= rear) && (usedNum > 0)) {
        mid = (front + rear) / 2;
        U32 listIndexTemp = m_list.AllocdAndusedListIndex[mid];
        p_MemoryPool_list = m_list.p_list[listIndexTemp];
        if (p_MemoryPool_list == NULL) {
            DEV_LOGE("FIND LIST ERR");
            break;
        }
        if (realSize < p_MemoryPool_list->realSize) {
            rear = mid - 1;
        } else if (realSize > p_MemoryPool_list->realSize) {
            front = mid + 1;
        } else if (realSize == p_MemoryPool_list->realSize) {
            *pp_memoryPool_list = p_MemoryPool_list;
            *listIndex = listIndexTemp;
            ret = RET_OK;
            break;    //find
        }
    }
    if (*pp_memoryPool_list == NULL) {
        U32 unusedListNum = DEV_MEMORY_POOL_LIST_MAX - m_list.usedNum;
        DEV_IF_LOGE_RETURN_RET((unusedListNum <= 0), RET_ERR, "MemoryPool cannot be allocated, try to increase DEV_MEMORY_POOL_LIST_MAX");
        U32 listIndexTemp = m_list.AllocdButUnusedListIndex[unusedListNum - 1];
        if (m_list.p_list[listIndexTemp] == NULL) {
            m_list.p_list[listIndexTemp] = (Dev_MemoryPool_list*)Dev_malloc(sizeof(Dev_MemoryPool_list));
            if (m_list.p_list[listIndexTemp] != NULL) {
                memset(m_list.p_list[listIndexTemp], 0, (sizeof(Dev_MemoryPool_list)));
                m_list.p_list[listIndexTemp]->realSize = realSize;

                p_MemoryPool_list = m_list.p_list[listIndexTemp];
                for (int i = 0; i < DEV_MEMORY_POOL_NODE_MAX; i++) {
                    p_MemoryPool_list->unallocdNodeIndex[i] = i; //Save nodes that have not been used
                }

                *pp_memoryPool_list = m_list.p_list[listIndexTemp];
                *listIndex = listIndexTemp;
                /*****************************************************/
                usedNum = m_list.usedNum;
                front = 0;
                rear = usedNum - 1;
                p_MemoryPool_list = NULL;
                while (front <= rear) {
                    mid = (front + rear) / 2;
                    p_MemoryPool_list = m_list.p_list[m_list.AllocdAndusedListIndex[mid]];
                    if (p_MemoryPool_list == NULL) {
                        DEV_LOGE("FIND LIST ERR");
                        break;
                    }
                    if (realSize > p_MemoryPool_list->realSize) {
                        front = mid + 1;
                    } else {
                        rear = mid - 1;
                    }
                }
                for (int j = usedNum - 1; j >= front; j--) {
                    m_list.AllocdAndusedListIndex[j + 1] = m_list.AllocdAndusedListIndex[j];
                }
                m_list.AllocdAndusedListIndex[front] = listIndexTemp;
                /*****************************************************/
                ret = RET_OK;
                m_list.usedNum++;
            }
        }
    }
    DEV_IF_LOGE_RETURN_RET((*pp_memoryPool_list==NULL), RET_ERR, "MemoryPool cannot be allocated, try to increase DEV_MEMORY_POOL_LIST_MAX");
    return ret;
}

static S32 DevMemoryPool_RemoveList(Dev_MemoryPool_list *p_MemoryPool_list, U32 listIndex) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_IF_LOGE_RETURN_RET((p_MemoryPool_list == NULL), RET_ERR, "ARG ERR");
    DEV_IF_LOGE_RETURN_RET((m_list.p_list[listIndex] != p_MemoryPool_list), RET_ERR, "ARG ERR");
    if ((p_MemoryPool_list->allNum == 0) && (p_MemoryPool_list->usedNum == 0)) {
        /*****************************************************/
        U32 realSize = p_MemoryPool_list->realSize;
        S32 usedListNum = m_list.usedNum;
        S32 front = 0;
        S32 rear = usedListNum - 1;
        S32 mid = 0;
        Dev_MemoryPool_list *p_MemoryPool_list_temp = NULL;
        while ((front <= rear) && (usedListNum > 0)) {
            mid = (front + rear) / 2;
            U32 listIndexUused = m_list.AllocdAndusedListIndex[mid];
            p_MemoryPool_list_temp = m_list.p_list[listIndexUused];
            if (p_MemoryPool_list_temp == NULL) {
                DEV_LOGE("FIND LIST ERR");
                break;
            }
            if (realSize < p_MemoryPool_list_temp->realSize) {
                rear = mid - 1;
            } else if (realSize > p_MemoryPool_list_temp->realSize) {
                front = mid + 1;
            } else if (realSize == p_MemoryPool_list_temp->realSize) {
                break;    //find
            }
        }
        for (int i = front; i < usedListNum - 1; i++) {
            m_list.AllocdAndusedListIndex[i] = m_list.AllocdAndusedListIndex[i + 1];
        }
        /*****************************************************/
        U32 unusedListNum = DEV_MEMORY_POOL_LIST_MAX - m_list.usedNum;
        Dev_free((void*)p_MemoryPool_list);
        m_list.p_list[listIndex] = NULL;
        m_list.AllocdButUnusedListIndex[unusedListNum] = listIndex; //Save list index that have not been used
        m_list.usedNum--;
        if (m_list.usedNum < 0) {
            m_list.usedNum = 0;
        }
    }
    return RET_OK;
}

static S32 DevMemoryPool_Check(Dev_MemoryPool_node *p_MemoryPool_node) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_IF_LOGE_RETURN_RET((p_MemoryPool_node == NULL), RET_ERR, "arg ERR");
    S32 ret = RET_OK;
    if (p_MemoryPool_node->addr != NULL) {
        U8 *dataU8 = (U8*)p_MemoryPool_node->addr;
        U64 *dataU64 = (U64*)dataU8;
        if ((dataU64[0] != DEV_MEMORY_FRONT_MARK)) {
            DEV_LOGE("ERROR! Memory check overflow!!!!!! DEV_MEMORY_FRONT_MARK");
            DEV_LOGE("ERROR! Memory check overflow!!!!!! DEV_MEMORY_FRONT_MARK");
            DEV_LOGE("ERROR! Memory check overflow!!!!!! DEV_MEMORY_FRONT_MARK");
            ret |= RET_ERR;
        }
        dataU64 = (U64*)&dataU8[p_MemoryPool_node->userSzie + DEV_MEMORY_MARK_OFFSET];
        if ((dataU64[0] != DEV_MEMORY_REAR_MARK)) {
            DEV_LOGE("ERROR! Memory check overflow!!!!!! DEV_MEMORY_REAR_MARK");
            DEV_LOGE("ERROR! Memory check overflow!!!!!! DEV_MEMORY_REAR_MARK");
            DEV_LOGE("ERROR! Memory check overflow!!!!!! DEV_MEMORY_REAR_MARK");
            ret |= RET_ERR;
        }
    }
    if (ret != RET_OK) {
        DEV_LOGE("[-----------------------------------------------]");
        DEV_LOGE("ERROR! INFO NAME:[%s]", p_MemoryPool_node->tagName);
        DEV_LOGE("ERROR! INFO FILE:[%s]", p_MemoryPool_node->fileInfo);
        DEV_LOGE("ERROR! INFO ADDR:[%p]", p_MemoryPool_node->addr);
        DEV_LOGE("ERROR! INFO SIZE:[%d]", p_MemoryPool_node->userSzie);
        DEV_LOGE("ERROR! INFO TIMESTAMP :[%ld]", p_MemoryPool_node->allocTimestamp);
        DEV_LOGE("ERROR! INFO USED  TIME:[%ldMS]", Device->time->GetTimestampMs()-p_MemoryPool_node->allocTimestamp);
        DEV_LOGE("ERROR! INFO WAIT  TIME:[%ldMS]", p_MemoryPool_node->waitTime);
        DEV_LOGE("ERROR! INFO ALLOC TIME:[%ldMS]", p_MemoryPool_node->allocTime);
        DEV_LOGE("[-----------------------------------------------]");
        DEV_LOGE("ERROR! Serious errors need to be solved perfectly!!!!!!");
        __DevMemoryPool_Report();
//        init_f = FALSE;    //Serious errors need to be solved perfectly
    }
    return ret;
}

static S32 DevMemoryPool_Mark(Dev_MemoryPool_node *p_MemoryPool_node) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_IF_LOGE_RETURN_RET((p_MemoryPool_node == NULL), RET_ERR, "arg ERR");
    if (p_MemoryPool_node->addr != NULL) {
        U8 *dataU8 = (U8*)p_MemoryPool_node->addr;
        U64 *dataU64 = (U64*)dataU8;
        dataU64[0] = DEV_MEMORY_FRONT_MARK;
        dataU64 = (U64*)&dataU8[p_MemoryPool_node->userSzie + DEV_MEMORY_MARK_OFFSET];
        dataU64[0] = DEV_MEMORY_REAR_MARK;
    }
    return RET_OK;
}

static S32 DevMemoryPool_AddRealSizeMap(void *addr, U32 realSize, U32 listIndex, U32 nodeIndex) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_IF_LOGE_RETURN_RET((addr == NULL), RET_ERR, "arg ERR");
    DEV_IF_LOGE_RETURN_RET((m_realSize_map.usedNum >= DEV_MEMORY_POOL_LIST_MAX * DEV_MEMORY_POOL_LIST_MAX), RET_ERR, "arg ERR");
    Dev_MemoryPool_map_node insertNode = {.nodeIndex = nodeIndex, .listIndex = listIndex};
    Dev_MemoryPool_list *p_MemoryPool_list = NULL;
    Dev_MemoryPool_node *p_MemoryPool_node = NULL;
    S32 usedNum = m_realSize_map.usedNum;
    S32 front = 0;
    S32 frontTemp = 0;
    S32 rear = usedNum - 1;
    //Find the same realSize front and rear
    //find front
    while (front <= rear) {
        p_MemoryPool_list = m_list.p_list[m_realSize_map.node[(front + rear) / 2].listIndex];
        DEV_IF_LOGE_RETURN_RET((p_MemoryPool_list == NULL), RET_ERR, "arg ERR");
        if (realSize > p_MemoryPool_list->realSize) {
            front = (front + rear) / 2 + 1;
        } else {
            rear = (front + rear) / 2 - 1;
        }
    }
    frontTemp = front;
    //find rear
    rear = usedNum - 1;
    while (front <= rear) {
        p_MemoryPool_list = m_list.p_list[m_realSize_map.node[(front + rear) / 2].listIndex];
        DEV_IF_LOGE_RETURN_RET((p_MemoryPool_list == NULL), RET_ERR, "arg ERR");
        if (realSize < p_MemoryPool_list->realSize) {
            rear = (front + rear) / 2 - 1;
        } else {
            front = (front + rear) / 2 + 1;
        }
    }
    front = frontTemp;
    while (front <= rear) {
        p_MemoryPool_list = m_list.p_list[m_realSize_map.node[(front + rear) / 2].listIndex];
        DEV_IF_LOGE_RETURN_RET((p_MemoryPool_list == NULL), RET_ERR, "arg ERR");
        p_MemoryPool_node = p_MemoryPool_list->p_node[m_realSize_map.node[(front + rear) / 2].nodeIndex];
        DEV_IF_LOGE_RETURN_RET((p_MemoryPool_node == NULL), RET_ERR, "arg ERR");
        if (addr > p_MemoryPool_node->addr) {
            front = (front + rear) / 2 + 1;
        } else {
            rear = (front + rear) / 2 - 1;
        }
    }
    for (int j = usedNum - 1; j >= front; j--) {
        m_realSize_map.node[j + 1] = m_realSize_map.node[j];
    }
    m_realSize_map.node[front] = insertNode;
    m_realSize_map.usedNum++;
    return RET_OK;
}

static S32 DevMemoryPool_RemoveRealSizeMap(void *addr, U32 realSize) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_IF_LOGE_RETURN_RET((addr == NULL), RET_ERR, "arg ERR");
    S32 ret = RET_ERR;
    S32 usedNum = m_realSize_map.usedNum;
    S32 front = 0;
    S32 frontTemp = 0;
    S32 rear = usedNum - 1;
    S32 mid = 0;
    Dev_MemoryPool_list *p_MemoryPool_list = NULL;
    Dev_MemoryPool_node *p_MemoryPool_node = NULL;
    //Find the same realSize front and rear
    //find front
    while (front <= rear) {
        p_MemoryPool_list = m_list.p_list[m_realSize_map.node[(front + rear) / 2].listIndex];
        DEV_IF_LOGE_RETURN_RET((p_MemoryPool_list == NULL), RET_ERR, "arg ERR");
        if (realSize > p_MemoryPool_list->realSize) {
            front = (front + rear) / 2 + 1;
        } else {
            rear = (front + rear) / 2 - 1;
        }
    }
    frontTemp = front;
    //find rear
    rear = usedNum - 1;
    while (front <= rear) {
        p_MemoryPool_list = m_list.p_list[m_realSize_map.node[(front + rear) / 2].listIndex];
        DEV_IF_LOGE_RETURN_RET((p_MemoryPool_list == NULL), RET_ERR, "arg ERR");
        if (realSize < p_MemoryPool_list->realSize) {
            rear = (front + rear) / 2 - 1;
        } else {
            front = (front + rear) / 2 + 1;
        }
    }
    front = frontTemp;
    while (front <= rear) {
        mid = (front + rear) / 2;
        p_MemoryPool_list = m_list.p_list[m_realSize_map.node[mid].listIndex];
        DEV_IF_LOGE_RETURN_RET((p_MemoryPool_list == NULL), RET_ERR, "arg ERR");
        p_MemoryPool_node = p_MemoryPool_list->p_node[m_realSize_map.node[mid].nodeIndex];
        DEV_IF_LOGE_RETURN_RET((p_MemoryPool_node == NULL), RET_ERR, "arg ERR");
        if (addr < p_MemoryPool_node->addr) {
            rear = mid - 1;
        } else if (addr > p_MemoryPool_node->addr) {
            front = mid + 1;
        } else if (addr == p_MemoryPool_node->addr) {
            ret = RET_OK;
            break;    //find
        }
    }
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "arg ERR");
    for (int j = mid; j < usedNum - 1; j++) {
        m_realSize_map.node[j] = m_realSize_map.node[j + 1];
    }
    m_realSize_map.usedNum--;
    if (m_realSize_map.usedNum < 0) {
        m_realSize_map.usedNum = 0;
    }
    return ret;
}

static S32 DevMemoryPool_AddAddrMap(void *addr, U32 realSize, U32 listIndex, U32 nodeIndex) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_IF_LOGE_RETURN_RET((addr == NULL), RET_ERR, "arg ERR");
    DEV_IF_LOGE_RETURN_RET((m_addr_map.usedNum >= DEV_MEMORY_POOL_LIST_MAX * DEV_MEMORY_POOL_LIST_MAX), RET_ERR, "arg ERR");
    Dev_MemoryPool_map_node insertNode = {.nodeIndex = nodeIndex, .listIndex = listIndex};
    S32 usedNum = m_addr_map.usedNum;
    S32 front = 0;
    S32 rear = usedNum - 1;
    Dev_MemoryPool_list *p_MemoryPool_list = NULL;
    Dev_MemoryPool_node *p_MemoryPool_node = NULL;
    while (front <= rear) {
        p_MemoryPool_list = m_list.p_list[m_addr_map.node[(front + rear) / 2].listIndex];
        DEV_IF_LOGE_RETURN_RET((p_MemoryPool_list == NULL), RET_ERR, "arg ERR");
        p_MemoryPool_node = p_MemoryPool_list->p_node[m_addr_map.node[(front + rear) / 2].nodeIndex];
        DEV_IF_LOGE_RETURN_RET((p_MemoryPool_node == NULL), RET_ERR, "arg ERR");
        if (addr > p_MemoryPool_node->addr) {
            front = (front + rear) / 2 + 1;
        } else {
            rear = (front + rear) / 2 - 1;
        }
    }
    for (int j = usedNum - 1; j >= front; j--) {
        m_addr_map.node[j + 1] = m_addr_map.node[j];
    }
    m_addr_map.node[front] = insertNode;
    m_addr_map.usedNum++;
    return RET_OK;
}

static S32 DevMemoryPool_RemoveAddrMap(void *addr, U32 *listIndex, U32 *nodeIndex) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_IF_LOGE_RETURN_RET((addr == NULL), RET_ERR, "arg ERR");
    DEV_IF_LOGE_RETURN_RET((listIndex == NULL), RET_ERR, "arg ERR");
    DEV_IF_LOGE_RETURN_RET((nodeIndex == NULL), RET_ERR, "arg ERR");
    DEV_IF_LOGE_RETURN_RET((m_addr_map.usedNum >= DEV_MEMORY_POOL_LIST_MAX * DEV_MEMORY_POOL_LIST_MAX), RET_ERR, "arg ERR");
    S32 usedNum = m_addr_map.usedNum;
    S32 front = 0;
    S32 rear = usedNum - 1;
    S32 mid = 0;
    S32 ret = RET_ERR;
    Dev_MemoryPool_list *p_MemoryPool_list = NULL;
    Dev_MemoryPool_node *p_MemoryPool_node = NULL;
    while (front <= rear) {
        mid = (front + rear) / 2;
        p_MemoryPool_list = m_list.p_list[m_addr_map.node[mid].listIndex];
        DEV_IF_LOGE_RETURN_RET((p_MemoryPool_list == NULL), RET_ERR, "arg ERR");
        p_MemoryPool_node = p_MemoryPool_list->p_node[m_addr_map.node[mid].nodeIndex];
        DEV_IF_LOGE_RETURN_RET((p_MemoryPool_node == NULL), RET_ERR, "arg ERR");
        if (addr < p_MemoryPool_node->addr) {
            rear = mid - 1;
        } else if (addr > p_MemoryPool_node->addr) {
            front = mid + 1;
        } else if (addr == p_MemoryPool_node->addr) {
            *listIndex = m_addr_map.node[mid].listIndex;
            *nodeIndex = m_addr_map.node[mid].nodeIndex;
            ret = RET_OK;
            break;    //find
        }
    }
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "Find addr map err");
    for (int j = mid; j < usedNum - 1; j++) {
        m_addr_map.node[j] = m_addr_map.node[j + 1];
    }
    m_addr_map.usedNum--;
    if (m_addr_map.usedNum < 0) {
        m_addr_map.usedNum = 0;
    }
    return ret;
}

static U32 DevMemoryPool_Clean(void) {
    Dev_MemoryPool_list *p_MemoryPool_list = NULL;
    Dev_MemoryPool_node *p_MemoryPool_node = NULL;
    for (int i = 0; i < DEV_MEMORY_POOL_LIST_MAX; i++) {
        p_MemoryPool_list = m_list.p_list[i];
        if (p_MemoryPool_list != NULL) {
            for (int j = 0; j < DEV_MEMORY_POOL_NODE_MAX; j++) {
                p_MemoryPool_node = p_MemoryPool_list->p_node[j];
                if (p_MemoryPool_node != NULL) {
                    if (p_MemoryPool_node->addr != NULL) {
                        free(p_MemoryPool_node->addr); //Other system Malloc interface can be used
                        p_MemoryPool_node->addr = NULL;
                    }
                    Dev_free((void*)p_MemoryPool_node);
                    p_MemoryPool_list->p_node[j] = NULL;
                    p_MemoryPool_list->allNum--;
                    p_MemoryPool_list->usedNum--;
                    if (p_MemoryPool_list->allNum <= 0) {
                        p_MemoryPool_list->allNum = 0;
                    }
                    if (p_MemoryPool_list->usedNum <= 0) {
                        p_MemoryPool_list->usedNum = 0;
                    }
                }
            }
            if ((p_MemoryPool_list->allNum == 0) && (p_MemoryPool_list->usedNum == 0)) {
                Dev_free((void*)p_MemoryPool_list);
                m_list.p_list[i] = NULL;
            }
        }
    }
    return RET_OK;
}

static S32 DevMemoryPool_GetNode(Dev_MemoryPool_list *p_MemoryPool_list, U32 *nodeIndex, Dev_MemoryPool_node **pp_MemoryPool_node) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_IF_LOGE_RETURN_RET((p_MemoryPool_list == NULL), RET_ERR, "arg ERR");
    Dev_MemoryPool_node *p_MemoryPool_node = NULL;
    S32 unusedAllocdNum = p_MemoryPool_list->allNum - p_MemoryPool_list->usedNum;
    if (unusedAllocdNum > 0) {
        U32 nusedIndex = p_MemoryPool_list->AllocdButUnusedNodeIndex[unusedAllocdNum - 1];
        if (p_MemoryPool_list->p_node[nusedIndex]->used != TRUE) {
            *pp_MemoryPool_node = p_MemoryPool_list->p_node[nusedIndex];
            *nodeIndex = nusedIndex;
            p_MemoryPool_list->usedNum++;
            p_MemoryPool_node = *pp_MemoryPool_node;
            S32 ret = DevMemoryPool_Check(p_MemoryPool_node);
            DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "MEM CHECK ERR");
        }
    }

    if (*pp_MemoryPool_node == NULL) {
        S32 unusedNum = DEV_MEMORY_POOL_NODE_MAX - p_MemoryPool_list->allNum;
        S32 unusedNodeIndex = p_MemoryPool_list->unallocdNodeIndex[unusedNum - 1];
        if (p_MemoryPool_list->p_node[unusedNodeIndex] == NULL) {
            p_MemoryPool_node = (Dev_MemoryPool_node*)Dev_malloc(sizeof(Dev_MemoryPool_node));
            p_MemoryPool_list->p_node[unusedNodeIndex] = p_MemoryPool_node;
            if (p_MemoryPool_list->p_node[unusedNodeIndex] != NULL) {
                memset(p_MemoryPool_list->p_node[unusedNodeIndex], 0, (sizeof(Dev_MemoryPool_node)));
                *pp_MemoryPool_node = p_MemoryPool_node;
                *nodeIndex = unusedNodeIndex;
                p_MemoryPool_list->usedNum++;
                p_MemoryPool_list->allNum++;
                if (p_MemoryPool_node->addr == NULL) {
                    p_MemoryPool_node->addr = malloc(p_MemoryPool_list->realSize);    //Other system Malloc interface can be used
                }
            }
        }
    }
    DEV_IF_LOGE_RETURN_RET((*pp_MemoryPool_node==NULL), RET_ERR, "MemoryPool cannot be allocated, try to increase DEV_MEMORY_POOL_NODE_MAX");
    return RET_OK;
}

static U32 DevMemoryPool_GetRealFree(Dev_MemoryPool_list *p_MemoryPool_list) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_IF_LOGE_RETURN_RET((p_MemoryPool_list == NULL), RET_ERR, "arg ERR");
    U32 realSize = p_MemoryPool_list->realSize;
    S32 unusedAllocdNum = p_MemoryPool_list->allNum - p_MemoryPool_list->usedNum;
    S32 thresholdNum = sizeof(m_free_threshold) / sizeof(Dev_MemoryPool_free_threshold);
    for (int i = 0; i < thresholdNum; i++) {
        if (m_free_threshold[i].realSize >= realSize) {
            if (unusedAllocdNum >= m_free_threshold[i].countThreshold) {
                return RET_OK;
            }
            break;
        }
    }
    return RET_ERR;
}

static S32 DevMemoryPool_RemoveNode(Dev_MemoryPool_list *p_MemoryPool_list, Dev_MemoryPool_node *p_MemoryPool_node, U32 nodeIndex) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_IF_LOGE_RETURN_RET((p_MemoryPool_list == NULL), RET_ERR, "arg ERR");
    DEV_IF_LOGE_RETURN_RET((p_MemoryPool_list->p_node[nodeIndex] != p_MemoryPool_node), RET_ERR, "arg ERR");
    S32 unusedAllocdNum = p_MemoryPool_list->allNum - p_MemoryPool_list->usedNum;
    S32 unusedNum = DEV_MEMORY_POOL_NODE_MAX - p_MemoryPool_list->allNum;
    DEV_IF_LOGE_RETURN_RET((unusedNum == DEV_MEMORY_POOL_NODE_MAX), RET_ERR, "No more remove");
    if (DevMemoryPool_GetRealFree(p_MemoryPool_list) == RET_OK) {
        p_MemoryPool_node->userSzie = 0;
        if (p_MemoryPool_node->addr != NULL) {
            free(p_MemoryPool_node->addr); //Other system Malloc interface can be used
            p_MemoryPool_node->addr = NULL;
        }
        Dev_free((void*)p_MemoryPool_node);
        p_MemoryPool_list->p_node[nodeIndex] = NULL;
        p_MemoryPool_list->allNum--;
        if (p_MemoryPool_list->allNum <= 0) {
            p_MemoryPool_list->allNum = 0;
        }
        p_MemoryPool_list->unallocdNodeIndex[unusedNum] = nodeIndex; //Save nodes that have not been used
    } else {
        p_MemoryPool_node->unusedTimestamp = Device->time->GetTimestampMs();
        p_MemoryPool_list->AllocdButUnusedNodeIndex[unusedAllocdNum] = nodeIndex; //Save nodes that have not been freed
    }
    p_MemoryPool_list->usedNum--;
    if (p_MemoryPool_list->usedNum <= 0) {
        p_MemoryPool_list->usedNum = 0;
    }
    return RET_OK;
}

void* DevMemoryPool_Malloc(U32 size, MARK_TAG tagName) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), NULL, "NOT INIT");
    S64 ret = RET_ERR;
    S64 startTimestamp = Device->time->GetTimestampMs();
    ret = Device->mutex->Lock(m_mutex_Id);
    S64 allocTime = 0;
    const char *noDirFileName = DevMemoryPool_NoDirFileName(__fileName);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), NULL, "mutex lock err");
    void *addr = NULL;
    Dev_MemoryPool_list *p_MemoryPool_list = NULL;
    Dev_MemoryPool_node *p_MemoryPool_node = NULL;
    U32 listIndex = 0;
    U32 nodeIndex = 0;
    U32 realSize = DevMemoryPool_GetRealSize(size);
    ret = DevMemoryPool_GetList(realSize, &listIndex, &p_MemoryPool_list);
    DEV_IF_LOGE_GOTO((p_MemoryPool_list == NULL), exit, "Find memory pool list ERR [%s][%d][%d]", noDirFileName, __fileLine, size);
    ret = DevMemoryPool_GetNode(p_MemoryPool_list, &nodeIndex, &p_MemoryPool_node);
    DEV_IF_LOGE_GOTO((p_MemoryPool_node == NULL), exit, "Find memory pool node ERR [%s][%d][%d]", noDirFileName, __fileLine, size);
    p_MemoryPool_node->allocTimestamp = startTimestamp;
    p_MemoryPool_node->userSzie = size;
    Dev_sprintf(p_MemoryPool_node->fileInfo, "[%s]:[%d]", noDirFileName, __fileLine);
    Dev_sprintf(p_MemoryPool_node->tagName, "[%s]", tagName);
    p_MemoryPool_node->waitTime = Device->time->GetTimestampMs() - startTimestamp;
    allocTime = Device->time->GetTimestampMs();
    p_MemoryPool_node->allocTime = Device->time->GetTimestampMs() - allocTime;
    DEV_IF_LOGE_GOTO((p_MemoryPool_node->addr == NULL), exit, "malloc memory ERR [%s][%d][%d]", noDirFileName, __fileLine, size);
    ret = DevMemoryPool_Mark(p_MemoryPool_node);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "Find memory pool node ERR [%s][%d][%d]", noDirFileName, __fileLine, size);
    ret = DevMemoryPool_AddAddrMap(p_MemoryPool_node->addr, realSize, listIndex, nodeIndex);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "Find memory pool node ERR [%s][%d][%d]", noDirFileName, __fileLine, size);
    ret = DevMemoryPool_AddRealSizeMap(p_MemoryPool_node->addr, realSize, listIndex, nodeIndex);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "Find memory pool node ERR [%s][%d][%d]", noDirFileName, __fileLine, size);
    p_MemoryPool_node->used = TRUE;
    p_MemoryPool_list->realSize = realSize;
    addr = (void*)((U8*)p_MemoryPool_node->addr + DEV_MEMORY_MARK_OFFSET);
    exit: ret |= Device->mutex->Unlock(m_mutex_Id);
    return addr;
}

S64 DevMemoryPool_Free(void *addr) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    if (addr == NULL) {
        return RET_OK;
    }
    S64 ret = RET_ERR;
    ret = Device->mutex->Lock(m_mutex_Id);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), ret, "mutex lock err");
    Dev_MemoryPool_list *p_MemoryPool_list = NULL;
    Dev_MemoryPool_node *p_MemoryPool_node = NULL;
    U32 listIndex = 0;
    U32 nodeIndex = 0;
    void *realAddr = (void*)(((U8*)addr) - DEV_MEMORY_MARK_OFFSET);
    ret = DevMemoryPool_RemoveAddrMap(realAddr, &listIndex, &nodeIndex);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "Find memory pool node ERR double free");
    p_MemoryPool_list = m_list.p_list[listIndex];
    DEV_IF_LOGE_RETURN_RET((p_MemoryPool_list == NULL), RET_ERR, "arg ERR");
    p_MemoryPool_node = p_MemoryPool_list->p_node[nodeIndex];
    DEV_IF_LOGE_RETURN_RET((p_MemoryPool_node == NULL), RET_ERR, "arg ERR");
    ret = DevMemoryPool_RemoveRealSizeMap(realAddr, p_MemoryPool_list->realSize);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "Find memory pool node ERR double free");
    ret = DevMemoryPool_Check(p_MemoryPool_node);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "Find memory pool node ERR");
    p_MemoryPool_node->used = FALSE;
    ret = DevMemoryPool_RemoveNode(p_MemoryPool_list, p_MemoryPool_node, nodeIndex);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "Find memory pool node ERR");
    ret = DevMemoryPool_RemoveList(p_MemoryPool_list, listIndex);
    DEV_IF_LOGE_GOTO((ret != RET_OK), exit, "Find memory pool node ERR");
    exit: ret |= Device->mutex->Unlock(m_mutex_Id);
    return ret;
}

S64 DevMemoryPool_Init(void) {
    if (init_f == TRUE) {
        return RET_OK;
    }
    S64 ret = RET_OK;
    ret |= Device->mutex->Create(&m_mutex_Id, MARK("m_mutex_Id"));
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "Create mutex ID err");
    memset((void*)&m_list, 0, sizeof(m_list));
    memset((void*)&m_addr_map, 0, sizeof(m_addr_map));
    memset((void*)&m_realSize_map, 0, sizeof(m_realSize_map));
    for (int i = 0; i < DEV_MEMORY_POOL_LIST_MAX; i++) {
        m_list.AllocdButUnusedListIndex[i] = i; //Save list index that have not been used
    }
    init_f = TRUE;
    return ret;
}

S64 DevMemoryPool_Deinit(void) {
    if (init_f != TRUE) {
        return RET_OK;
    }
    S64 ret = RET_OK;
    if (m_realSize_map.usedNum > 0) {
        ret |= DevMemoryPool_Report();
    }
    ret |= DevMemoryPool_Clean();
    init_f = FALSE;
    memset((void*)&m_list, 0, sizeof(m_list));
    memset((void*)&m_addr_map, 0, sizeof(m_addr_map));
    memset((void*)&m_realSize_map, 0, sizeof(m_realSize_map));
    ret |= Device->mutex->Destroy(&m_mutex_Id);
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "MemoryPool Deinit err[%ld]", ret);
    return ret;
}

const Dev_MemoryPool m_dev_memoryPool = {
        .Init           =DevMemoryPool_Init,
        .Deinit         =DevMemoryPool_Deinit,
        .Malloc         =DevMemoryPool_Malloc,
        .Free           =DevMemoryPool_Free,
        .Report         =DevMemoryPool_Report,
};
