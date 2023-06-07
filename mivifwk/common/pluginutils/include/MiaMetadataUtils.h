#ifndef _METADATAUTILS_H_
#define _METADATAUTILS_H_

#include <sys/types.h>

namespace mialgo2 {
/// @brief Vendor Tags
struct VendorTagInfo
{
    const char *pTagName; ///< Vendor Tag Name
    uint32_t tagId;       ///< Vendor Tag Id used to query
};

/// @brief Structure containing width and height integer values, along with a start offset
typedef struct MiRect
{
    uint32_t left;   ///< x coordinate for top-left point
    uint32_t top;    ///< y coordinate for top-left point
    uint32_t width;  ///< width
    uint32_t height; ///< height
} MIRECT;

typedef struct
{
    int32_t horizonalShift; ///< x cordinate of the pixel
    int32_t verticalShift;  ///< y cordinate of the pixel
} MIImageShiftData;

typedef struct
{
    MIRECT cropRegion;
    float userZoomRatio;
    MIImageShiftData shiftVector;
} MICropInfo;

/// @brief Structure similar to ChiRect, but point's coordinate can be negative
typedef struct ChiRectINT
{
    int32_t left;    ///< x coordinate for top-left point
    int32_t top;     ///< y coordinate for top-left point
    uint32_t width;  ///< width
    uint32_t height; ///< height
} MIRECTINT;

/// @brief Structure containing width and height integer values
typedef struct MiDimension
{
    uint32_t width;  ///< width
    uint32_t height; ///< height
} MIDIMENSION;

typedef struct
{
    int32_t xMin;   ///< Top-left x-coordinate of the region
    int32_t yMin;   ///< Top-left y-coordinate of the region
    int32_t xMax;   ///< Width of the region
    int32_t yMax;   ///< Height of the region
    int32_t weight; ///< Weight of the region
} WeightedRegion;

typedef struct
{
    int32_t cameraId;
    int32_t orientation;
    float gender;
    int32_t isBeautyIntelligent;
    int32_t beautyLevel;
    int32_t beautySlimFaceRatio;
    int32_t beautyEnlargeEyeRatio;
    int32_t beautyBrightRatio;
    int32_t beautySoftenRatio;
    int32_t beautyNose3DRatio;
    int32_t beautyRisorius3DRatio;
    int32_t beautyLips3DRatio;
    int32_t beautyChins3DRatio;
    int32_t beautyNeck3DRatio;
} InputMetadataBeauty;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Constant Declarations
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#if defined(UMI_CAM)
/// align with sensor_id, should be modify
enum XiaoMiSensorPosition {
    SensorIdRearMacro2x = 0,
    SensorIdRearWide = 1,
    SensorIdFront = 2,
    SensorIdRearUltra = 3,
    SensorIdRearDepth = 4,
    ///////bellow placeholder
    SensorIdRearTele = 0xF0,
    SensorIdRearTele4x = 0xF1,
    SensorIdRearMacro = 0xF2,
    SensorIdFrontAux = 0xF3,
};

#elif defined(CMI_CAM)
/// align with sensor_id, should be modify
enum XiaoMiSensorPosition {
    SensorIdRearWide = 0,
    SensorIdRearUltra = 1,
    SensorIdFront = 2,
    SensorIdRearMacro2x = 3,
    SensorIdRearTele4x = 4,
    ///////bellow placeholder
    SensorIdRearDepth = 0xF0,
    SensorIdRearTele = 0xF1,
    SensorIdRearMacro = 0xF2,
    SensorIdFrontAux = 0xF3,
};

#elif defined(LMI_CAM)
/// align with sensor_id, should be modify
enum XiaoMiSensorPosition {
    SensorIdRearWide = 0,
    SensorIdRearTele = 1,
    SensorIdFront = 2,
    SensorIdRearUltra = 3,
    SensorIdRearDepth = 4,
    SensorIdRearMacro = 5,
    ///////bellow placeholder
    SensorIdRearTele4x = 0xF0,
    SensorIdRearMacro2x = 0xF1,
    SensorIdFrontAux = 0xF3,
};

#else
/// align with sensor_id, should be modify
enum XiaoMiSensorPosition {
    SensorIdRearWide = 0,
    SensorIdRearTele = 1,
    SensorIdFront = 2,
    SensorIdRearUltra = 3,
    SensorIdFrontAux = 4,
    SensorIdRearMacro = 5,
    SensorIdRearTele4x = 6,
    SensorIdRearMacro2x = 7,
    SensorIdRearDepth = 8,
};
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Enumeration describing Camera Role
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef enum {
    CameraRoleTypeDefault = 0, ///< Camera type Default
    CameraRoleTypeUltraWide,   ///< Camera type ultra wide
    CameraRoleTypeWide,        ///< Camera type wide
    CameraRoleTypeTele,        ///< Camera type tele
    CameraRoleTypeTele4X,      ///< Camera type tele 4x
    CameraRoleTypeMacro,       ///< Camera type marco
    CameraRoleTypeMax,         ///< Max role to indicate error
} CameraRoleType;

typedef struct
{
    CameraRoleType currentCameraRole; ///< Current camera role - wide / tele
    uint32_t currentCameraId;         ///< Current camera id
    uint32_t logicalCameraId;         ///< Logical camera id
    CameraRoleType masterCameraRole;  ///< Master camera role - wide / tele
} MultiCameraIdRole;

typedef struct
{
    uint32_t currentCameraId; ///< Current camera id
    uint32_t logicalCameraId; ///< Logical camera id
    uint32_t masterCameraId;  ///< master camera id
    CameraRoleType currentCameraRole;
} MultiCameraIds;

/// @brief Max number of faces detected
static const uint8_t FDMaxFaces = 10;

/// @brief Defines maximum number of linked session for Multicamera
#define MAX_LINKED_SESSIONS 2
#define MAX_ANALYIZE_FACE   3

/// @brief Defines maximum number of streams for a camera
static const uint32_t MaxLinkedCameras = 4;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Struct for a weighted region used for focusing/metering areas
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    uint8_t numFaces;              ///< Number of faces detected
    uint8_t faceScore[FDMaxFaces]; ///< List of face confidence scores for detected faces
    MIRECT faceRect[FDMaxFaces];   ///< List of face rect information for detected faces
} FDMetadataResults;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure describing camera metadata needed by SAT node
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    CameraRoleType cameraRole;       ///< Camera role this metadata belongs to
    CameraRoleType masterCameraRole; ///< Master camera role
    MIRECT userCropRegion;           ///< Overall user crop region
    MIRECT fovRectIFE;               ///< IFE FOV wrt to active sensor array
    MIRECT fovRectIPE;               ///< IPE FOV wrt to active sensor array
    WeightedRegion afFocusROI;       ///< AF focus ROI
    MIRECT activeArraySize;          ///< Wide sensor active array size
    FDMetadataResults fdMetadata;    ///< Face detection metadata
    bool isQuadCFASensor;            ///< For tell WT or WU combo
    int afType;
} CameraMetadata;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure describing Multi camera RTB input meta data structure
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    CameraMetadata cameraMetadata[MAX_LINKED_SESSIONS]; ///< Camera metdata for RTB
} InputMetadataBokeh; // Get fromt vendortag "com.xiaomi.multicamera.InputMetadataBokeh"

typedef struct
{
    uint32_t faceNum;
    float gender[MAX_ANALYIZE_FACE];
    float age[MAX_ANALYIZE_FACE];
    float faceScore[MAX_ANALYIZE_FACE];
    float prop[MAX_ANALYIZE_FACE];
} OutputMetadataFaceAnalyze;

typedef struct
{
    int32_t genderAndAgeEnable;
    int32_t faceScoreEnable;
    int32_t orientation;
    FDMetadataResults fdMetadata;
    OutputMetadataFaceAnalyze faceResult;
} InputMetadataFaceGrade;

typedef struct
{
    uint32_t number;
    int32_t ev[5];
} OutputMetadataHdrChecker;

union MiHDRMetaData {
    char _[64];
    struct _config
    {
        char version;
        char camera_id;
        char hdr_type;
        char orientation;
    } config;
};

typedef struct
{
    MiHDRMetaData miaiHDRMetaData;
} AiAsdHDRMetaData;

typedef struct
{
    float fLuxIndex;
    float fISPGain;
    float fSensorGain;
    float fADRCGain;
    float fADRCGainMax;
    float fADRCGainMin;
    int32_t i32CamMode;
} InputMetadataHdr;

typedef struct
{
    CameraMetadata cameraMetadata; ///< Camera metdata
} InputMetadataSR;

typedef struct
{
    uint8_t m_isNoByPass;
    int32_t orientation;
    int64_t sensorTimestamp;
} InputMetadataMiBokeh;

/// @brief Defines the AEC flash type
typedef enum {
    FlashInfoTypeOff,             ///< Flash off case
    FlashInfoTypePre,             ///< Flash is in preflash  case
    FlashInfoTypeMain,            ///< Flash in in mainflash case
    FlashInfoTypeCalibration,     ///< Flash in dual led calibaration case
    FlashInfoTypeMax = 0x7FFFFFFF ///< Anchor to indicate the last item in the defines
} AECFlashInfoType;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Describes AEC Output Data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Preflash states as decided by AEC algorithm
typedef enum {
    PreFlashStateInactive,      ///< Pre-flash off/inactive
    PreFlashStateStart,         ///< Estimation Starts; Preflash turns on - Sensor Node
    PreFlashStateTriggerFD,     ///< Start FD under preflash, consumed by FD Node
    PreFlashStateTriggerAF,     ///< Start AF under preflash, consumed by AF Node
    PreFlashStateTriggerAWB,    ///< Start AWB under preflash, consumed by AWB Node
    PreFlashStateCompleteLED,   ///< Estimation Completes with LED; Preflash turns Off - Sensor Node
    PreFlashStateCompleteNoLED, ///< Estimation Completes but no LED required; - Sensor Node
    PreFlashStateRER            ///< Red Eye Reduction has started
} PreFlashState;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Describes AEC Output Data
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Calibaration flash states as decided by AEC algorithm
typedef enum {
    CalibrationFlashReady,      ///< Calib-flash off/inactive
    CalibrationFlashCollecting, ///< Estimation Starts; Calibflash turns on - Sensor Node
    CalibrationPartialComplete, ///< One of the measurement done; Calibflash turns off - Sensor Node
    CalibrationFlashComplete    ///< Estimation completes; Calibflash turns off - Sensor Node
} CalibrationFlashState;

/// @brief Defines the exposure parameters to control the frame exposure setting to sensor and ISP
typedef struct
{
    uint64_t exposureTime;   ///< The exposure time in nanoseconds used by sensor
    float linearGain;        ///< The total gain consumed by sensor only
    float sensitivity;       ///< The total exposure including exposure time and gain
    float deltaEVFromTarget; ///< The current exposure delta compared with the desired target
} AECExposureData;

static const uint8_t ExposureIndexCount = 3;
static const uint8_t LEDSettingCount = 2; ///< Maximum number of LED Settings

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Describes Auto Exposure Control (AEC) Frame Control information
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    float luxIndex;        ///< Future frame lux index,  consumed by ISP
    uint64_t exposureTime; ///< The exposure time in nanoseconds used by sensor
    float linearGain;      ///< The total gain consumed by sensor only
    float sensitivity;     ///< The total exposure including exposure time and gain
    float adrcgain;        ///< The current exposure delta compared with the desired target
} InputMetadataAecInfo;    // Get fromt vendortag "com.xiaomi.statsconfigs.AecInfo"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure describing shift offset information
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    int32_t horizonalShift; ///< x cordinate of the pixel
    int32_t verticalShift;  ///< y cordinate of the pixel
} ImageShiftData;

typedef struct
{
    MIRECT cropRegion;
    int32_t isQuadCFASensor; ///< is QCFA Sensor
} SRCropRegion;

typedef struct
{
    MIRECT cropRegion;
    int32_t isQuadCFASensor;
    ImageShiftData shiftVector;
} SATCropInfo;

typedef struct
{
    ImageShiftData shiftSnapshot;
} SRShift;

// Face Detection static declaration
static const uint8_t MaxFaceROIs = 10; ///< Maximum index for Face ROIs

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Describes Common Statistics Data Structures
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Structure describing the rectangle coordinates
typedef struct
{
    uint32_t left;   ///< x coordinate of the ROI
    uint32_t top;    ///< y coordinate of the ROI
    uint32_t width;  ///< Width of the ROI
    uint32_t height; ///< Height of the ROI
} RectangleCoordinate;

// add for 3D pose
typedef struct
{
    int32_t valid; ///< Whether this info is valid
    float pitch;   ///< Up-Down direction of the face for x axis
    float yaw;     ///< Left-Right direction of the face for y axis
    float roll;    ///< Roll angle of the face for z axis
} Mi3DPoseCoordinate;

/// @brief Describes Face ROI
typedef struct
{
    uint32_t id;                  ///< Id used to track a face in the scene
    uint32_t confidence;          ///< Confidence of this face
    RectangleCoordinate faceRect; ///< Detected Face rectangle
    Mi3DPoseCoordinate mi3DPose;  ///< mi 3D pose
} FaceROIData;

/// @brief Structure describing Face ROI data
typedef struct
{
    uint64_t requestId;                       ///< Request ID corresponding to this results
    uint32_t ROICount;                        ///< Number of ROIs
    FaceROIData unstabilizedROI[MaxFaceROIs]; ///< Unstabilized Face ROI data
    FaceROIData stabilizedROI[MaxFaceROIs];   ///< Stabilized Face ROI data
} FaceROIInformation;

// Max string length value
static const uint16_t MaximumStringLength = 256;

typedef struct
{
    uint8_t bokehFNumberApplied[MaximumStringLength];
} BokehFNumberApplied;

typedef struct
{
    double fpcRatio;
    double fpcViewCropRatio;
    double fpcPara[21];
} DistortioFpcData;

typedef struct
{
    int calFilePathLen;
    char *calFilePath; ///< bokeh calibration data file path
    void *reserved;
} MiaStaticInitParams;
} // namespace mialgo2

#endif //