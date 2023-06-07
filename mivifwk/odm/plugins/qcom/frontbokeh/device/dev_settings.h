/**
 * @file        dev_settings.h
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

#ifndef __DEV_SETTINGS_H__
#define __DEV_SETTINGS_H__

#define DEV_SETTING_CHAR_BUF_MAX    (256)
#define DEV_SETTING_NAEM_MAX        (64)

typedef enum DevSetting_Type {
    DEV_SETTING_TYPE_BOOL = 1,
    DEV_SETTING_TYPE_INT = 2,
    DEV_SETTING_TYPE_FLOAT = 3,
    DEV_SETTING_TYPE_CHAR = 4,
    DEV_SETTING_TYPE_END,
} DEV_SETTING_TYPE;

#define ADD_SETTINGS_START  struct __SettingsTable {
#define ADD_SETTINGS_END    };

#define ADD_bool_SETTINGS(settingName,defaultValue)                     \
        struct settingName {                                            \
            char name[DEV_SETTING_NAEM_MAX]=#settingName;               \
            U8 type=DEV_SETTING_TYPE_BOOL;                              \
            U8 defaultFlag=1;                                           \
            union value {                                               \
                double float_value;                                     \
                char char_value[DEV_SETTING_CHAR_BUF_MAX];              \
                int int_value;                                          \
                int bool_value = defaultValue;                          \
            }value;                                                     \
        }settingName

#define ADD_int_SETTINGS(settingName,defaultValue)                      \
        struct settingName {                                            \
            char name[DEV_SETTING_NAEM_MAX]=#settingName;               \
            U8 type=DEV_SETTING_TYPE_INT;                               \
            U8 defaultFlag=1;                                           \
            union value {                                               \
                double float_value;                                     \
                char char_value[DEV_SETTING_CHAR_BUF_MAX];              \
                int int_value = defaultValue;                           \
                int bool_value;                                         \
            }value;                                                     \
        }settingName

#define ADD_float_SETTINGS(settingName,defaultValue)                    \
        struct settingName {                                            \
            char name[DEV_SETTING_NAEM_MAX]=#settingName;               \
            U8 type=DEV_SETTING_TYPE_FLOAT;                             \
            U8 defaultFlag=1;                                           \
            union value {                                               \
                double float_value=defaultValue;                        \
                char char_value[DEV_SETTING_CHAR_BUF_MAX];              \
                int int_value;                                          \
                int bool_value;                                         \
            }value;                                                     \
        }settingName

#define ADD_char_SETTINGS(settingName,defaultValue)                     \
        struct settingName {                                            \
            char name[DEV_SETTING_NAEM_MAX]=#settingName;               \
            U8 type=DEV_SETTING_TYPE_CHAR;                              \
            U8 defaultFlag=1;                                           \
            union value {                                               \
                double float_value;                                     \
                char char_value[DEV_SETTING_CHAR_BUF_MAX]=defaultValue; \
                int int_value;                                          \
                int bool_value;                                         \
            }value;                                                     \
        }settingName

#define ADD_SETTINGS(type,settingName,defaultValue) ADD_##type##_SETTINGS(settingName,defaultValue)

typedef struct __Dev_Settings Dev_Settings;
struct __Dev_Settings {
    S32 (*Init)(void);
    S32 (*Deinit)(void);
    S32 (*Report)(void);
    S32 (*Save)(void);
    S32 (*SetDef)(void);
};

extern const Dev_Settings m_dev_settings;

typedef struct __SettingsTable SettingsTable;

SettingsTable* DevSettings_getter(void);

#define Settings DevSettings_getter()


#endif /* __DEV_SETTINGS_H__ */

