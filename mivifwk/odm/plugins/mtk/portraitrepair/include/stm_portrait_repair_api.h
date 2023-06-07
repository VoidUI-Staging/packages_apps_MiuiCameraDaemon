/*===========================================================================
   Copyright (c) 2018,2020 by Tetras Technologies, Incorporated.
   All Rights Reserved.
============================================================================*/

/*===========================================================================

                           EDIT HISTORY FOR FILE

This section contains comments describing changes made to this file.
Notice that changes are listed in reverse chronological order.
Modification record
1. 2019-9-20, create the first version apis.
2. 2020-1-23, add config and hair remove api.
3. 2020-3-18, add no ref process api.
4. 2021-5-25, add create_with_allocator api for honor
============================================================================*/

#ifndef STM_PORTRAIT_REPAIR_API_H_
#define STM_PORTRAIT_REPAIR_API_H_ 1

#include "stm_common.h"

///< @brief 获取库版本号
///< @return 返回版本号, 不需要释放内存
PORTRAIT_REPAIR_API
const char *stm_portrait_repair_version();

///< @brief 创建人像修复句柄
///< @param [in] models 输入模型组, 为NULL时将采用内部默认模型
///< @param [in] model_count 输入模型个数
///< @param [out] handle 初始化好的句柄
///< @return 创建成功返回STM_OK 失败返回错误码
PORTRAIT_REPAIR_API
stm_result_t stm_portrait_repair_create(const stm_model_t *models, const int model_count,
                                        stm_handle_t *handle_out);

///< @brief 创建人像修复句柄 - 新增
///< @param [in] models 输入模型组, 为NULL时将采用内部默认模型
///< @param [in] model_count 输入模型个数
///< @param [in] gwpasan_allocator 内存池管理接口，非空，可参考“HwaAlgoMemAllocatorManager.h”文件中
///< hw::hwa_algo_mem_allocator_impl_t定义
///< @param [out] handle 初始化好的句柄
///< @return 创建成功返回STM_OK 失败返回错误码
PORTRAIT_REPAIR_API
stm_result_t stm_portrait_repair_create_with_allocator(const stm_model_t *models,
                                                       const int model_count,
                                                       void *gwpasan_allocator,
                                                       stm_handle_t *handle_out);

///< @brief 销毁句柄并释放模型等内存
///< @param [in] handle 修复上下文句柄
PORTRAIT_REPAIR_API
void stm_portrait_repair_destroy(stm_handle_t handle);

///< @brief 不依赖参考图执行人像修复, 可以配置ISO
///< @param [in] handle 修复上下文句柄
///< @param [in] input_image 待修复的图像
///< @param [in] rects 待修复上的人脸框
///< @param [in] rects_count 待修复图人脸个数
///< @param [in] iso_value 图片ISO值，必要参数
///< @param [in] face_angle 图片人脸角度，必要参数，取值是 0， 90， 180，
///< 270其中之一，分别代表正向，右偏水平，倒向，左偏水平
///< @param [out] out_image 修复后的图像, 内存由接口分配, 未检测到人脸会直接返回输入图
///< @return 处理成功返回STM_OK 失败返回错误码
PORTRAIT_REPAIR_API
stm_result_t stm_portrait_repair_process_v4(const stm_handle_t handle,
                                            const stm_image_t *input_image, const stm_rect_t *rects,
                                            const int rects_count, float iso_value, int face_angle,
                                            stm_image_t **out_image);

///< @brief 不依赖参考图执行人像修复, 可以配置ISO, 输入输出用同一个buffer
///< @param [in] handle 修复上下文句柄
///< @param [in/out] image 待修复的图像, 如果图像执行了修复, 则修复后的图像填充到该buffer
///< @param [in] rects 待修复上的人脸框
///< @param [in] rects_count 待修复图人脸个数
///< @param [in] iso_value 图片ISO值，必要参数
///< @param [in] face_angle 图片人脸角度，必要参数，取值是 0， 90， 180，
///< 270其中之一，分别代表正向，右偏水平，倒向，左偏水平
///< @return 处理成功返回STM_OK 失败返回错误码
PORTRAIT_REPAIR_API
stm_result_t stm_portrait_repair_process_ext(const stm_handle_t handle, stm_image_t *image,
                                             const stm_rect_t *rects, const int rects_count,
                                             float iso_value, int face_angle);

///< @brief 针对输入的YUV图执行人像修复， 修复结果数据将覆盖原输入数据，并保持输入数据格式
///< @param [in] handle 修复上下文句柄
///< @param [in/out] image 待修复的图像, 如果图像执行了修复, 则修复后的图像填充到该buffer
///< @param [in] rects 待修复上的人脸框
///< @param [in] rects_count 待修复图人脸个数
///< @param [in] iso_value 图片ISO值，必要参数
///< @param [in] face_angle 图片人脸角度，必要参数，取值是 0， 90， 180，
///< 270其中之一，分别代表正向，右偏水平，倒向，左偏水平
///< @return 处理成功返回STM_OK 失败返回错误码
PORTRAIT_REPAIR_API
stm_result_t stm_portrait_repair_process_with_yuv(const stm_handle_t handle, yuv_image_t *yuv_image,
                                                  const stm_rect_t *rects, const int rects_count,
                                                  float iso_value, int face_angle);

///< @brief 针对输入的YUV图执行人像修复， 修复结果数据将覆盖原输入数据，并保持输入数据格式
///< @param [in] handle 修复上下文句柄
///< @param [in/out] image 待修复的图像, 如果图像执行了修复, 则修复后的图像填充到该buffer
///< @param [in] rects 待修复上的人脸框
///< @param [in] rects_count 待修复图人脸个数
///< @param [in] iso_value 图片ISO值，必要参数
///< @param [in] face_angle 图片人脸角度，必要参数，取值是 0， 90， 180，
///< 270其中之一，分别代表正向，右偏水平，倒向，左偏水平
///< @param [out] img_ext 人像修复结果,外部传入结构体地址,当输入为nullptr时,结果体不填充
///< @return 处理成功返回STM_OK 失败返回错误码
PORTRAIT_REPAIR_API
stm_result_t stm_portrait_repair_process_with_yuv_ext(const stm_handle_t handle,
                                                      yuv_image_t *yuv_image,
                                                      const stm_rect_t *rects,
                                                      const int rects_count, float iso_value,
                                                      int face_angle, FACESR_EXIF_INFO *img_ext);

///< @brief 释放接口分配的图像对象
///< @param [in] image 接口内分配的图像对象
PORTRAIT_REPAIR_API
void stm_portrait_repair_release_img(stm_image_t *image);

///< @brief 配置参数，主要控制算法运行对修复操作的一些域值与强度参数
///< @param [in] handle 修复上下文句柄
///< @param [in] config_type 具体可配置的参数类型, 可参考stm_common.h文件中相关常量STM_CONFIG_*
///< 的定义
///< @param [in] config_val 具体配置的参数结构，该结构具体类型由config_type决定，
///< 可参考stm_common.h文件中相关常量STM_CONFIG_* 的定义
///< @return 配置成功返回STM_OK 失败返回错误码
PORTRAIT_REPAIR_API
stm_result_t stm_portrait_repair_config(const stm_handle_t handle, const int config_type,
                                        void *config_val);

///< @brief 通过json文件配置参数，主要控制算法运行对修复操作的一些域值与强度参数
///< @param [in] handle 修复上下文句柄
///< @param [in] json_file 配置文件路径
///< @return 配置成功返回STM_OK 失败返回错误码
PORTRAIT_REPAIR_API
stm_result_t stm_portrait_repair_config_json(const stm_handle_t handle, const char *json_file);

///< @brief
///< 取消算法处理，加快算法退出处理流程。该函数为阻塞性方法，须保证与process接口不同在同一线程调用
///< @param [in] handle 修复上下文句柄
PORTRAIT_REPAIR_API
void stm_portrait_repair_cancel(const stm_handle_t handle);

#endif // STM_PORTRAIT_REPAIR_API_H_
