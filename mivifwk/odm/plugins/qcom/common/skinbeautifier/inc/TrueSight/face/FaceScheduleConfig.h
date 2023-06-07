#pragma once

#include <cstdint>
#include <memory>

#include <TrueSight/TrueSightDefine.hpp>
#include <TrueSight/Common/IString.h>

namespace TrueSight {

/**
 * algorithm accuracy type
 *     ALG_ACC_TYPE_(NX)LARGE: N数值越大算法精度越高
 *     ALG_ACC_TYPE_(NX)LARGE: The larger the value of N, the higher the accuracy of the algorithm
 *     ALG_ACC_TYPE_(NX)SMALL: N数值越大算法精度越高
 *     ALG_ACC_TYPE_(NX)SMALL: The larger the value of N, the higher the precision of the algorithm
 *     ALG_ACC_TYPE_(NX)LARGE 精度 > ALG_ACC_TYPE_(NX)SMALL
 */
enum ALG_ACC_TYPE : int32_t {

    ALG_ACC_TYPE_NONE      = 0x0,  // turn off

    ALG_ACC_TYPE_LARGE     = 0x10, // L version
    ALG_ACC_TYPE_XLARGE    = 0x11, // XL version
    ALG_ACC_TYPE_2XLARGE   = 0x12, // 2XL version reserved
    ALG_ACC_TYPE_3XLARGE   = 0x13, // 3XL version reserved

    ALG_ACC_TYPE_SMALL     = 0x20, // S version
    ALG_ACC_TYPE_XSMALL    = 0x21, // XS version reserved
    ALG_ACC_TYPE_2XSMALL   = 0x22, // 2XS version reserved
    ALG_ACC_TYPE_3XSMALL   = 0x23, // 3XS version reserved

#pragma region // Face AE
    ALG_ACC_TYPE_LARGE_FAE   = 0x30, // L version for FaceAE
    //ALG_ACC_TYPE_XLARGE_FAE  = 0x31, // XL version for FaceAE reserved
    //ALG_ACC_TYPE_2XLARGE_FAE = 0x31, // 2XL version for FaceAE reserved
    //ALG_ACC_TYPE_3XLARGE_FAE = 0x31, // 3XL version for FaceAE reserved

    ALG_ACC_TYPE_SMALL_FAE   = 0x40, // S version for FaceAE
    //ALG_ACC_TYPE_XSMALL_FAE  = 0x41, // XS version for FaceAE reserved
    //ALG_ACC_TYPE_2XSMALL_FAE = 0x42, // 2XS version for FaceAE reserved
    //ALG_ACC_TYPE_3XSMALL_FAE = 0x43, // 3XS version for FaceAE reserved
#pragma endregion
};

enum FACE_ROLL_TYPE : int32_t {
    FACE_ROLL_TYPE_NONE    = 0x0, // 关闭人脸框角度检测算法
    FACE_ROLL_TYPE_S       = 0x1, // 打开人脸框角度检测Small算法
    FACE_ROLL_TYPE_L       = 0x2, // 打开人脸框角度检测Large算法
    /*FACE_ROLL_TYPE_NONE: Turn off the face frame angle detection algorithm
    FACE_ROLL_TYPE_S : Open face frame angle detection Small algorithm
    FACE_ROLL_TYPE_L : Open face frame angle detection Large algorithm */
};

enum FACEBOX_ADJUST_TYPE : int32_t  {
    FACEBOX_ADJUST_TYPE_NONE = 0x0, // 关闭人脸框矫正算法
    FACEBOX_ADJUST_TYPE_L = 0x2,    // 打开人脸框矫正算法Large version
    /* FACEBOX_ADJUST_TYPE_NONE : Disable face frame correction algorithm
    FACEBOX_ADJUST_TYPE_L : Open face frame correction algorithm Large algorithm */
};

/**
 * 人脸检测调度配置
 * Face Detection Scheduling Configuration
 */
class FaceScheduleConfigInner;
class TrueSight_PUBLIC FaceScheduleConfig {

public:
    FaceScheduleConfig();
    FaceScheduleConfig(const FaceScheduleConfig &rhs);
    FaceScheduleConfig(FaceScheduleConfig &&rhs) noexcept;
    ~FaceScheduleConfig();

    FaceScheduleConfig& operator=(const FaceScheduleConfig &rhs);
    FaceScheduleConfig& operator=(FaceScheduleConfig &&rhs) noexcept;

public:
    /**
     * 设置资源文件路径
     * Set resource file path
     * @param path
     * @return
     */
    FaceScheduleConfig& setResourcePath(const char *path);
    FaceScheduleConfig& getResourcePath(IString &path);

    /**
     * 设置是否开启低精度计算(armv8.2 fp16), 使用低精度性能更优
     * Set whether to enable low-precision computing (armv8.2 fp16), use low-precision for better performance
     * @param flag
     * @return
     */
    FaceScheduleConfig& setEnablePrecisionLow(bool flag);
    FaceScheduleConfig& getEnablePrecisionLow(bool &flag);

public: // face detection config
    /**
     * 设置是否开启内部人脸框检测
     * Set and get "whether to enable internal face bounding box detection".
     * @param flag
     * @return
     */
    FaceScheduleConfig& setEnableInnerFD(bool flag);
    FaceScheduleConfig& getEnableInnerFD(bool &flag);

    /**
     * 设置可检测人的脸数
     * Set and get "the number of detectable faces". Default detectable face is 10. If you want to change the number, please make sure it is larger than 0.
     */
    FaceScheduleConfig& setMaxFaceCount(int num);
    FaceScheduleConfig& getMaxFaceCount(int &num);

    /**
     * 设置可跟踪的人脸数
     * Set and get "the number of face alignment". Default detectable face is 10. If you want to change the number, please make sure it is larger than 0.
     */
    FaceScheduleConfig& setMaxFaceAlignmentCount(int num);
    FaceScheduleConfig& getMaxFaceAlignmentCount(int &num);

    /**
     * 设置支持的最小人脸比例 : ratio = (float)MinFace / (float)std::min(image_width, image_height)
     * Set or get the minimum supported face ratio: ratio = (float)MinFace / (float)std::min(image_width, image_height).
     */
    FaceScheduleConfig& setMinFaceRatio(float ratio);
    FaceScheduleConfig& getMinFaceRatio(float &ratio);

    /**
     * 设置人脸检测实时跟踪场景间隔多少帧执行一次
     * Set or get "how often does it execute face detection in real-time tracking". It defaults to execute once every 7 frames.
     */
    FaceScheduleConfig& setFDIntervalFrames(int interval_frames);
    FaceScheduleConfig& getFDIntervalFrames(int &interval_frames);

    /**
     * 无人脸情况下间隔帧的加速倍率
     *  实际速度 = int(interval_frames / ratio)
     *  Set or get the acceleration ratio of interval frames in the case of no face: actual speed = int(interval_frames / ratio).
     */
    FaceScheduleConfig& setFDIntervalFramesSpeedupRatio(float ratio);
    FaceScheduleConfig& getFDIntervalFramesSpeedupRatio(float &ratio);

    /**
     * 设置人脸角度检测算法, 若外部可提供正常的人脸角度则无需设置
     * Set or get the algorithm of face angle detection . You don't have to perform this step if you can provide accurate face Angle values
     * @param type
     *     FACE_ROLL_TYPE_NONE : 关闭人脸角度检测算法
     *     FACE_ROLL_TYPE_S    : 打开人脸框角度检测Small算法
     *     FACE_ROLL_TYPE_L    : 打开人脸框角度检测Large算法
     * @param isAsync -- 仅FaceTracker有效
     *     false : 在异步线程执行
     *     ture  : 串行执行
     * @param isAsync Only FaceTracker is valid; false means executed in an asynchronous thread; true means executed serially.
     * @return
     */
    FaceScheduleConfig& setFaceRollType(FACE_ROLL_TYPE type, bool isAsync = false);
    FaceScheduleConfig& getFaceRollType(FACE_ROLL_TYPE &type, bool &isAsync);

    /**
     * 设置人脸框矫正算法, 若外部可提供精确的人脸框则无需此设置
     *     !!! 若设置了此选项则setFaceRollType不生效
     * Set or get the face bounding box correction algorithm. You don't have to perform this step if you can provide an accurate face bounding box.
     * @param type
     *     FACEBOX_ADJUST_TYPE_NONE : 关闭人脸角度检测算法
     *     FACEBOX_ADJUST_TYPE_L    : 打开人脸框角度检测Large算法
     * @return
     */
    FaceScheduleConfig& setFaceboxAdjustType(FACEBOX_ADJUST_TYPE type);
    FaceScheduleConfig& getFaceboxAdjustType(FACEBOX_ADJUST_TYPE &type);

public: // face alignment config
    /**
     * 设置是否开启嘴巴Mask
     * Set or get whether to enable the mouth mask, the default flag is false.
     * @param flag
     * @return
     */
    FaceScheduleConfig& setEnableMouthMask(bool flag);
    FaceScheduleConfig& getEnableMouthMask(bool &flag);

    /**
     * 设置是否开启嘴巴Mask : 若开启此选项则默认setEnableMouthRobost(true)
     * Set or get whether to enable mouth landmark Mask. If you choose to enable, the default flag is setEnableMouthMask(true).
     * @param flag
     * @return
     */
    FaceScheduleConfig& setEnableMouthRobost(bool flag);
    FaceScheduleConfig& getEnableMouthRobost(bool &flag);

    /**
     * 设置是否增加眼睛点精度
     * Set or get whether to increase eye landmark precision.
     * @param flag
     * @return
     */
    FaceScheduleConfig& setEnableEyeRobost(bool flag);
    FaceScheduleConfig& getEnableEyeRobost(bool &flag);

    /**
     * 设置是否开启额头点检测
     * Set or get whether to enable forehead point detection
     * @param flag
     * @return
     */
    FaceScheduleConfig& setEnableForehead(bool flag);
    FaceScheduleConfig& getEnableForehead(bool &flag);

    /**
     * \~chinese 设置是否开启人脸解析
     * \~english Set or get whether to enable face parsing
     * @param flag
     * @return
     */
    FaceScheduleConfig& setEnableFaceParsing(bool flag);
    FaceScheduleConfig& getEnableFaceParsing(bool &flag);

    /**
     * 设置人脸点算法精度
     * Set or get the algorithm precision of face points.
     * @param type
     * @return
     */
    FaceScheduleConfig& setFaceLandmarkType(ALG_ACC_TYPE type);
    FaceScheduleConfig& getFaceLandmarkType(ALG_ACC_TYPE &type);

    /**
     * 设置是否使用NPU
     *      Note: NPU下setFaceLandmarkType不会产生效果
     * @param flag
     * @return
     */
    FaceScheduleConfig& setEnableNPU(bool flag);
    FaceScheduleConfig& getEnableNPU(bool &flag);

    /**
     * 内部使用的线程数
     * Set or get the number of threads used internally.
     * 0: 自动控制
     * 0 means Automatic control
     * 1~N: 允许使用的最多线程数
     * 1~N means the maximum number of threads allowed to use
     */
    FaceScheduleConfig& setNumThread(int num);
    FaceScheduleConfig& getNumThread(int &num);

    /**
     * 内部使用多线程的策略
     * Set or get whether to use an independent thread for internal face detection. Disable by default.
     * @param mode 0: 单人脸单线程, 1: 单人脸多线程
     * @param mode 0: Single-face single thread; 1: Single-face multi-threading. Default is 0.
     * @return
     */
    FaceScheduleConfig& setModeThread(int mode);
    FaceScheduleConfig& getModeThread(int &mode);

    /**
     * 内部是否使用独立的线程来运算
     * Set whether to use an independent thread for operation internally when setting/getting face detection, the default is no.
     * @param flag true: detect时将使用另外线程池的线程来执行, false: detect时直接在所在的调用线程执行
     * @return
     */
    FaceScheduleConfig& setEnableStandaloneThread(bool flag);

    /**
     * 内部是否使用独立的线程来运算
     * Get whether to use an independent thread for operation internally when setting/getting face detection, the default is no.
     * @param flag
     * @return
     */
    FaceScheduleConfig& getEnableStandaloneThread(bool &flag);

    /**
     * 人脸点使用策略, 索引顺序与人脸顺序一致, 每个位置代表第几个人脸使用的策略
     * Set or get the mask policy for opening the mouth, the index order is the same as the face order, and each position represents the policy used by the first face.
     *    1. 使用时总的数量不会超过maxFaceCount
     *    1. When used, the total number of the strategy will not exceed maxFaceCount;
     *    2. 使用时总的数量超过设置数量, 则未设置策略的人脸按照最后一个策略执行
     *    2. If the total number of faces exceeds the number of strategies faces without strategies will be executed according to the last strategy;
     *    3. 若无设置或者设置大小为0则默认全部使用faceLandmarkType作为策略
     *    3. If you don't perform this step or the set number is 0, faceLandmarkType will be applied to all faces by default;
     */
    FaceScheduleConfig& setFaceLandmarkTypeStrategy(const IVector<int> &strategy);
    FaceScheduleConfig& getFaceLandmarkTypeStrategy(IVector<int> &strategy);
    const IVector<int> *getFaceLandmarkTypeStrategy();

    /**
     * 开启嘴巴Mask策略, 索引顺序与人脸顺序一致, 每个位置代表第几个人脸使用的策略
     * Set or get the strategy to enable the mouth mask, the index order is the same as the face order.
     *    0. 0: 关闭, 1: 开启
     *    0. 0 means off; 1 means on.
     *    1. 生效条件为enableMouthMask == true, 总的数量不会超过maxFaceCount
     *    1. The premise is "enableMouthMask == true", and the total number of the strategy will not exceed maxFaceCount
     *    2. 使用时总的数量超过设置数量, 则未设置策略的人脸按照最后一个策略执行
     *    2. If the total number of faces exceeds the number of use strategies, faces without strategies will be executed according to the last strategy;
     *    3. 若无设置或者设置大小为0则默认全开
     *    3. If you don't perform this step or the set number is 0, the mouth mask will be enabled by default.
     */
    FaceScheduleConfig& setMouthMaskStrategy(const IVector<int> &strategy);
    FaceScheduleConfig& getMouthMaskStrategy(IVector<int> &strategy);
    const IVector<int> *getMouthMaskStrategy();

    /**
     * 开启眼睛高精度模型策略, 索引顺序与人脸顺序一致, 每个位置代表第几个人脸使用的策略
     * Set and get the strategy to enable the high-precision mode of eyes. The index order is the same as the face order.
     *    0. 0: 关闭, 1: 开启
     *    0. 0 means off; 1 means on.
     *    1. 生效条件为enableMouthMask == true, 总的数量不会超过maxFaceCount
     *    1. The premise is "enableMouthMask == true", and the total number of the strategy will not exceed maxFaceCount
     *    2. 使用时总的数量超过设置数量, 则未设置策略的人脸按照最后一个策略执行
     *    2. If the total number of faces exceeds the number of use strategies, faces without strategies will be executed according to the last strategy;
     *    3. 若无设置或者设置大小为0则默认全开
     *    3. If you don't perform this step or the set number is 0, the mouth mask will be enabled by default.
     */
    FaceScheduleConfig& setEyeRobostStrategy(const IVector<int> &strategy);
    FaceScheduleConfig& getEyeRobostStrategy(IVector<int> &strategy);
    const IVector<int> *getEyeRobostStrategy();

    /**
     * 设置判定为非人脸的置信度边界, 非精细调试不建议设置!!!
     * Set or get the confidence threshold for all faces.
     *     A. 图像模式
     *        val = 0 : 所有人脸框均会有人脸点
     *        val > 0 : 当算法输出的人脸点置信度大于val则正常输出人脸, 否则此人脸会被剔除
     *     A. Image mode
     *        If val = 0, all face bounding boxes will be presented with face landmarks;
     *        If val > 0 (it means the confidence number of a face's landmarks output by the algorithm is larger than threshold val), the face will be output normally, otherwise the face will be removed.
     *     B. 视频模式
     *        val = 0 : 第一帧输入的人脸框均会有人脸点, 跟踪帧按原有逻辑进行剔除非人脸不受此参数影响(但是会受到第一帧结果的影响)
     *            如果输入的外部人脸框roll较为不准可降低此值(不再使用此方式, 请更换为setFaceRollType增加一定的计算量来完成角度的矫正)
     *            警告: 当外部人脸框输入过于不准时, 一些较难场景会使得出现连续性的错误
     *        val > 0 : 当算法输出的人脸点置信度大于val则正常输出人脸, 否则此人脸会被剔除
     *     B. Real-time Preview
     *     If val = 0:
     *         In the first frame, face landmarks will be presented in the face bounding box. As for tracking sequences, faces will be removed or retained accordingly. Non-face will not be affected by this parameter (but will be affected by the result of the first frame);
     *         If the input roll of the face bounding box is inaccurate, this value can be reduced (this method is no longer used, please replace it with setFaceRollType to increase a certain amount of calculation to complete the angle correction);
     *         **Warning**： If the face bounding box input is too inaccurate, you may meet continuous errors!
     *     If val > 0 (it means the confidence number of a face's landmarks output by the algorithm is larger than threshold val), the face will be output normally, otherwise the face will be removed.
     * @param val
     * @return
     */
    FaceScheduleConfig& setFaceConfidenceKillThreshold(float val);
    FaceScheduleConfig& getFaceConfidenceKillThreshold(float &val);

public: // Face Attributes
    /**
     * 设置是否开启性别检测
     * Set or get whether to enable gender detection. Disable by default.
     * @param flag
     * @return
     */
    FaceScheduleConfig& setEnableGender(bool flag);
    FaceScheduleConfig& getEnableGender(bool &flag);

    /**
     * 设置是否开启年龄二分类检测
     * Set or get whether to enable age classification. Disable by default.
     * @param flag
     * @return
     */
    FaceScheduleConfig& setEnableAgeDivide(bool flag);
    FaceScheduleConfig& getEnableAgeDivide(bool &flag);

    /**
     * 设置是否开启年龄检测
     * Set or get whether to enable age detection. Disable by default.
     * @param flag
     * @return
     */
    FaceScheduleConfig& setEnableAge(bool flag);
    FaceScheduleConfig& getEnableAge(bool &flag);

private:
    void copy(const FaceScheduleConfig &rhs);
    void move(FaceScheduleConfig &&rhs);

private:
    FaceScheduleConfigInner *inner_ = nullptr;
    friend class FaceTrackerImpl;
    friend class FaceDetectorImpl;
    friend class FaceFeaturesInner;
    friend class FaceFramesUtils;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * 人脸检测调度默认配置, 只读
 */
class FaceScheduleConfigInner {

    friend class FaceTrackerImpl;
    friend class FaceDetectorImpl;
    friend class FaceFeaturesInner;
    friend class FaceScheduleConfig;
    friend class FaceFramesUtils;
private:
    FaceScheduleConfigInner();
    ~FaceScheduleConfigInner();

private:
    IString resourcePath_;         // 资源文件路径
    bool enablePrecisionLow = true; // 是否允许使用低精度计算(armv8.2 fp16), 使用低精度性能更优

private:
    bool enableInnerFD_ = false;   // 是否开启内部人脸框检测

    int maxFaceCount_ = 10;        // 可检测人的脸数
    int maxFaceAlignmentCount_ = 10;        // 可跟踪人的脸数
    float minFaceRatio_ = 0.05f;   // 检测的最小人脸比例, default : 24.0f / 480.0f = 0.05f
    int facedetection_interval_frames_ = 7; // 实时模式下, 人脸检测执行的间隔帧数
    float facedetection_interval_frames_speedup_ratio_ = 1.5f; // 实时模式下, 人脸检测执行的间隔帧数在无人脸下的加速倍率

    FACE_ROLL_TYPE faceRollType = FACE_ROLL_TYPE_NONE; // face roll algorithm mode
    bool faceRollAsync = false; // 是否异步执行人脸角度算法
    FACEBOX_ADJUST_TYPE faceboxAdjustType_ = FACEBOX_ADJUST_TYPE_NONE; // face box adjust algorithm mode

private:
    bool enableMouthMask_ = false;      // 是否开启嘴巴Mask
    bool enableMouthRobost_ = false;    // 是否增加嘴巴点精度
    bool enableEyeRobost_ = false;      // 是否增加眼睛点精度
    bool enableForehead_ = false;       // 是否开启额头点
    bool enableFaceParsing_ = false;    // 是否开启面部解析分割
    ALG_ACC_TYPE faceLandmarkType_ = ALG_ACC_TYPE_NONE; // face landmark algorithm mode

    bool enableNPU_ = false;       // 是否开启NPU
    int numThread_ = 0;            // 线程数
    int modeThread_ = 0;           // 线程模式
    bool enableStandaloneThread_ = false; // detect时是否使用独立的线程

    bool enableFaceLandmarkTypeStrategy_ = false;
    IVector<int> faceLandmarkTypeStrategy_; // 各人脸人脸点算法策略

    bool enableMouthMaskStrategy_ = false;
    IVector<int> mouthMaskStrategy_; // 各人脸人嘴巴Mask策略

    bool enableEyeRobostStrategy_ = false;
    IVector<int> eyeRobostStrategy_; // 各人脸眼睛点增强策略

    float faceConfidenceKillThreshold_ = 0.5f;

private: // Face Attributes
    bool enableGender_ = false;      // 是否开启性别检测
    bool enableAgeDivide_ = false;   // 是否开启年龄二分类检测
    bool enableAge_ = false;         // 是否开启年龄检测

public:
    // for test
    int serialize(const char* filename);
    int deserialize(const char* filename);
    void log_output(const char* tag = nullptr);
};

} // namespace TrueSight