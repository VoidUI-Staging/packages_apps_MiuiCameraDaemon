
#ifndef _XMIHDR_H_
#define _XMIHDR_H_
#include <sstream>
// #include "arcsofthdr.h"

//#define XMI_LLHDR_AEINFO ARC_LLHDR_AEINFO
// #define LPXMI_LLHDR_AEINFO LPARC_LLHDR_AEINFO

// #define XmiHDRCommonAEParam ArcHDRCommonAEParam

// #define XmiHDRParam ArcHDRParam

namespace mihdr {
typedef int BOOL;
typedef char CHAR;
typedef uint8_t SBYTE;
typedef unsigned char UCHAR;
typedef int INT;
typedef unsigned int UINT;
typedef float FLOAT;

typedef uint8_t UINT8;
typedef int8_t INT8;
typedef uint16_t UINT16;
typedef int16_t INT16;
typedef uint32_t UINT32;
typedef int32_t INT32;
typedef uint64_t UINT64;
typedef int64_t INT64;

typedef max_align_t MAXALIGN_T;
typedef size_t SIZE_T;
typedef void VOID;
static const UINT8 ExposureIndexShort = 0; ///< Exposure setting index for short exposure
static const UINT8 ExposureIndexLong = 1;  ///< Exposure setting index for long exposure
static const UINT8 ExposureIndexSafe =
    2; ///< Exposure setting index for safe exposure (between short and long)
static const UINT8 ExposureIndexCount = 3; ///< Exposure setting max index
static const UINT8 LEDSetting1 = 0;        ///< Index for LED setting 1
static const UINT8 LEDSetting2 = 1;        ///< Index for LED setting 2
static const UINT8 LEDSettingCount = 2;    ///< Maximum number of LED Settings

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

/// @brief Y Histogram Stretch config data
typedef struct
{
    BOOL enable;   ///< Enable flag set by algo for ISP
    FLOAT clamp;   ///< Dark Clamp Value for Hist Stretch
    FLOAT scaling; ///< Scaling Value for Hist Stretch
    std::string toString()
    {
        std::stringstream st;
        st << "{enable:" << enable << " , clamp:" << clamp << " , scaling:" << scaling << "}";
        return st.str();
    }
} AECYHistStretchData;

/// @brief: Structure representing the calculated CDF for the in-sensor HDR 3 exposure trigger final
/// result
struct CDFSet
{
    FLOAT resultCDF0; ///< Calculated CDF 0
    FLOAT resultCDF5; ///< Calculated CDF 5
    std::string toString()
    {
        std::stringstream st;
        st << "{resultCDF0:" << resultCDF0 << " , resultCDF5:" << resultCDF5 << "}";
        return st.str();
    }
};

/// @brief: Structure representing the in-sensor HDR 3 exposure trigger final result
struct InSensorHDR3ExpTriggeResult
{
    BOOL trigger0; ///< Trigger condition 0
    BOOL trigger1; ///< Trigger condition 1
    BOOL trigger2; ///< Trigger condition 2
    std::string toString()
    {
        std::stringstream st;
        st << "{trigger0:" << trigger0 << " , trigger1:" << trigger1 << " , trigger2:" << trigger2
           << "}";
        return st.str();
    }
};

/// @brief: Structure representing in-sensor HDR 3 exposure trigger information
struct InSensorHDR3ExpTriggerInfo
{
    BOOL isTriggerInfoValid; ///< Flag to indicate whether the informatino is valid or not
    InSensorHDR3ExpTriggeResult
        finalTriggerResult; ///< The in-sensor HDR 3 exposure trigger final result
    CDFSet calculatedCDF;   ///< The calculated CDF trigger result
    FLOAT realDRCGain;      ///< The reak drc gain trigger result
    FLOAT ambientRaw;       ///< The ambient raw trigger result
    FLOAT faceLuma;         ///< The face luma CDF trigger result

    std::string toString()
    {
        std::stringstream st;
        st << "{isTriggerInfoValid:" << isTriggerInfoValid
           << " , finalTriggerResult:" << finalTriggerResult.toString()
           << " , calculatedCDF:" << calculatedCDF.toString() << " , realDRCGain:" << realDRCGain
           << " , ambientRaw:" << ambientRaw << " , faceLuma:" << faceLuma << "}";
        return st.str();
    }
};

/// @brief: Structure representing MFHDR trigger information
struct MFHDRTriggerInfo
{
    BOOL isSeamlessMFHDRSupported; ///< Flag to indicate whether the informatino is valid or not
    BOOL finalTriggerResult;       ///< The MFHDR trigger final result
    FLOAT realDRCGain;             ///< The reak drc gain for the MFHDR trigger result
    FLOAT ambientRaw;              ///< The ambient raw for the MFHDR trigger result
    CDFSet calculatedCDF;          ///< The calculated CDF for the IHDR trigger result
    FLOAT reserved1;               ///< Reserved Parameters
    FLOAT reserved2;               ///< Reserved Parameters
    FLOAT reserved3;               ///< Reserved Parameters
    FLOAT reserved4;               ///< Reserved Parameters
    FLOAT reserved5;               ///< Reserved Parameters
    FLOAT reserved6;               ///< Reserved Parameters
    std::string toString()
    {
        std::stringstream st;
        st << "{isSeamlessMFHDRSupported:" << isSeamlessMFHDRSupported
           << " , finalTriggerResult:" << finalTriggerResult << " , realDRCGain:" << realDRCGain
           << " , realDRCGain:" << realDRCGain << " , ambientRaw:" << ambientRaw
           << ",calculatedCDF:" << calculatedCDF.toString() << "reserved1:" << reserved1 << ","
           << "reserved2:" << reserved2 << ",reserved3:" << reserved3 << ",reserved4:" << reserved4
           << ",reserved5:" << reserved5 << ",reserved6:" << reserved6 << "}";
        return st.str();
    }
};

/// @brief Defines the exposure parameters to control the frame exposure setting to sensor and ISP
typedef struct
{
    UINT64 exposureTime;     ///< The exposure time in nanoseconds used by sensor
    FLOAT linearGain;        ///< The total gain consumed by sensor only
    FLOAT sensitivity;       ///< The total exposure including exposure time and gain
    FLOAT deltaEVFromTarget; ///< The current exposure delta compared with the desired target
    std::string toString()
    {
        std::stringstream st;
        st << "{exposureTime:" << exposureTime << " , linearGain:" << linearGain
           << " , sensitivity:" << sensitivity << ",deltaEVFromTarget:" << deltaEVFromTarget << "}";
        return st.str();
    }
} AECExposureData;

typedef struct
{
    // Internal Member Variables
    AECExposureData exposureInfo[ExposureIndexCount]; ///< Exposure info including gain and exposure
                                                      ///< time; consumed by ISP & sensor
    FLOAT luxIndex;                                   ///< Future frame lux index,  consumed by ISP
    FLOAT prevLuxIndex;                    ///< Previous frame lux index,  consumed by ISP
    AECFlashInfoType flashInfo;            ///< Flash information if it is main or preflash
    PreFlashState preFlashState;           ///< Preflash state from AEC, consumed by Sensor
    CalibrationFlashState calibFlashState; ///< Calibration state from AEC, consumed by Sensor
    FLOAT LEDFirstEntryRatio;              ///< ratio of LED entry w.r.t first entry
    FLOAT LEDLastEntryRatio;               ///< ratio of LED entry w.r.t last entry
    UINT32
    LEDCurrents[LEDSettingCount]; ///< The LED currents value for the use case of LED snapshot
    FLOAT LEDInfluenceRatio;      ///< The sensitivity ratio which is calculated from
                                  ///  sensitivity with no flash / sensitivity with
                                  ///  preflash
    FLOAT predictiveGain; /// digital gain from predictive convergence algorithm, to be applied to
                          /// ISP module (AWB)
    FLOAT digitalGainForSimulation;     ///< Digital gain for AEC simulation
    FLOAT compenADRCGain;               ///< Compensation ADRC gain
    BOOL isInSensorHDR3ExpSnapshot;     ///< Decide the sensor mode for CFA sensor
    AECYHistStretchData stretchControl; ///< Y Histogram Stretch Control
    InSensorHDR3ExpTriggerInfo
        inSensorHDR3ExpTriggerOutput; ///< In-sensor HDR 3 exposure trigger output information

    //#ifdef __XIAOMI_CAMERA__
    BOOL isMFHDRSnapshot;                    ///< Decide MFHDR Trigger
    MFHDRTriggerInfo MFHDRTriggerOutputInfo; ///< MFHDR trigger output information
    INT32 evSet[3];
    BOOL isMFHDREvCtrl; ///< Flag to indicate whether the EV Control is enable or not
    BOOL isMFHDREVLocked;
    //#endif

    std::string toString()
    {
        std::stringstream st;
        st << "{exposureInfo:[";
        for (int i = 0; i < ExposureIndexCount; i++) {
            st << exposureInfo[i].toString() << " ";
        }
        st << "],\n";
        st << "luxIndex:" << luxIndex << ",\n"
           << "prevLuxIndex:" << prevLuxIndex << ",\n"
           << "flashInfo:" << (int)flashInfo << ",\n"
           << "preFlashState:" << (int)preFlashState << ",\n";
        st << "calibFlashState:" << (int)calibFlashState << ",\n"
           << "LEDFirstEntryRatio:" << LEDFirstEntryRatio << ",\n"
           << "LEDLastEntryRatio:" << LEDLastEntryRatio << ",\n";
        st << "LEDCurrents:[";
        for (int i = 0; i < LEDSettingCount; i++) {
            st << LEDCurrents[i] << " ";
        }
        st << "],\n";
        st << "LEDInfluenceRatio:" << LEDInfluenceRatio << ",\n"
           << "predictiveGain:" << predictiveGain << ",\n"
           << "digitalGainForSimulation:" << digitalGainForSimulation << ",\n";
        st << "compenADRCGain:" << compenADRCGain << ",\n"
           << "isInSensorHDR3ExpSnapshot:" << isInSensorHDR3ExpSnapshot << ",\n";
        st << "stretchControl:" << stretchControl.toString() << ",\n";
        st << "inSensorHDR3ExpTriggerOutput" << inSensorHDR3ExpTriggerOutput.toString() << ",\n";
        st << "isMFHDRSnapshot:" << isMFHDRSnapshot << ",\n";
        st << "MFHDRTriggerOutputInfo:" << MFHDRTriggerOutputInfo.toString() << ",\n";
        st << "evSet:[";
        for (int i = 0; i < 3; i++) {
            st << evSet[i] << " ";
        }
        st << "],\n";
        st << "isMFHDREvCtrl:" << isMFHDREvCtrl << ",\n"
           << "isMFHDREVLocked:" << isMFHDREVLocked << "\n";
        st << "}";
        return st.str();
    }
} AECFrameControl;

// AWB static definitions
static const UINT8 AWBNumCCMRows = 3;       ///< Number of rows for Color Correction Matrix
static const UINT8 AWBNumCCMCols = 3;       ///< Number of columns for Color Correction Matrix
static const UINT32 AWBAlgoDecisionMax = 3; ///< Number of AWB decision output
static const UINT8 MaxCCMs = 3;             ///< Max number of CCMs supported

typedef struct
{
    FLOAT rGain; ///< Red gains
    FLOAT gGain; ///< Green gains
    FLOAT bGain; ///< Blue gains
    std::string toString()
    {
        std::stringstream st;
        st << "{rGain:" << rGain << " , gGain:" << gGain << " , bGain:" << bGain << "}";
        return st.str();
    }
} AWBGainParams;
typedef struct
{
    BOOL isCCMOverrideEnabled;               ///< Flag indicates if CCM override is enabled.
    FLOAT CCM[AWBNumCCMRows][AWBNumCCMCols]; ///< The color correction matrix
    FLOAT CCMOffset[AWBNumCCMRows];          ///< The offsets for color correction matrix
    std::string toString()
    {
        std::stringstream st;
        st << "{isCCMOverrideEnabled:" << isCCMOverrideEnabled << ",\n";
        st << "CCM:[\n";
        for (int i = 0; i < AWBNumCCMRows; i++) {
            st << "[";
            for (int j = 0; j < AWBNumCCMCols; j++) {
                st << CCM[i][j] << " ";
            }
            st << "],\n";
        }
        st << "],\n";
        st << "CCMOffset:[";
        for (int k = 0; k < AWBNumCCMRows; k++) {
            st << CCMOffset[k] << " ";
        }
        st << "]}\n";
        return st.str();
    }
} AWBCCMParams;
/// @brief Defines the format of the decision data for AWB algorithm.
typedef struct
{
    FLOAT rg;
    FLOAT bg;
    std::string toString()
    {
        std::stringstream st;
        st << "rg:" << rg << ",\n"
           << "bg:" << bg << "\n";
        return st.str();
    }
} AWBDecisionPoint;
typedef struct
{
    AWBDecisionPoint point[AWBAlgoDecisionMax]; ///< AWB decision point in R/G B/G coordinate
    FLOAT correlatedColorTemperature[AWBAlgoDecisionMax]; ///< Correlated Color Temperature: degrees
                                                          ///< Kelvin (K)
    UINT32 decisionCount;                                 ///< Number of AWB decision
    std::string toString()
    {
        std::stringstream st;
        st << "{point:[";
        for (int i = 0; i < AWBAlgoDecisionMax; i++) {
            st << "{" << point[i].toString() << "} ";
        }
        st << "],\n";
        st << "correlatedColorTemperature:[";
        for (int j = 0; j < AWBAlgoDecisionMax; j++) {
            st << correlatedColorTemperature[j] << " ";
        }
        st << "],\n";
        st << "decisionCount:" << decisionCount << "}\n";
        return st.str();
    }
} AWBAlgoDecisionInformation;
typedef struct
{
    // Internal Member Variables
    AWBGainParams AWBGains;                 ///< R/G/B gains
    UINT32 colorTemperature;                ///< Color temperature
    AWBCCMParams AWBCCM[MaxCCMs];           ///< Color Correction Matrix Value
    UINT32 numValidCCMs;                    ///< Number of Valid CCMs
    AWBAlgoDecisionInformation AWBDecision; ///< AWB decesion information
    FLOAT ambientRate;                      ///< ambientRate to trigger different CCM
    FLOAT MCCconf;                          ///< color checker SA confidence
    std::string toString()
    {
        std::stringstream st;
        st << "{AWBGains:" << AWBGains.toString() << ",\n"
           << "colorTemperature: " << colorTemperature << ",\n"
           << "AWBCCM:[";
        for (int i = 0; i < MaxCCMs; i++) {
            st << AWBCCM[i].toString() << " ";
        }
        st << "],\n";
        st << "numValidCCMs:" << numValidCCMs << ",\n";
        st << "AWBDecision:" << AWBDecision.toString() << ",\n";
        st << "ambientRate:" << ambientRate << ",\n"
           << "MCCconf:" << MCCconf << "}\n";
        return st.str();
    }
} AWBFrameControl;
} // namespace mihdr

typedef struct
{
    UINT64 exposureTime;
    FLOAT linearGain;
    FLOAT sensitivity;
    std::string toString()
    {
        std::stringstream st;
        st << "{exposureTime:" << exposureTime << " , linearGain:" << linearGain
           << " , sensitivity:" << sensitivity << "}";
        return st.str();
    }
} MiAECExposureData;

typedef struct
{
    FLOAT rGain;
    FLOAT gGain;
    FLOAT bGain;
    UINT32 colorTemperature;

    std::string toString()
    {
        std::stringstream st;
        st << "{rGain:" << rGain << " , gGain:" << gGain << " , bGain:" << bGain
           << "}, colorTemperature=" << colorTemperature;
        return st.str();
    }
} MiAWBGainParams;

#endif
