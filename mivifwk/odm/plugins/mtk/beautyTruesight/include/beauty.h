#ifndef _BEAUTY_H_
#define _BEAUTY_H_

//#include <MiBuf.h>

#define FPC_FACE_MAX_ROI 10
#define ZOOM_WOI_COUNT   20

#define FD_Max_Landmark  118
#define FD_Max_FaceCount 10
#define FD_Max_FACount   3

#define MAX_NUMBER_OF_FACE_DETECT 3
#define MAX_NUMBER_OF_FACE_DETECT 3
#define MAX_FOREHEAD_POINT_NUM    40

#define NUMBER_OF_OUTPUT_PORTS    5
#define MAX_NUMBER_OF_FACE_DETECT 3

typedef struct
{
    // Detection frame coordinates, Upper left
    int roi_x0;
    int roi_y0;
    // lower right
    int roi_x1;
    int roi_y1;
} MotionMiVideoRect;

struct position
{
    float x;
    float y;
};

struct milandmarks
{
    struct position points[FD_Max_Landmark];
};

struct mouthmask
{
    unsigned char mouth_mask[64][64];
};

struct mouthmatrix
{
    float mouth_matrix[6];
};

struct milandmarksvis
{
    float value[FD_Max_Landmark];
};

struct faceroi
{
    int left;
    int top;
    int width;
    int height;
};

struct dimension
{
    unsigned int width;
    unsigned int height;
};

struct ForeheadPoints
{
    float points[80];
};

struct MiFDBeautyParam
{
    unsigned int miFaceCountNum;
    struct faceroi face_roi[FD_Max_FaceCount];
    struct milandmarks milandmarks[FD_Max_FACount];
    struct milandmarksvis milandmarksvis[FD_Max_FACount];
    struct mouthmask mouthmask[FD_Max_FACount];
    struct mouthmatrix mouthmatrix[FD_Max_FACount];
    struct dimension refdimension;
    struct ForeheadPoints foreheadPoints[FD_Max_FACount];
    int lmk_accuracy_type[FD_Max_FACount];
    float male_score[FD_Max_FACount];
    float female_score[FD_Max_FACount];
    float adult_score[FD_Max_FACount];
    float child_score[FD_Max_FACount];
    float age[FD_Max_FACount];
    int faceid[FD_Max_FaceCount];
    float rollangle[FD_Max_FaceCount];
    float confidence[FD_Max_FaceCount];
    int isMouthMaskEnabled;
    int isGenderEnabled;
    int isForeheadEnabled;
    int size_adjusted;
};

#if 0
struct FrameInfo
{
    int iso;                // iso when image captured, use default = 150 when unknown;
    int nLuxIndex;          // 室内室外Lux index参数  默认值 170(室外)
    int nFlashTag;            // 是否使用闪光灯, 0否, 1是, 2常亮
    int nSoftLightTag;        // 是否使用柔光环, 0否, 1是, 2常亮
    float fExposure;       // 曝光时间;  default = 0.0
    float fAdrc;          // adrc数值 正常范围为0.0-3.0
    int nColorTemperature;   // 色温, 正常数值范围0-10000

};
#endif
#endif
