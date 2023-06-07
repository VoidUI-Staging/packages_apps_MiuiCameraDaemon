/**
 * @file        dev_libPool.cpp
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

#include <dlfcn.h>
#include <dirent.h>
#include "device.h"
#include "dev_libPool.h"

static U32 init_f = FALSE;
static S64 m_mutex_Id;
static S64 m_mutex_OpneId;

#define DEV_LIBPOOL_DIR_OPEN_MAX          (64)

#define LIB_EXTENSION_NAME                "so"
#define LIB_ENTRY_INTERFACE               "Entry"

typedef struct __DevLibPool_Dir_List {
    S32 usedNum;
    char __fileName[FILE_NAME_LEN_MAX];
    U32 __fileLine;
    char dir[FILE_NAME_LEN_MAX];
    void *fd[DEV_LIBPOOL_LIB_OPEN_MAX];
    DEV_LIBPOOL_LIST list;
} DEV_LIBPOOL_DIR_LIST;

static DEV_LIBPOOL_DIR_LIST m_libPoolList[DEV_LIBPOOL_DIR_OPEN_MAX];
static S32 m_libPoolListDir_num = 0;

typedef struct __DevLibPool_Open_List {
    S32 usedNum;
    char __fileName[FILE_NAME_LEN_MAX];
    U32 __fileLine;
    char dir[FILE_NAME_LEN_MAX];
    char name[FILE_NAME_LEN_MAX];
    void *fd;
    DEV_LIBPOOL_ENTRY entry;
} DEV_LIBPOOL_OPEN_LIST;

static DEV_LIBPOOL_OPEN_LIST m_libPoolOpenList[DEV_LIBPOOL_DIR_OPEN_MAX];
static S32 m_libPoolListOpen_num = 0;

enum FileNameToken {
    Com,       ///< com
    Vendor,    ///< vendor name
    Module,    ///< module name
    Target,    ///< Target name
    Extension, ///< File Extension
    Max        ///< Max tokens for file name
};

static const char* __DevLibPool_NoDirFileName(const char *pFilePath) {
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

S32 DevLibPool_GetFilesFromPath(const char *pFileSearchPath, char *pFileNames, const char *pVendorName, const char *pModuleName,
        const char *pTargetName, const char *pExtension) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    S64 ret = RET_OK;
    U16 fileCount = 0;
    DIR *pDirectory = NULL;
    dirent *pDirent = NULL;
    const char *pToken = NULL;
    char *pContext = NULL;
    UINT tokenCount = 0;
    bool isValid = TRUE;
    const char *pCom = static_cast<const char*>("com");
    UINT maxFileNameToken = static_cast<UINT>(FileNameToken::Max);
    const char *pTokens[maxFileNameToken];
    char fileName[FILE_NAME_LEN_MAX+1];
    pDirectory = opendir(pFileSearchPath);
    if (NULL != pDirectory) {
        while (NULL != (pDirent = readdir(pDirectory))) {
            Dev_strncpy(fileName, pDirent->d_name, FILE_NAME_LEN_MAX);
            tokenCount = 0;
            pToken = Dev_strtok_r(fileName, ".", &pContext);
            isValid = TRUE;
            for (U8 i = 0; i < maxFileNameToken; i++) {
                pTokens[i] = NULL;
            }
            while ((NULL != pToken) && (tokenCount < maxFileNameToken)) {
                pTokens[tokenCount++] = pToken;
                pToken = Dev_strtok_r(NULL, ".", &pContext);
            }

            if ((NULL == pToken) && (maxFileNameToken - 1 <= tokenCount) && (maxFileNameToken >= tokenCount)) {
                U8 index = 0;
                for (U8 i = 0; (i < maxFileNameToken) && (isValid == TRUE); i++) {
                    switch (static_cast<FileNameToken>(i)) {
                    case FileNameToken::Com:
                        pToken = pCom;
                        index = i;
                        break;
                    case FileNameToken::Vendor:
                        pToken = pVendorName;
                        index = i;
                        break;
                    case FileNameToken::Module:
                        pToken = pModuleName;
                        index = i;
                        break;
                    case FileNameToken::Target:
                        if ((maxFileNameToken == tokenCount + 1) && (0 == Dev_strcmp(pTargetName, "*"))) {
                            continue;
                        } else {
                            pToken = pTargetName;
                            index = i;
                        }
                        break;
                    case FileNameToken::Extension:
                        if (maxFileNameToken == tokenCount + 1) {
                            pToken = pExtension;
                            index = i - 1;
                        } else {
                            pToken = pExtension;
                            index = i;
                        }
                        break;
                    default:
                        break;
                    }
                    if (0 != Dev_strcmp(pToken, "*")) {
                        if (0 == Dev_strcmp(pToken, pTokens[index])) {
                            isValid = TRUE;
                        } else {
                            isValid = FALSE;
                        }
                    }
                }
                if (TRUE == isValid) {
                    ret = Dev_snprintf(pFileNames + (fileCount * FILE_NAME_LEN_MAX), FILE_NAME_LEN_MAX, "%s%s%s", pFileSearchPath, "/",
                            pDirent->d_name);
                    DEV_IF_LOGE((ret < 0), "File name too long");
                    fileCount++;
                }
            }
        }
        closedir(pDirectory);
    }
    return fileCount;
}

static S32 DevLibPool_DirLoad(const char *dir, void **pFd, DEV_LIBPOOL_LIST *p_libPoolList) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_IF_LOGE_RETURN_RET((p_libPoolList == NULL)||(dir==NULL)||(pFd==NULL), RET_ERR, "LibPool arg err");
    S32 ret = RET_ERR;
    S32 num = 0;
    void *fd = NULL;
    char libFileName[DEV_LIBPOOL_LIB_OPEN_MAX][FILE_NAME_LEN_MAX] = { { 0 } };
    num = DevLibPool_GetFilesFromPath(dir, &libFileName[0][0], "*", "*", "*", LIB_EXTENSION_NAME);
    for (S32 i = 0; i < num; i++) {
        const UINT bindFlags = RTLD_NOW | RTLD_LOCAL;
        fd = dlopen(libFileName[i], bindFlags);
        if (NULL != fd) {
            DEV_LIBPOOL_ENTRY devLibPool_Entry = (DEV_LIBPOOL_ENTRY) dlsym(fd, LIB_ENTRY_INTERFACE);
            if (NULL != devLibPool_Entry) {
                p_libPoolList->entry[p_libPoolList->num] = devLibPool_Entry;
                Dev_snprintf(p_libPoolList->name[p_libPoolList->num], FILE_NAME_LEN_MAX, "%s", __DevLibPool_NoDirFileName(libFileName[i]));
                pFd[p_libPoolList->num] = fd;
//                DEV_LOGI("LOADING LIB [%s]ENTRY[0x%p]FD[%x]", &p_libPoolList->name[p_libPoolList->num][0], p_libPoolList->entry[p_libPoolList->num],
//                        pFd[p_libPoolList->num]);
                p_libPoolList->num++;
                ret = RET_OK;
            } else {
                if (RET_OK != dlclose(fd)) {
                    const char *pErrorMessage = dlerror();
                    DEV_LOGE("CLOSE LIB ERR : %s", (NULL == pErrorMessage) ? pErrorMessage : "N/A");
                }
            }
        }
    }
    return ret;
}

static U32 DevLibPool_DirRevert(DEV_LIBPOOL_DIR_LIST *p_libPoolList_i) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_IF_LOGE_RETURN_RET((p_libPoolList_i == NULL), RET_ERR, "LibPool arg err");
    S32 ret = RET_OK;
    for (int i = 0; i < p_libPoolList_i->list.num; i++) {
        void *fd = p_libPoolList_i->fd[i];
        if (NULL != fd) {
            if (RET_OK != dlclose(fd)) {
                const char *pErrorMessage = dlerror();
                DEV_LOGE("DirRevert ERR : %s", (NULL == pErrorMessage) ? pErrorMessage : "N/A");
                ret = RET_ERR;
            }
        }
    }
    return ret;
}

S64 DevLibPool_OpenDir(const char *dir, DEV_LIBPOOL_LIST *libList, MARK_TAG tagName) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_IF_LOGE_RETURN_RET((libList == NULL)||(dir==NULL), RET_ERR, "LibPool arg err");
    Device->mutex->Lock(m_mutex_Id);
    S64 ret = RET_ERR;
    for (int i = 0; i < m_libPoolListDir_num; i++) {
        if (Dev_strncmp(dir, m_libPoolList[i].dir, FILE_NAME_LEN_MAX) == 0) {
            Dev_memcpy((void*) libList, (void*) &m_libPoolList[i].list, sizeof(DEV_LIBPOOL_LIST));
            m_libPoolList[i].usedNum++;
            ret = RET_OK;
            break;
        }
    }
    if (ret != RET_OK) {
        ret = DevLibPool_DirLoad(dir, m_libPoolList[m_libPoolListDir_num].fd, &m_libPoolList[m_libPoolListDir_num].list);
        if (ret == RET_OK) {
            Dev_snprintf(m_libPoolList[m_libPoolListDir_num].dir, FILE_NAME_LEN_MAX, "%s", dir);
            Dev_snprintf(m_libPoolList[m_libPoolListDir_num].__fileName, FILE_NAME_LEN_MAX, "%s", __DevLibPool_NoDirFileName(__fileName));
            m_libPoolList[m_libPoolListDir_num].__fileLine = __fileLine;
            Dev_memcpy((void*) libList, (void*) &m_libPoolList[m_libPoolListDir_num].list, sizeof(DEV_LIBPOOL_LIST));
            m_libPoolList[m_libPoolListDir_num].usedNum++;
            m_libPoolListDir_num++;
        }
    }
    Device->mutex->Unlock(m_mutex_Id);
    DEV_IF_LOGW_RETURN_RET((ret != RET_OK), RET_ERR, "LibPool No lib found =%s", dir);
    return ret;
}

S64 DevLibPool_CloseDir(const char *dir) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_IF_LOGE_RETURN_RET((dir == NULL), RET_ERR, "LibPool arg err");
    Device->mutex->Lock(m_mutex_Id);
    S64 ret = RET_OK;
    for (int i = 0; i < m_libPoolListDir_num; i++) {
        if (Dev_strncmp(dir, m_libPoolList[i].dir, FILE_NAME_LEN_MAX) == 0) {
            m_libPoolList[i].usedNum--;
            if (m_libPoolList[i].usedNum <= 0) {
                ret |= DevLibPool_DirRevert(&m_libPoolList[i]);
                for (int j = i; j < m_libPoolListDir_num - 1; j++) {
                    m_libPoolList[j] = m_libPoolList[j + 1];
                }
                m_libPoolListDir_num--;
                if (m_libPoolListDir_num <= 0) {
                    m_libPoolListDir_num = 0;
                }
            }
            break;
        }
    }
    Device->mutex->Unlock(m_mutex_Id);
    DEV_IF_LOGW_RETURN_RET((ret != RET_OK), RET_ERR, "LibPool No lib found =%s", dir);
    return ret;
}

static U32 DevLibPool_LibLoad(const char *dir, const char *name, void **pFd, DEV_LIBPOOL_ENTRY *entry) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_IF_LOGE_RETURN_RET((entry == NULL)||(dir==NULL)||(pFd==NULL), RET_ERR, "LibPool arg err");
    S32 ret = RET_ERR;
    void *fd = NULL;
    char libFileName[FILE_NAME_LEN_MAX] = { 0 };
    Dev_snprintf(libFileName, FILE_NAME_LEN_MAX, "%s/%s", dir, name);
    const UINT bindFlags = RTLD_NOW | RTLD_LOCAL;
    fd = dlopen(libFileName, bindFlags);
    if (NULL != fd) {
        DEV_LIBPOOL_ENTRY devLibPool_Entry = (DEV_LIBPOOL_ENTRY) dlsym(fd, LIB_ENTRY_INTERFACE);
        if (NULL != devLibPool_Entry) {
            *entry = devLibPool_Entry;
            *pFd = fd;
//            DEV_LOGI("LOADING LIB [%s]ENTRY[0x%p]FD[%x]", name, *entry, *pFd);
            ret = RET_OK;
        } else {
            if (RET_OK != dlclose(fd)) {
                const char *pErrorMessage = dlerror();
                DEV_LOGE("CLOSE LIB ERR : %s", (NULL == pErrorMessage) ? pErrorMessage : "N/A");
            }
        }
    }
    return ret;
}

S64 DevLibPool_Open(const char *dir, const char *name, DEV_LIBPOOL_ENTRY *entry, MARK_TAG tagName) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_IF_LOGE_RETURN_RET((entry == NULL), RET_ERR, "LibPool arg err");
    DEV_IF_LOGE_RETURN_RET((name == NULL)||(dir==NULL), RET_ERR, "LibPool arg err");

    Device->mutex->Lock(m_mutex_OpneId);
    S64 ret = RET_ERR;
    for (int i = 0; i < m_libPoolListOpen_num; i++) {
        if ((Dev_strncmp(dir, m_libPoolOpenList[i].dir, FILE_NAME_LEN_MAX) == 0)
                || (Dev_strncmp(name, m_libPoolOpenList[i].name, FILE_NAME_LEN_MAX) == 0)) {
            *entry = m_libPoolOpenList[i].entry;
            m_libPoolOpenList[i].usedNum++;
            ret = RET_OK;
            break;
        }
    }
    if (ret != RET_OK) {
        ret = DevLibPool_LibLoad(dir, name, &m_libPoolOpenList[m_libPoolListOpen_num].fd, &m_libPoolOpenList[m_libPoolListOpen_num].entry);
        if (ret == RET_OK) {
            Dev_snprintf(m_libPoolOpenList[m_libPoolListOpen_num].dir, FILE_NAME_LEN_MAX, "%s", dir);
            Dev_snprintf(m_libPoolOpenList[m_libPoolListOpen_num].__fileName, FILE_NAME_LEN_MAX, "%s", __DevLibPool_NoDirFileName(__fileName));
            Dev_snprintf(m_libPoolOpenList[m_libPoolListOpen_num].name, FILE_NAME_LEN_MAX, "%s", name);
            m_libPoolOpenList[m_libPoolListOpen_num].__fileLine = __fileLine;
            *entry = m_libPoolOpenList[m_libPoolListOpen_num].entry;
            m_libPoolOpenList[m_libPoolListOpen_num].usedNum++;
            m_libPoolListOpen_num++;
        }
    }
    Device->mutex->Unlock(m_mutex_OpneId);
    DEV_IF_LOGW_RETURN_RET((ret != RET_OK), RET_ERR, "LibPool No lib found =%s", dir);
    return ret;
}

S64 DevLibPool_Close(const char *dir, const char *name) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_IF_LOGE_RETURN_RET((dir == NULL)||(dir == name), RET_ERR, "LibPool arg err");
    Device->mutex->Lock(m_mutex_OpneId);
    S64 ret = RET_OK;
    for (int i = 0; i < m_libPoolListOpen_num; i++) {
        if (Dev_strncmp(dir, m_libPoolOpenList[i].dir, FILE_NAME_LEN_MAX) == 0) {
            m_libPoolOpenList[i].usedNum--;
            if (m_libPoolOpenList[i].usedNum <= 0) {
                void *fd = m_libPoolOpenList[i].fd;
                if (NULL != fd) {
                    if (RET_OK != dlclose(fd)) {
                        const char *pErrorMessage = dlerror();
                        DEV_LOGE("DirClose ERR : %s", (NULL == pErrorMessage) ? pErrorMessage : "N/A");
                        ret = RET_ERR;
                    }
                }
                for (int j = i; j < m_libPoolListOpen_num - 1; j++) {
                    m_libPoolOpenList[j] = m_libPoolOpenList[j + 1];
                }
                m_libPoolListOpen_num--;
                if (m_libPoolListOpen_num <= 0) {
                    m_libPoolListOpen_num = 0;
                }
            }
            break;
        }
    }
    Device->mutex->Unlock(m_mutex_OpneId);
    DEV_IF_LOGW_RETURN_RET((ret != RET_OK), RET_ERR, "LibPool No lib found =%s", dir);
    return ret;
}

S64 DevLibPool_Report(void) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    S64 ret = RET_OK;
    DEV_LOGI("[------------------------LIB OPEN REPORT START-----------------]");
    for (int i = 0; i < m_libPoolListDir_num; i++) {
        if (m_libPoolList[i].usedNum > 0) {
            DEV_LOGI("[---APPLY MARK FILE[%s][%d]--------------]", m_libPoolList[i].__fileName, m_libPoolList[i].__fileLine);
            DEV_LOGI("[---OPEN DIR [%d/%d][%s]--------------]", i + 1, m_libPoolListDir_num, m_libPoolList[i].dir);
            for (int j = 0; j < m_libPoolList[i].list.num; j++) {
                DEV_LOGI("[---OPEN LIB [%d/%d][%s]ENTRY[%p]FD[%x]---]", j + 1, m_libPoolList[i].list.num, m_libPoolList[i].list.name[j],
                        m_libPoolList[i].list.entry[j], m_libPoolList[i].fd[j]);
            }
        }
    }

    for (int i = 0; i < m_libPoolListOpen_num; i++) {
        DEV_LOGI("[---APPLY MARK FILE[%s][%d]--------------]", m_libPoolOpenList[i].__fileName, m_libPoolOpenList[i].__fileLine);
        DEV_LOGI("[---OPEN LIB [%d/%d][%s][%s]ENTRY[%p]FD[%p]---]", i + 1, m_libPoolListOpen_num, m_libPoolOpenList[i].dir,
                m_libPoolOpenList[i].name, m_libPoolOpenList[i].entry, m_libPoolOpenList[i].fd);
    }

    DEV_LOGI("[------------------------LIB OPEN REPORT END-------------------]");
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "LibPool Init err[%ld]", ret);
    return ret;
}

S64 DevLibPool_Init(void) {
    if (init_f == TRUE) {
        return RET_OK;
    }
    S64 ret = RET_OK;
    ret |= Device->mutex->Create(&m_mutex_Id, MARK("m_mutex_Id"));
    ret |= Device->mutex->Create(&m_mutex_OpneId, MARK("m_mutex_OpneId"));
    memset((void*) &m_libPoolList, 0, sizeof(m_libPoolList));
    m_libPoolListDir_num = 0;
    memset((void*) &m_libPoolOpenList, 0, sizeof(m_libPoolOpenList));
    m_libPoolListOpen_num = 0;
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "LibPool Init err[%ld]", ret);
    init_f = TRUE;
    return ret;
}

S64 DevLibPool_Deinit(void) {
    if (init_f != TRUE) {
        return RET_OK;
    }
    S64 ret = RET_OK;
    if ((m_libPoolListDir_num > 0) || (m_libPoolListOpen_num > 0)) {
        DevLibPool_Report();
    }
    //TODO 强制关闭打开的LIB
    ret |= Device->mutex->Destroy(&m_mutex_Id);
    ret |= Device->mutex->Destroy(&m_mutex_OpneId);
    init_f = FALSE;
    DEV_IF_LOGE_RETURN_RET((ret != RET_OK), RET_ERR, "LibPool Deinit err[%ld]", ret);
    return ret;
}

const Dev_LibPool m_dev_libPool = {
        .Init           =DevLibPool_Init,
        .Deinit         =DevLibPool_Deinit,
        .OpenDir        =DevLibPool_OpenDir,
        .CloseDir       =DevLibPool_CloseDir,
        .Open           =DevLibPool_Open,
        .Close          =DevLibPool_Close,
        .Report         =DevLibPool_Report,
};
