#ifndef MIAI_DEBUR_H_
#define MIAI_DEBUR_H_

#define MIAI_SR_DEBLUR_VERSION "MIAI_SR_DEBLUR_2021_1119_1110"
#define BRCKFlatSceneInfoSize 5
#define FACE_COUNT 10
namespace mia_deblur
{
    enum SceneType
    {
        SCENE_NONE = 0,
        SCENE_DOC = 1,
        SCENE_CHESS = 2,
        SCENE_NONCHESS = 3,
        SCENE_WOOD = 4,
        DEPURPLE = 5,
    };
    enum WorkStatus{
        NOT_WORK = -1,
        WORK_FAILED = 0,
        WORK_SUCCESS = 1,
    };
    enum SDInitMode{
        NOT_INIT = 0,
        INIT_NORMAL = 1,
        INIT_WARMUP = 2,
        INIT_WARMUP_MEMORY_SAVER = 3,
    };
    union MIAIDeblurParams {
        char _[32];
        struct _config
        {
            char orientation;   //0:0/360; 1:90; 2:180; 3:270
            char txt_detect;     //是否检测文字，默认不检测文字，可以通过：persist.vendor.camera.miaideblur.txt_detect 设置
            char disable_deblur; //是否调过算法，默认执行deblur算法，可以通过：persist.vendor.camera.miaideblur.disable_deblur 设置
            SceneType sceneType;
            void *flatSceneInfo;
            int tofDistance;
            int sceneflag;
            float sceneConfidence;
        } config;
        MIAIDeblurParams()
        {
            config.orientation = 0;
            config.txt_detect = 0;
            config.disable_deblur = 0;
            config.flatSceneInfo = nullptr;
            config.tofDistance = 0;
            config.sceneflag = 0;
            config.sceneConfidence = 0.0f;
        }
    };
    typedef struct
    {
        unsigned int  confidence[BRCKFlatSceneInfoSize]; ///< Confidence to all AF bracketing regions
        unsigned int  defocus[BRCKFlatSceneInfoSize];     ///< Defocus to all AF bracketing regions
        unsigned int  flag;                               ///< Flat scene information 0:unknown 1:flat 2:multi-depth
    } AFFlatSceneInfo;

    typedef struct
    {
        int startX;
        int startY;
        int width;
        int height;
    } FaceRect;

    typedef struct {
        int         faceNum;
        FaceRect    faceRectList[FACE_COUNT];
    } FaceInfo;

    bool deblur_init(const char **initParams, SceneType sceneType);
    bool deblur_init(const char **initParams, SceneType sceneType, bool initSceneDetect);
    /**
     * sdInitFlag: 0 不初始化sd 1:正常初始化sd 2:初始化sd完成后跑一次假数据...
     */
    bool deblur_init(const char **initParams, SceneType sceneType, SDInitMode sdInitMode);
    void setFaceInfoBeforeProcess(FaceInfo* faceInfo);
    int deblur_process(void *src, void *dst, MIAIDeblurParams *params);
    void deblur_release();
} // namespace mia_deblur
#endif // MIAI_DEBUR_H_
