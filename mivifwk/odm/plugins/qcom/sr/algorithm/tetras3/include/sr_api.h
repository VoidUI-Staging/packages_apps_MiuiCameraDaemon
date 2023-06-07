#ifndef __SR_API_H__
#define __SR_API_H__

#include "sr_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/// ＠brief  sr初始化，获取句柄
/// @param  [in]  info     camera的信息。包含ID跟名字
/// @param  [out] pHandle　所创建的sr的句柄
/// @return 该函数执行所产生的返回码（０－－正常运行）
SR_SDK_API
int sr_create_ex(sr_camera_info_t info, void **phandle);
typedef int (*FUNCPTR_SR_CREATE_EX)(sr_camera_info_t info, void **phandle);

/// ＠brief  sr初始化，获取句柄
/// @param  [out] pHandle　所创建的sr的句柄
/// @return 该函数执行所产生的返回码（０－－正常运行）
SR_SDK_API
int sr_create(void **phandle);
typedef int (*FUNCPTR_SR_CREATE)(void **phandle);

/// ＠brief 销毁sr句柄
/// @param [in] handle  sr句柄
/// @return 该函数执行所产生的返回码（０－－正常运行）
SR_SDK_API
int sr_destroy(void *handle);
typedef int (*FUNCPTR_SR_DESTORY)(void *handle);

/// ＠brief 对给定的多张图像进行sr处理
/// @param [in]  handle　 sr句柄
/// @param [in]  p_in_img 待进行sr处理的多张输入图像
/// @param [in]  nImageNum 输入图像p_in_img对应的图像张数
/// @param [in]  imgRefIdx 输入图像中reference图的索引
/// @param [in]  fScaleR  需要放大的倍率
/// @param [in]  pSRParam 超分的接口参数结构体指针
/// @param [out] out_img　sr处理完后的输出图像，须调用cv_image_release进行释放
SR_SDK_API
int sr_run_scale_ex(const void *handle, const sr_image_t *p_in_img, const int nImageNum,
                    const int imgRefIdx, const float fScaleR, const sr_interface_param_t *pSRParam,
                    sr_image_t *out_img);
typedef int (*FUNCPTR_SR_RUN_SCALE_EX)(const void *handle, const sr_image_t *p_in_img,
                                       const int nImageNum, const int imgRefIdx,
                                       const float fScaleR, const sr_interface_param_t *pSRParam,
                                       sr_image_t *out_img);

/// ＠brief 根据fScaleR对给定的多张图像进行sr处理
///         为兼容旧版SDK,不推荐使用
/// @param [in]  handle　 sr句柄
/// @param [in]  p_in_img 待进行sr处理的多张输入图像
/// @param [in]  nImageNum 输入图像p_in_img对应的图像张数
/// @param [in]  fScaleR  需要放大的倍率
/// @param [in]  pSRParam 超分的接口参数结构体指针
/// @param [out] out_img　sr处理完后的输出图像，须调用cv_image_release进行释放
SR_SDK_API
int sr_run_scale(const void *handle, const sr_image_t *p_in_img, const int nImageNum,
                 const float fScaleR, const sr_interface_param_t *pSRParam, sr_image_t *out_img);
typedef int (*FUNCPTR_SR_RUN_SCALE)(const void *handle, const sr_image_t *p_in_img,
                                    const int nImageNum, const float fScaleR,
                                    const sr_interface_param_t *pSRParam, sr_image_t *out_img);

/// ＠brief 对给定的多张图像进行sr处理
/// @param [in]  handle　 sr句柄
/// @param [in]  p_in_img 待进行sr处理的多张输入图像
/// @param [in]  nImageNum 输入图像p_in_img对应的图像张数
/// @param [in]  imgRefIdx 输入图像中reference图的索引
/// @param [in]  pRect    需要放大的矩形区域
/// @param [in]  pSRParam 超分的接口参数结构体指针
/// @param [out] out_img　sr处理完后的输出图像，须调用cv_image_release进行释放
SR_SDK_API
int sr_run_ex(const void *handle, const sr_image_t *p_in_img, const int nImageNum,
              const int imgRefIdx, const sr_rect_t *pRect, const sr_interface_param_t *pSRParam,
              sr_image_t *out_img);
typedef int (*FUNCPTR_SR_RUN_EX)(const void *handle, const sr_image_t *p_in_img,
                                 const int nImageNum, const int imgRefIdx, const sr_rect_t *pRect,
                                 const sr_interface_param_t *pSRParam, sr_image_t *out_img);

/// ＠brief 根据pRect对给定的多张图像进行sr处理
///         新版SDK接口,推荐使用
/// @param [in]  handle　 sr句柄
/// @param [in]  p_in_img 待进行sr处理的多张输入图像
/// @param [in]  nImageNum 输入图像p_in_img对应的图像张数
/// @param [in]  pRect    需要放大的矩形区域
/// @param [in]  pSRParam 超分的接口参数结构体指针
/// @param [out] out_img　sr处理完后的输出图像，须调用cv_image_release进行释放
SR_SDK_API
int sr_run(const void *handle, const sr_image_t *p_in_img, const int nImageNum,
           const sr_rect_t *pRect, const sr_interface_param_t *pSRParam, sr_image_t *out_img);
typedef int (*FUNCPTR_SR_RUN)(const void *handle, const sr_image_t *p_in_img, const int nImageNum,
                              const sr_rect_t *pRect, const sr_interface_param_t *pSRParam,
                              sr_image_t *out_img);

/// ＠brief 获取超分SDK版本号
/// @return SDK版本号字符串头指针
SR_SDK_API
const char *sr_getversion();
typedef const char *(*FUNCPTR_SR_GETVERSION)();

/// ＠brief  超清画质sr初始化，获取句柄
/// @param  [out] pHandle　所创建的sr的句柄
/// @return 该函数执行所产生的返回码（０－－正常运行）
SR_SDK_API
int srsuper_create(void **phandle);
typedef int (*FUNCPTR_SRSUPER_CREATE)(void **phandle);

/// ＠brief 销毁sr句柄
/// @param [in] handle  sr句柄
/// @return 该函数执行所产生的返回码（０－－正常运行）
SR_SDK_API
int srsuper_destroy(void *handle);
typedef int (*FUNCPTR_SRSUPER_DESTORY)(void *handle);

/// ＠brief 对给定的多张图像进行超清画质算法处理
/// @param [in] handle　 sr句柄
/// @param [in] p_in_img 待进行sr处理的多张输入图像
/// @param [in] nImageNum 输入图像v_in_img对应的图像张数
/// @param [in] fScaleR 需要放大的倍率
/// @param [in] pSrParam 超分的接口参数结构体指针
/// @param [out] out_img　sr处理完后的输出图像的结构体指针，由调用者创建内存
SR_SDK_API
int srsuper_run(const void *handle, const sr_image_t *p_in_img, const int nImageNum,
                const float fScaleR, const sr_interface_param_t *pSRParam, sr_image_t *p_out_img);
typedef int (*FUNCPTR_SRSUPER_RUN)(const void *handle, const sr_image_t *p_in_img,
                                   const int nImageNum, const float fScaleR,
                                   const sr_interface_param_t *pSRParam, sr_image_t *p_out_img);

// ＠brief 获取超分SDK版本号
/// @return SDK版本号字符串头指针
SR_SDK_API
const char *srsuper_getversion();
typedef const char *(*FUNCPTR_SRSUPER_GETVERSION)();

SR_SDK_API void sr_set_log_level(int level);

/// ＠brief 用于设置事件回调函数，在sr_run之前调用
/// @param [in] handle　 sr句柄
/// @param [in] callback 回调函数指针，函数能够处理各类事件，需要确保函数是非阻塞的，
///                      且时间在500us以内
SR_SDK_API
int sr_set_callback(const void *handle, FUNCPTR_SR_EVENT_CALLBACK callback);

/// ＠brief 用于设置扩展接口函数
/// @param [in] handle　 sr句柄
/// @param [in] extension_name  扩展函数名
/// @param [in] extension_param 扩展函数参数，顺序及union成员按照指定方式
/// @param [in]                 当扩展crop数据输入时候，指定extention_name为
/// SR_EXTENTION_CROPPED_INPUT_RUN_T
/// @param [in]
/// 当扩展crop数据输入时候，extention_param为union的i成员，分别为原始输入图像宽，原始输入图像高，
/// 原始输出图像宽，原始输出图像高
/// @param [in]                                                                   crop区域x, y,
/// width, height
SR_SDK_API
int sr_set_extention(const void *handle, const sr_extention_t extention_name,
                     const sr_extention_para_t *extention_param);

// ＠brief 设置拓展模块传入的参数
/// @param [in] handle　  sr句柄
/// @param [in] func_name 拓展feature的名称
/// @param [in] arg_idx   拓展feature的第几个参数
/// @param [in] arg_bytes 参数数据的长度
/// @param [in] arg_type  参数数据的类型
/// @param [in] arg_ptr   参数数据地址的指针
/// @param [in] is_tmp_memory 参数数据是否临时内存，指示内部是否需要自己存储
/// @param [in] arg_pp        预留拓展用，目前没用到
/// @return API调用result，表示是否成功
SR_SDK_API
int sr_set_extention_ptr(const void *handle, const sr_extention_t func_name, const int arg_idx,
                         const int arg_bytes, const sr_ext_arg_type_t arg_type, const void *arg_ptr,
                         const bool is_tmp_memory, void **arg_pp);

// ＠brief 处理预览raw数据并返回对应参数
/// @param [in] handle　  sr句柄
/// @param [in] p_in_img  待进行处理的raw数据
/// @param [in] nImageNum 输入图像p_in_img对应的图像张数
/// @param [out] out_param 根据输入图像计算的输出参数
/// @return API调用result，表示是否成功
SR_SDK_API
int sr_run_preview_raw(const void *handle,               ///<初始化句柄
                       const sr_image_t *p_in_img,       ///<输入raw数据指针
                       const sr_bhist_adrc_t *in_params, ///<图像adrc与gain
                       sr_prev_calc_value_t *out_param   ///< 输出曝光序列
);
typedef int (*FUNCPTR_SR_RUN_PREVIEW_RAW)(const void *handle, const sr_image_t *p_in_img,
                                          const sr_bhist_adrc_t *in_params, ///<图像adrc与gain
                                          sr_prev_calc_value_t *out_param);

#ifdef __cplusplus
}
#endif

#endif /* __SR_API_H__ */
