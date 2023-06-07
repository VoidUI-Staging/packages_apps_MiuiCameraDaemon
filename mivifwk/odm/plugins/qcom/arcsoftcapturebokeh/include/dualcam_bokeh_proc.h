/************************************************************************************

Filename  : dualcam_bokeh_proc.h
Content   :
Created   : May. 01, 2020
Author    : ningyi (ningyi@xiaomi.com)
Copyright : Copyright 2020 Xiaomi Mobile Software Co., Ltd. All Rights reserved.

*************************************************************************************/
#ifndef _DUALCAM_BOKEH_PROC_H_
#define _DUALCAM_BOKEH_PROC_H_

//output mask size info define
#define MIALGO_MULTICAM_MASK_W                      (960)
#define MIALGO_MULTICAM_MASK_H                      (720)

enum ALGO_BOKEH_FORMAT
{
    BOKEH_FORMAT_DEFAULT = 0,
    BOKEH_FORMAT_NV12 = 1,
    BOKEH_FORMAT_NV21 = 2,
    BOKEH_FORMAT_GRAY = 3,
    BOKEH_FORMAT_OTHERS = 4,
};

enum ALGO_BEAUTY_LENS_MODE
{
    BMODE_PORTRAIT,
    BMODE_FOOD,
    BMODE_PET,
    BMODE_OBJECT,
    BMODE_OFF
};

typedef struct s_AlgoBokehImg
{
    int width;
    int height;
    int stride;
    int slice;
    void *pData;
    ALGO_BOKEH_FORMAT format;
} AlgoBokehImg;

typedef struct
{
    int x;                                      // x coordinates, Usually based on 0
    int y;                                      // y coordinates, Usually based on 0
} AlgoBokehPoint;

typedef struct
{
    int x;                                      // x coordinates of the top left corner of the rectangle
    int y;                                      // y coordinates of the top left corner of the rectangle
    int width;                                  // width in pixels
    int height;                                 // height in pixels
    int angle;                                  // face angle 0/90/180/270
} AlgoBokehRect;

enum ALGO_CAPTURE_BOKEH_ZOOM_RATIO
{
    CAPTURE_BOKEH_2X = 0,
    CAPTURE_BOKEH_1X = 1
};

enum ALGO_CAPTURE_BOKEH_FRAME
{
    CAPTURE_BOKEH_4_3 = 0,
    CAPTURE_BOKEH_16_9 = 1,
    CAPTURE_BOKEH_FULL_FRAME = 2
};

enum ALGO_CAPTURE_BOKEH_MODE
{
    CAPTURE_BOKEH_QUALITY_MODE = 0,
    CAPTURE_BOKEH_PERFORMANCE_MODE = 1
};

enum ALGO_CAPTURE_BOKEH_DEBUG
{
    CAPTURE_BOKEH_DEBUG_OFF = 0,
    CAPTURE_BOKEH_DEBUG_ON = 1
};

typedef struct AlgoBokehInterface
{
    int AfCode;                                   // [in]  The lens position info.S
    int aperture;                                 // [in]  The intensity of blur,Range is [0,24], default as 11.
    int ISO;                                      // [in]  The iso of current image.
    int TOF;                                      // [in]  The dot tof
    int motionFlag = 0;                           // [in]  The trigger motion flag for serial scenes, new scene as 0, same as 1.
    int sceneTag;                                 // [out] The sceneTag result
    float sceneTagConfidence;                     // [out] The sceneTag confidence
} AlgoBokehInterface;

typedef struct AlgoBokehLaunchParams
{
    unsigned char*calib_buf;                      // [in]  The calibration buffer
    int calib_buf_size;                           // [in]  The calibration buffer's size
    const char* jsonFilename;                     // [in]  The json file full name
    unsigned int MainImgW;                        // [in]  The main img width
    unsigned int MainImgH;                        // [in]  The main img height
    unsigned int AuxImgW;                         // [in]  The aux img width
    unsigned int AuxImgH;                         // [in]  The aux img height
    ALGO_CAPTURE_BOKEH_FRAME frame;               // [in]  The capture bokeh frame
    ALGO_CAPTURE_BOKEH_ZOOM_RATIO zoomRatio;      // [in]  The capture bokeh zoom ration
    ALGO_CAPTURE_BOKEH_MODE bokehMode;            // [in]  The Switch between bokeh performance and normal version
    ALGO_CAPTURE_BOKEH_DEBUG debugMode;           // [in]  The dump switch, defined by persist prop
    int  arrangement;                             // [in]The dual cameras arrangement
    void *simulationInfo = nullptr;               // [in]the information for offline simulation
    unsigned int thermallevel = ( unsigned int )0;                  // [in] The Phone temperature 0: Low < 35; 1: Enter 35 ~ 37;2: mid  46 ~48; 3: High 49 ~ 50
    ALGO_BEAUTY_LENS_MODE beautyLensMode = BMODE_OFF;  // [in] The mode of beauty lens
} AlgoBokehLaunchParams;

typedef struct
{
    AlgoBokehImg *imgL;                         // [in]  The main image input. Generally, use smaller FOV sensor as main.
    AlgoBokehImg *imgR;                         // [in]  The auxiliary image input.
    AlgoBokehImg *imgL_EV_1;                    // [in]  The  EV-1 main image input.
    AlgoBokehRect fRect;                        // [in]  The face rect.
    AlgoBokehPoint tp;                          // [in]  The touch point.
    AlgoBokehInterface interface;               // [in]  The dumpInfo interface.
    const char* dstPath;                        // [in]  Useless Param in Android, pass NULL.
    const char* cShortName;                     // [in]  Useless Param in Android, pass NULL.
} AlgoBokeInitParams;

typedef struct
{
    unsigned char *result_data_buf;             // [out] The ptr of disparity map.
    int result_data_size;                       // [in]  The size of disparity map.
    AlgoBokehImg *imgMask;                      // [out] The mask image output.
} AlgoBokehDepthParams;

typedef struct
{
    AlgoBokehImg *imgOri;                       // [in]  The original image input for bokeh effect.
    unsigned char *depth_info_buf;              // [in]  The ptr of disparity map.
    int depth_info_size;                        // [in]  The size of disparity map.
    AlgoBokehImg *imgResult;                    // [out] The bokeh image output.
} AlgoBokehEffectParams;

/************************************************************************
*the following functions are used to calculate Bokeh Effect in Capture
************************************************************************/
int MIALGO_CaptureInitWhenLaunch(               // return 0 if success, otherwise fail, only called once when launching
    void **phEngine,                            // [in]  The algorithm engine
    AlgoBokehLaunchParams& launchParams         // [in] The algorithm launch params
);

void MIALGO_CaptureInitEachPic(                 // no return value, called before every capture.
    void **phEngine,                            // [in]  The algorithm engine
    AlgoBokeInitParams& initParams              // [in]  The algorithm init params
);

int MIALGO_CaptureGetDepthDataSize(             // return 0 if success, otherwise fail.
    void **phEngine,                            // [in]  The algorithm engine.
    int *size                                   // [out] The size of disparity map.
);

int MIALGO_CaptureGetDepthProc(                 // return 0 if success, otherwise fail.
    void **phEngine,                            // [in]  The algorithm engine.
    AlgoBokehDepthParams& depthParams           // [in]  The algorithm depth params.
);

int MIALGO_CaptureBokehEffectProc(              // return 0 if success, otherwise fail.
    void **phEngine,                            // [in]  The algorithm engine.
    AlgoBokehEffectParams& bokehParams          // [in]  The algorithm bokeh effect params
);

void MIALGO_CaptureDestoryEachPic(              // no return value, called after every capture.
    void **phEngine                             // [in]  The algorithm engine.
);

int MIALGO_CaptureDestoryWhenClose(             // return 0 if success, otherwise fail, only called once when node deInit.
    void **phEngine                             // [in]  The algorithm engine.
);

#endif
