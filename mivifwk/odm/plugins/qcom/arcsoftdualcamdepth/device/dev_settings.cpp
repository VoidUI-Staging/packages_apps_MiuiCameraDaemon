/**
 * @file        dev_settings.cpp
 * @brief
 * @details
 * @author      LZL
 * @date        2021.04.06
 * @version     0.1
 * @note
 * @warning
 * @par         History:
 *
 */

#include "device.h"
#include "settings.h"
#include "dev_settings.h"

#define SETTINGS_FILE_SIZE_MAX  (1024*1024*8)

typedef struct __Dev_settingNode Dev_settingNode;
struct __Dev_settingNode {
    U8 name[DEV_SETTING_NAEM_MAX];
    U8 type;
    U8 defaultFlag;
    union value {
        double float_value;
        char char_value[DEV_SETTING_CHAR_BUF_MAX];
        int int_value;
        int bool_value;
    } value;
};

#define SETTINGS_TABLE_NODE_SIZE    (sizeof(Dev_settingNode))

static SettingsTable m_settingsTable;

static U8 m_settings_buf[SETTINGS_FILE_SIZE_MAX] = { 0 };

static U8 init_f = FALSE;

SettingsTable* DevSettings_getter(void) {
    return &m_settingsTable;
}

S32 DevSettings_Save(void) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_LOGI("USER SETTINGS SAVE!");
    return RET_OK;
}

S32 DevSettings_Def(void) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    DEV_LOGI("USER SETTINGS SET DEFAULT!");
    return RET_OK;
}

static S32 DevSettings_Match(void) {
    U32 checkNum = 0;
    S32 settingsNum = sizeof(m_settingsTable) / SETTINGS_TABLE_NODE_SIZE;
    DEV_IF_LOGW_RETURN_RET(settingsNum <= 0, RET_ERR, "USER SETTINGS NUM ERR");
    Dev_settingNode *settingTable = (Dev_settingNode*) &m_settingsTable;
    for (int i = 0; i < settingsNum; i++) {
        char tempName[DEV_SETTING_NAEM_MAX + 5] = { 0 };
        Dev_sprintf(tempName, "\n%s=", settingTable[i].name); //"key=value"
        char *haveFoundTemp = Dev_strstr((char*) m_settings_buf, (char*) tempName);
        char *haveFound = NULL;
        while (haveFoundTemp != NULL) { //Find the last one
            haveFound = haveFoundTemp;
            haveFoundTemp = Dev_strstr((char*) haveFoundTemp + 1, (char*) tempName);
        };

        if (haveFound == NULL) {
            Dev_sprintf(tempName, "\n%s =", settingTable[i].name); //"key =value"
            haveFoundTemp = Dev_strstr((char*) m_settings_buf, (char*) tempName);
            haveFound = NULL;
            while (haveFoundTemp != NULL) { //Find the last one
                haveFound = haveFoundTemp;
                haveFoundTemp = Dev_strstr((char*) haveFoundTemp + 1, (char*) tempName);
            };
        }

        if (haveFound != NULL) {
            if (settingTable[i].type == DEV_SETTING_TYPE_BOOL) {
                char tempValueBool[DEV_SETTING_CHAR_BUF_MAX] = { 0 };
                int n = Dev_sscanf(haveFound, "%[^=]=%s", tempName, tempValueBool);
                int tempValue = 0;
                if ((Dev_strstr((char*) tempValueBool, (char*) "TRUE") != NULL) || (Dev_strstr((char*) tempValueBool, (char*) "true") != NULL)
                        || (Dev_strstr((char*) tempValueBool, (char*) "1") != NULL)) {
                    tempValue = 1;
                }
                if ((Dev_strstr((char*) tempValueBool, (char*) "FALSE") != NULL) || (Dev_strstr((char*) tempValueBool, (char*) "false") != NULL)
                        || (Dev_strstr((char*) tempValueBool, (char*) "0") != NULL)) {
                    tempValue = 0;
                }
                if (n > 1) {
                    settingTable[i].value.bool_value = tempValue;
                    settingTable[i].defaultFlag = 0;
                    checkNum++;
                }
            } else if (settingTable[i].type == DEV_SETTING_TYPE_INT) {
                char tempValueHex[DEV_SETTING_CHAR_BUF_MAX] = { 0 };
                int n = Dev_sscanf(haveFound, "%[^=]=%s", tempName, tempValueHex);
                int tempValue = Dev_strtol(tempValueHex, NULL, 16);
                if (n > 1) {
                    settingTable[i].value.int_value = tempValue;
                    settingTable[i].defaultFlag = 0;
                    checkNum++;
                }
            } else if (settingTable[i].type == DEV_SETTING_TYPE_FLOAT) {
                double tempValue = 0;
                int n = Dev_sscanf(haveFound, "%[^=]=%lf", tempName, &tempValue);
                if (n > 1) {
                    settingTable[i].value.float_value = tempValue;
                    settingTable[i].defaultFlag = 0;
                    checkNum++;
                }
            } else if (settingTable[i].type == DEV_SETTING_TYPE_CHAR) {
                char tempValue[DEV_SETTING_CHAR_BUF_MAX] = { 0 };
                int n = Dev_sscanf(haveFound, "%[^=]=%s", tempName, tempValue);
                if (n > 1) {
                    Dev_sprintf((char*) settingTable[i].value.char_value, "%s", tempValue);
                    settingTable[i].defaultFlag = 0;
                    checkNum++;
                }
            }
        }
    }
    DEV_IF_LOGW(checkNum == 0, "USER SETTINGS No matching settings form settings file!");
    return RET_OK;
}

static S32 DevSettings_Report(void) {
    DEV_IF_LOGE_RETURN_RET((init_f != TRUE), RET_ERR, "NOT INIT");
    S32 settingsNum = sizeof(m_settingsTable) / SETTINGS_TABLE_NODE_SIZE;
    DEV_IF_LOGW_RETURN_RET(settingsNum <= 0, RET_ERR, "USER SETTINGS NUM ERR");
    DEV_LOGI("[------------------------SETTINGS REPORT START-----------------]");
//    DEV_LOGI("USER SETTINGS NUM =%ld", settingsNum);
    Dev_settingNode *settingTable = (Dev_settingNode*) &m_settingsTable;
    for (int i = 0; i < settingsNum; i++) {
        if (settingTable[i].type == DEV_SETTING_TYPE_BOOL) {
            DEV_LOGI("USER SETTINGS[%s][default=%d][%d/%d][%s=%d=0x%x]", "BOOL ", settingTable[i].defaultFlag, i + 1, settingsNum,
                    settingTable[i].name, settingTable[i].value.bool_value, settingTable[i].value.bool_value);
        } else if (settingTable[i].type == DEV_SETTING_TYPE_INT) {
            DEV_LOGI("USER SETTINGS[%s][default=%d][%d/%d][%s=%d=0x%x]", "INT  ", settingTable[i].defaultFlag, i + 1, settingsNum, settingTable[i].name,
                    settingTable[i].value.int_value, settingTable[i].value.int_value);
        } else if (settingTable[i].type == DEV_SETTING_TYPE_FLOAT) {
            DEV_LOGI("USER SETTINGS[%s][default=%d][%d/%d][%s=%f]", "FLOAT", settingTable[i].defaultFlag, i + 1, settingsNum, settingTable[i].name,
                    settingTable[i].value.float_value);
        } else if (settingTable[i].type == DEV_SETTING_TYPE_CHAR) {
            DEV_LOGI("USER SETTINGS[%s][default=%d][%d/%d][%s=%s]", "CHAR ", settingTable[i].defaultFlag, i + 1, settingsNum, settingTable[i].name,
                    settingTable[i].value.char_value);
        }
    }
    DEV_LOGI("[------------------------SETTINGS REPORT END  -----------------]");
    return RET_OK;
}

S32 DevSettings_Init(void) {
    init_f = TRUE;
    DEV_IF_LOGW_RETURN_RET(Device->file->GetSize(SETTINGS_FILE_C1) <= 0, RET_ERR, "SETTINGS FILE SIZE ERR");
    DEV_IF_LOGE_RETURN_RET(Device->file->GetSize(SETTINGS_FILE_C1) > SETTINGS_FILE_SIZE_MAX, RET_ERR, "SETTINGS FILE SIZE ERR");
    S32 size = Device->file->ReadOnce((const char*) SETTINGS_FILE_C1, m_settings_buf, Device->file->GetSize(SETTINGS_FILE_C1), 0);
    DEV_IF_LOGE_RETURN_RET(size <= 0, RET_ERR, "SETTINGS FILE OPEN ERR");
    DEV_IF_LOGE_RETURN_RET((DevSettings_Match() != RET_OK), RET_ERR, "USER SETTINGS CHECK ERROR!");
//    DevSettings_Report();
    return RET_OK;
}

S32 DevSettings_Deinit(void) {
    if (init_f != TRUE) {
        return RET_OK;
    }
    init_f = FALSE;
    return RET_OK;
}

const Dev_Settings m_dev_settings = {
        .Init       = DevSettings_Init,
        .Deinit     = DevSettings_Deinit,
        .Report     = DevSettings_Report,
        .Save       = DevSettings_Save,
        .SetDef     = DevSettings_Def,
};
