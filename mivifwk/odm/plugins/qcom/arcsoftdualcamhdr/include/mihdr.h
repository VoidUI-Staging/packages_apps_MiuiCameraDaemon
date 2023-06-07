#ifndef _XMI_MI_HDR_H_
#define _XMI_MI_HDR_H_

#define MAX_FACE_NUM            16
#define MAX_FACE_BUF_SIZE       40
#define MAX_DSP_FD_NUM          20
#define MAX_CROP_PARAM_LEN      4
#define HDR_RAW_MAX_FACE_NUMBER 8

#include <inttypes.h>

#include <functional>
#include <string>
#include <vector>

#include "MiBuf.h"
#include "arcsoft_high_dynamic_range.h"
#include "arcsoft_low_light_hdr.h"
int MinVLogLevelFromEnv();

extern "C" int dohdr(const std::vector<char *> &bytestvect, int width, int height, char *dst222);

enum AE_STATE {
    AE_STATE_INACTIVE = 0,
    AE_STATE_SEARCHING = 1,
    AE_STATE_CONVERGED = 2,
    AE_STATE_LOCKED = 3,
    AE_STATE_MAX = 0xffffffff
};

enum HDR_SWITCH_STATE {
    // 0: OFF 1: ON 2: AUTO
    HDR_SWITCH_OFF = 0,
    HDR_SWITCH_ON = 1,
    HDR_SWITCH_AUTO = 2,
    HDR_SWITCH_MAX = 0xffffffff
};

enum HDR_TYPE {
    MIAI_HDR_NORMAL = 0,
    MIAI_HDR_LOWLIGHT = 1,
    MIAI_HDR_FLASH = 2,
    MIAI_HDR_BOKEH = 3,
    MIAI_HDR_SR = 4,
};

enum HDR_EV0_PREPROCESS { PREPROCESS_MFNR = 0, PREPROCESS_SR = 1 };

enum CAMERA_ID {
    SELFIE_CAMERA1 = 0,
    SELFIE_CAMERA2,
    REAR_CAMERA1 = 10,
    REAR_TELE,
    REAR_WIDE_2,
    REAR_EXTRA_WIDE,
    REAR_MACRO,
    REAR_WIDE_108M,
};

enum MIAI_HDR_FLASH_MODE {
    MIAI_HDR_FLASH_MODE_OFF = 0,
    MIAI_HDR_FLASH_MODE_ON = 1,
    MIAI_HDR_FLASH_MODE_TORCH = 2,
    MIAI_HDR_FLASH_MODE_AUTO = 3,
};

enum MIAI_HDR_ANDROID_FLASH_MODE {
    MIAI_HDR_ANDROID_FLASH_MODE_OFF = 0,
    MIAI_HDR_ANDROID_FLASH_MODE_SINGLE = 1,
    MIAI_HDR_ANDROID_FLASH_MODE_TORCH = 2,
};

enum MIAI_HDR_TAB_MODE {
    MIAI_HDR_TAB_MODE_NORMAL = 0,
    MIAI_HDR_TAB_MODE_PORTRAIT = 1,
};

// enum AlgoUpHDRSnapshotType
// {
//     // rear camera
//     NoneHDR = 0,           // non hdr
//     AlgoUpNormalHDR = 1,   // default algoup for hdr
//     AlgoUpSTGHDR = 2,      // stagger algoup for hdr
//     AlgoUpZSLHDR = 3,      // zsl algoup for hdr
//     AlgoUpFinalZSLHDR = 4, // all ev(zsl ev0, nonzsl ev-, nonzsl ev+)
//                            // frame continous algoup for hdr
//     AlgoUpFlashHDR = 5,
//     MFHDR = 6,      // 4EV for hdr
//     STGMFHDR = 7,   // stagger exp for hdr
//     ZSLMFHDR = 8,   // 4EV zero shutter lag for hdr
//     FlashMFHDR = 9, // 4EV FlashHDR

//     // front camera
//     AlgoUpFrontNormalHDR = 10,   // default algoup for hdr
//     AlgoUpFrontFinalZSLHDR = 11, // all ev(zsl ev0, nonzsl ev-, nonzsl ev+)
//                                  // frame continous algoup for hdr
//     MAX = 12,
// };

enum MIAI_HDR_ALGO_MODE { MIAI_UNDEFINED_HDR = 0, MIAI_RAW_HDR = 1, MIAI_YUV_HDR = 2 };

typedef struct _tag_MIAI_HDR_FACEINFO
{
    MInt32 nFace;
    MRECT rcFace[HDR_RAW_MAX_FACE_NUMBER];
} MIAI_HDR_FACEINFO, *LPMIAI_HDR_FACEINFO;

enum MIAI_HDR_ADDITIONAL_CAP_MODE {
    MIAI_HDR_NONE = 0,
    MIAI_HDR_MOTION_CAPTURE = 1 //运动抓拍
};

union HDRMetaData {
    char _[512];
    struct _config
    {
        char version;
        char camera_id;
        char hdr_type;
        char orientation;
        float magnet_x;
        float magnet_y;
        float magnet_z;
        long long int timestamp;
        bool isLock;
        bool last_lock;
        int isMFHDREVLocked;
        int hdr_switch_status; // 0: OFF 1: ON 2: AUTO
        float zoom;
        uint64_t frameNum;
        bool isTrigger;
        bool isCapture;
        bool isSwitchTab;
        char flash_mode;
        char is_flash_snapshot;
        int se_trigger;
        char tab_mode; // 0 normal,1 portrait
        int appHDRSnapshotType;
        int fd[MAX_DSP_FD_NUM]; // input: 3 output:1 // for cdsp
        uint32_t cropSize[MAX_CROP_PARAM_LEN];
        char ev0_preprocess;
        uint32_t hdrSnapShotStyle; // 0:生动  1:经典
        MIAI_HDR_FACEINFO face_info;
        uint32_t scene_det_type;
    } config;
    HDRMetaData()
    {
        config.version = 0;
        config.camera_id = REAR_CAMERA1;
        config.hdr_type = MIAI_HDR_NORMAL;
        config.magnet_x = 0;
        config.magnet_y = 0;
        config.magnet_z = 0;
        config.timestamp = 0;
        config.isLock = false;
        config.last_lock = false;
        config.isMFHDREVLocked = 0;
        config.hdr_switch_status = 0;
        config.isSwitchTab = 0;
        config.se_trigger = 0;
        config.tab_mode = MIAI_HDR_TAB_MODE_NORMAL;
        config.appHDRSnapshotType = 0;
        config.ev0_preprocess = PREPROCESS_MFNR;
        config.hdrSnapShotStyle = 0; // 0:生动  1:经典
    }
    bool is_selfie() const { return config.camera_id < 10; }

    std::string cameraId() const
    {
        switch (config.camera_id) {
        case SELFIE_CAMERA1:
            return "SELFIE_CAMERA1";
        case SELFIE_CAMERA2:
            return "SELFIE_CAMERA2";
        case REAR_CAMERA1:
            return "REAR_WIDE";
        case REAR_TELE:
            return "REAR_TELE";
        case REAR_WIDE_2:
            return "REAR_WIDE_2";
        case REAR_EXTRA_WIDE:
            return "REAR_EXTRA_WIDE";
        case REAR_MACRO:
            return "REAR_MACRO";
        case REAR_WIDE_108M:
            return "REAR_WIDE_108M";
        default:
            return "UNKNOWN_CAMERA_ID";
        }
    }
};

struct MIAIFaceinfo
{
    int leftX;       // face position to left
    int topY;        // face position to top
    int size;        // face size
    int confidence;  // confidence value
    int roll;        // roll
    int pitch;       // pitch
    int yaw;         // yaw
    int reserved[4]; // reserved
};

typedef struct
{
    float rot_x;             // rot_x Angular velocity around x-axis. [rad/sec]
    float rot_y;             // rot_y Angular velocity around y-axis. [rad/sec]
    float rot_z;             // rot_z Angular velocity around z-axis. [rad/sec]
    long long int timestamp; // Time stamps. [nsec]
    bool sensorStatus;       // sensor available or not
} miai_gyro_data;

typedef struct
{
    float gravity_x;         // gravity_x Acceleration of gravity along the x axis. [m/s^2]
    float gravity_y;         // gravity_y Acceleration of gravity along the y axis. [m/s^2]
    float gravity_z;         // gravity_z Acceleration of gravity along the z axis. [m/s^2]
    long long int timestamp; // Time stamps. [nsec]
    bool sensorStatus;       // sensor available or not
} miai_gravity_data;

typedef struct
{
    float acce_x;            // acce_x Acceleration force along the x axis. [m/s^2]
    float acce_y;            // acce_y Acceleration force along the y axis. [m/s^2]
    float acce_z;            // acce_z Acceleration force along the z axis. [m/s^2]
    long long int timestamp; // Time stamps. [nsec]
    bool sensorStatus;       // sensor available or not
} miai_acce_data;

typedef struct _MIAIHDR_INPUT
{
    int i32CamMode;
    long long i64ExpTime;
    int i32IsoValue;
    int i32FrameNum;
    int i32FaceNum;
    MIAIFaceinfo faceFocus;           // the biggest face rect. left top x y, w, h  focus face
    MIAIFaceinfo faces[MAX_FACE_NUM]; // array of face information  max num 255
    int i32AEState;
    int i32AFState;
    float AE_target;  // target ae
    float current_AE; // real AE
    float adrc_gain;  //
    float focus_distance;
    float lux_index;
    int arcsoftHdrFlag; // for backlight detection
    int touchRect[4];   // touch roi region
    int i32IsAiEnable;
    miai_gyro_data gyroData;
    miai_acce_data acceData;
    miai_gravity_data gravityData;
} MIAIHDR_INPUT, *LPMIAIHDR_INPUT;

struct GainInfo
{
    int colorTemperature;
    float rGain;
    float gGain;
    float bGain;
};

struct eye_info
{
    MInt8 eyenum;
    HDRFDPoint left[MAX_EYELAND_POINT];
    HDRFDPoint right[MAX_EYELAND_POINT];
};

struct HDR3AMeta
{
    int input_num;
    int current_num;
    HDRMetaData hdrMetaData;
};

class XMI_HDR_ALGO_PARAMS
{
public:
    // data
    MIAI_HDR_ALGO_PARAMS algo_params;

public:
    static XMI_HDR_ALGO_PARAMS &getInstance();

private:
    XMI_HDR_ALGO_PARAMS();
};

typedef long (*callback_fun)(long lProgress, long lStatus, void *pParam);
typedef std::function<void()> callback_ion;

int initMinight();

const char *getVcsVersion();

std::string getKernelFilePath();

int nightProcess(const void *inputs, int input_size, void *output, int rotate, int64_t timestamp,
                 const void *pAeInfo, const HDRMetaData &meta, callback_fun fun, void *pCallBackObj,
                 callback_ion ion_release);

int nightProcess_eyeinfo(const void *inputs, int input_size, void *output, int rotate,
                         int64_t timestamp, const void *pAeInfo, const HDRMetaData &meta,
                         LPARC_LLHDR_FACEINFO face_info, const eye_info eyes[], callback_fun fun,
                         void *pCallBackObj, callback_ion ion_release);

int nightPreviewProcess(const void *input, void *output, int rotate, int64_t timestamp,
                        const void *pAeInfo, void *pCheckInfo);

void dumpYuvData(const char *suffix, const unsigned char *img, int width, int height, int uv_offset,
                 int pitch);

void dumpYuvToJpg(const char *suffix, const unsigned char *img, int width, int height,
                  int uv_offset, int pitch);

void dumpYuvData(const char *suffix, const unsigned char *img, int width, int height, int uv_offset,
                 int pitch, const char *directory);

void dumpYuvData(const char *suffix, const unsigned char *img, const unsigned char *img_uv,
                 int width, int height, int pitch, const char *directory);

void dumpYuvToJpg(const char *suffix, const unsigned char *img, int width, int height,
                  int uv_offset, int pitch, const char *directory);

void drawTextInYuv(const char *text, unsigned char *img, int width, int height, int uv_offset,
                   int pitch, int fontscale = 4, int thickness = 8, int orient = 90);

void drawOrientTextInYuv(unsigned char *img, int width, int height, int uv_offset, int pitch,
                         int fontscale = 4, int thickness = 8, int orient = 90,
                         std::string cameraId = "");

void drawTextInRgb(const char *text, unsigned char *img, int width, int height, int type);

void readYuvData(const char *path, unsigned char *img, int width, int height);

int finalizeHDR();

void processRawData(/*mihdr::*/ MiImageBuffer *buffer, int ev, int frameNum, GainInfo *info,
                    HDRMetaData *hdrMetaData);

void processMIAI_HDRMetaData(int ev, void *aec, void *awb, void *data);

void remove_redeyes(void *input, void *output, const eye_info eyes[], int orientation);

std::string getTimeKey();
std::string gettimestamp();

#endif
