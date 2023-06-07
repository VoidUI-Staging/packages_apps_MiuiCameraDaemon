#pragma once
#include <cstddef>
#include <cstdint>

#include <TrueSight/TrueSightDefine.hpp>
#include <TrueSight/Common/Types.h>
#include <TrueSight/Common/Image.h>
#include <TrueSight/Common/IVector.h>
#include <TrueSight/face/FaceScheduleConfig.h>

namespace TrueSight {

/**
 * Face parsing分类对应getFaceParsing返回结果中的下标索引
 */
enum class FaceParsingClassIndex {
    kBackground = 0,
    kSkin,
    kEyebrow,
    kEye,
    kLip,
    kInnerMouth,
    kHair,
    kFaceParsingNumClasses
};

class FaceFeaturesInner;
class TrueSight_PUBLIC FaceFeatures {
public:
    FaceFeatures();
    ~FaceFeatures();

    FaceFeatures(const FaceFeatures &rhs);
    FaceFeatures& operator=(const FaceFeatures &rhs);

public:
    /**
     * 人脸数量
     * Get the number of faces.
     * @return
     */
    size_t getFaceCount();

    /**
     * 重置人脸数量结构
     * Reset the number of faces.
     * @param count
     */
    void setFaceCount(size_t count);

    /**
     * 获取人脸ID
     *   FaceID为简单的跟踪编号, 并非人脸唯一标识
     *   在人脸Tracking丢失后重新进入其FaceID会不一致
     *   Get face ID. FaceID is a simple tracking number, not a unique face identification. Face ID will be different every time you enter.
     * @param index
     * @return 错误返回-1
     * @return Returns FaceID on success, and -1 on failure.
     */
    int getFaceID(size_t index);

    /**
     * 设置人脸ID
     *   FaceID为简单的跟踪编号, 并非人脸唯一标识
     *   在人脸Tracking丢失后重新进入其FaceID会不一致
     *   Set face ID. FaceID is a simple tracking number, not a unique face identification. Face ID will be different every time you enter.
     * @param faceID
     */
    void setFaceID(size_t index, int faceID);

    /**
     * 查找FaceID的索引
     *   FaceID为简单的跟踪编号, 并非人脸唯一标识
     *   在人脸Tracking丢失后重新进入其FaceID会不一致
     *   Find the face index by FaceID, and return szie_t(-1) if not found.
     * @param faceID
     * @return 找到则返回FaceID所在的index，没找到则返回size_t(-1)
     */
    size_t getIndexByFaceID(int faceID);

    /**
     * 获取人脸置信度
     * Get face confidence. When using an external face bounding box, you need to set the face confidence.
     * @param index
     * @return
     */
    float getFaceConfidence(size_t index);

    /**
     * 设置人脸置信度: 当使用外部人脸框时等场景使用
     * Set face confidence. When using an external face bounding box, you need to set the face confidence.
     * @param index
     * @param confidence
     */
    void setFaceConfidence(size_t index, float confidence);

    /**
     * 获取某个人脸的人脸框
     * Get the face bounding box.
     * @param index
     * @return
     */
    Rect2f getFaceBox(size_t index);

    /**
     * 设置某个人脸的人脸框
     * Set the face bounding box.
     * @param index
     * @param box
     */
    void setFaceBox(size_t index, Rect2f &box);

    /**
     * 获取人脸旋转弧度
     * Get the face rotation radian.
     * @param index
     * @return
     */
    float getRollRadian(size_t index);

    /**
     * 设置人脸旋转弧度
     * Set the face rotation radian.
     * @param index
     * @param roll
     */
    void setRollRadian(size_t index, float roll);

    /**
     * 人脸点
     * Get face landmarks.
     * @param index
     * @return
     */
    const IVector<Point2f>* getFaceLandmark2D(size_t index);

    /**
     * 设置人脸点
     * Set face landmarks.
     * @param index
     * @param lmk
     * @return
     */
    void setFaceLandmark2D(size_t index, const IVector<Point2f>* lmk);

    /**
     * 每个人脸点遮挡信息, FaceAE系列算法不支持人脸点遮挡(如ALG_ACC_TYPE_SAMLL_FAE)
     * Get the information of face landmark occlusion.
     * @param index
     * @return
     */
    const IVector<float>* getFaceLandmark2DVisable(size_t index);

    /**
     * 设置每个人脸点遮挡信息
     * Set the information of face landmark occlusion.
     * @param index
     * @param visable
     * @return
     */
    void setFaceLandmark2DVisable(size_t index, const IVector<float>* visable);

    /**
     * 获取额头点
     * Get forehead points
     * @param index
     * @param num_points，通常为40个(x,y)
     * @return
     */
    const float* getForeheadLandmark2D(size_t index, size_t &num_points);
    /**
     * 设置额头点
     * Set forehead points
     * @param index
     * @param lmk
     * @param num_points
     */
    void setForeheadLandmark2D(size_t index, const float* lmk, size_t num_points);

    /**
     * 嘴巴mask
     * Get mouth mask
     * @param index
     * @return
     */
    const Image* getMouthMask(size_t index, float mouth_matrix[6]);

    /**
     * 设置嘴巴mask
     * Set mouth mask
     * @param index
     * @param mask
     * @param mouth_matrix
     * @return
     */
    void setMouthMask(size_t index, const Image* mask, float mouth_matrix[6]);

    /**
     * \~chinese FaceParsing
     * \~english Get face parsing
     * @param index
     * @param face_parsing_matrix
     * @return
     */
    const IVector<Image>* getFaceParsing(size_t index, float face_parsing_matrix[6]);

    /**
     * \~chinese 设置FaceParsing
     * \~english Set face parsing
     * @param index
     * @param faceParsing
     * @param face_parsing_matrix
     */
    void setFaceParsing(size_t index, const IVector<Image>* faceParsing, float face_parsing_matrix[6]);

    /**
     * 获取对应人脸计算使用的算法精度类型
     *     仅当设置FaceScheduleConfig::setFaceLandmarkTypeStrategy时会存在不同人脸的差异
     * Get the algorithm precision type used for face calculation. See ALG_ACC_TYPE for arithmetic precision type
     * @param index
     * @return
     */
    ALG_ACC_TYPE getFaceLandmarkAccuracyType(size_t index);

    /**
     * 设置对应人脸计算使用的算法精度类型
     *     仅当设置FaceScheduleConfig::setFaceLandmarkTypeStrategy时会存在不同人脸的差异
     * Set the algorithm precision type used for face calculation. See ALG_ACC_TYPE for arithmetic precision type
     * @param index
     * @param type
     */
    void setFaceLandmarkAccuracyType(size_t index, ALG_ACC_TYPE type);

public: // Face Attributes
    /**
     * 获取性别
     * Get gender:
     * @param index : success 0, failed -1
     * @param maleScore 男性分数
     * @param femaleScore 女性分数
     */
    int getGender(size_t index, float &maleScore, float &femaleScore);

    /**
     * 设置性别
     * Set gender:
     * @param index : success 0, failed -1
     * @param maleScore 男性分数
     * @param femaleScore 女性分数
     */
    int setGender(size_t index, float maleScore, float femaleScore);

    /**
     * 获取年龄分类
     * Get age classification.
     * @param index : success 0, failed -1
     * @param adultScore 大人分数
     * @param adultScore adultScore is a probability score of a face is an adult's face which is determined by index.
     * @param childScore 儿童分数
     * @param childScore childScore is a probability score of a face is a kid's face which is determined by index.
     */
    int getAgeDivide(size_t index, float &adultScore, float &childScore);

    /**
     * 设置年龄分类
     * set age classification:
     * @param index : success 0, failed -1
     * @param adultScore 大人分数
     * @param adultScore adultScore is a probability score of a face is an adult's face which is determined by index.
     * @param childScore 儿童分数
     * @param childScore childScore is a probability score of a face is a kid's face which is determined by index.
     */
    int setAgeDivide(size_t index, float adultScore, float childScore);

    /**
     * 获取年龄 TODO: 当前功能不存在, 仅预留接口
     * Get age. reserved interface.
     * @param index
     * @return 年龄
     */
    float getAge(size_t index);

    /**
     * 设置年龄 TODO: 当前功能不存在, 仅预留接口
     * Set age. reserved interface.
     * @param index  人脸索引
     * @param age    年龄
     * @return
     */
    int setAge(size_t index, float age);

public:
    /**
     * 设置/获取生成当前人脸信息所使用的图像宽,高,旋转方向
     *  -- 当FaceFeatures不是源于检测器时需要设置, 例如从其他地方获取人脸信息并设置进来时
     *  Get or set the image width, height and rotation direction used to generate the information of current face.
     * @param width
     * @param height
     * @param orientation
     * @return
     */
    void setDetectionShape(int width, int height, int orientation);
    void getDetectionShape(int &width, int &height, int &orientation);

    /**
     * 对FaceFeatures进行8个方向的变换
     *  -- 当src FaceFeatures的数值是外部设置数值时需要通过setDetectionShape预先设置宽, 高, orientation
     *  Transform FaceFeatures in 8 directions.
     * @param src_facefeatures
     * @param dst_facefeatures
     * @param dst_orientation
     * @return
     */
    static int rotate(const FaceFeatures &src_facefeatures, FaceFeatures &dst_facefeatures, int dst_orientation);

    /**
     * 对FaceFeatures进行缩放
     *  -- 当src FaceFeatures的数值是外部设置数值时需要通过setDetectionShape预先设置宽, 高, orientation
     *  Zoom FaceFeatures.
     * @param src_facefeatures
     * @param dst_facefeatures
     * @param src_width
     * @param src_height
     * @param dst_width
     * @param dst_height
     * @return
     */
    static int resize(const FaceFeatures &src_facefeatures, FaceFeatures &dst_facefeatures, int src_width, int src_height, int dst_width, int dst_height);

    /**
     * 对FaceFeatures进行warp, 支持旋转, 缩放, 平移
     *  -- warp后需要重新使用setDetectionShape设置宽, 高, orientation; 若orientation无法知晓可通过getDetectionShape获得
     * Wrap FaceFeatures, support rotation, zoom, and translation.
     * @param src_facefeatures
     * @param dst_facefeatures
     * @param matrix
     * @return
     */
    static int warp(const FaceFeatures &src_facefeatures, FaceFeatures &dst_facefeatures, float matrix[6]);

public:
    // for test
    int serialize(const char* filename);
    int deserialize(const char* filename);

private:
    friend class FaceDetectorImpl;
    friend class FaceTrackerImpl;
    friend class XRealityPreviewImpl;
    friend class XRealityImageImpl;
    friend class LipsMaskImpl;
    friend class XRealityExt;
    /**
     * TrueSight私有数据请勿修改
     */
    FaceFeaturesInner *inner_ = nullptr;
};

} // namespace TrueSight