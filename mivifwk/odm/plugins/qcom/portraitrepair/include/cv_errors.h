#ifndef CV_SDK_TOOL_ERRORS_H_
#define CV_SDK_TOOL_ERRORS_H_ 1

///< @brief 返回错误类型
///< @code -10001 ~ -10999 通用错误类型
///<    -10000 ~ -10099 基本错误类型
///<    -10100 ~ -10199 模型相关错误类型
///<    -10200 ~ -10299 授权相关错误类型
///< @code -11000 ~ -19999 预留或未知错误类型
enum cv_sdk_base_error_ {
    ///< success
    STM_OK = 0,
    ///< invalid argument
    STM_E_INVALIDARG = -10001,
    ///< handle error
    STM_E_HANDLE = -10002,
    ///< out of memory
    STM_E_OUTOFMEMORY = -10003,
    ///< unkown error
    STM_E_FAIL = -10004,
    ///< definition not found
    STM_E_DELNOTFOUND = -10005,
    ///< invalid image format
    STM_E_INVALID_PIXEL_FORMAT = -10006,
    ///< file not exists
    STM_E_FILE_NOT_FOUND = -10007,
    ///< image format unrecognized
    STM_E_RET_UNRECOGNIZED = -10008,
    ///< error context
    STM_E_CONTEXT = -10009,

    ///< 读取模型文件失败
    STM_E_MODEL_READ_FILE_FAIL = -10101,
    ///< 模型格式不正确导致加载失败
    STM_E_MODEL_INVALID_FORMAT = -10102,
    ///< 模型文件过期
    STM_E_MODEL_EXPIRE = -10103,
    ///< 模型解压缩失败
    STM_E_MODEL_UNZIP_FAILED = -10104,
    ///< 子模型不存在
    STM_E_MODEL_SUBMODULE_NON_EXIT = -10105,

    ///< invliad license file
    STM_E_LIC_INVALID_AUTH = -10201,
    ///< could not activate offline
    STM_E_LIC_IS_NOT_ACTIVABLE = -10202,
    ///< activate failed
    STM_E_LIC_ACTIVE_FAIL = -10203,
    ///< invalid offline activate code
    STM_E_LIC_ACTIVE_CODE_INVALID = -10204,
    ///< wrong package name
    STM_E_LIC_INVALID_APPID = -10205,
    ///< license expired
    STM_E_LIC_AUTH_EXPIRE = -10206,
    ///< current platform is not allowed
    STM_E_LIC_PLATFORM_NOTSUPPORTED = -10207,
    ///< product version mismatch with license
    STM_E_LIC_PRODUCT_VERSION_MISMATCH = -10208,

    ///< interface unimplemented
    STM_E_STUB = -15001,
    ///< unsupported
    STM_E_UNSUPPORTED = -15002,
    ///< Everything went fine but need more input
    STM_E_CONTINUE = -15003
};

#endif