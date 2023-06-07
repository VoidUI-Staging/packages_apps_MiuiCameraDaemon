/**************************************************************
 *  Filename:    mialgo_layer.h
 *  Copyright:   XiaoMi Co., Ltd.
 *
 *  Description: A class to contain he most commonly used data
 *  structure
 *
 *  @author:     wangliming6@xiaomi.com
 *  @author:     yangguide@xiaomi.com
 *  @concat:     18817362189
 *  @version     2021-12-09
 **************************************************************/

#pragma once
#include <iostream>

#define MIALGO_IMG_MAGIC_AIO 69632


typedef enum {
    MIALGO_MEM_NONE_AIO = 0, /*!< invalid mem type */
    MIALGO_MEM_HEAP_AIO,     /*!< heap mem */
    MIALGO_MEM_ION_AIO,      /*!< ion mem */
    MIALGO_MEM_CL_AIO,       /*!< opencl svm */
    MIALGO_MEM_NUM_AIO,      /*!< mem type num */
} MialgoMemType_AIO;

typedef struct {
    MialgoMemType_AIO type;  /*!< memory type, @see MialgoMemType */
    unsigned long long size; /*!< memory size */
    unsigned int phy_addr;   /*!< physical address */
    signed int fd;           /*!< fd of ion memory */
} MialgoMemInfo_AIO;

typedef enum {
    MIALGO_IMG_INVALID_AIO = 0, /*!< invalid image format */

    // numeric
    MIALGO_IMG_NUMERIC_AIO =
        MIALGO_IMG_MAGIC_AIO + 100, /*!< numeric image format */

    // yuv
    MIALGO_IMG_GRAY_AIO = MIALGO_IMG_MAGIC_AIO + 200, /*!< gray image format */
    MIALGO_IMG_NV12_AIO,                              /*!< nv12 image format */
    MIALGO_IMG_NV21_AIO,                              /*!< nv21 image format */
    MIALGO_IMG_I420_AIO,                              /*!< i420 image format */
    MIALGO_IMG_NV12_16_AIO, /*!< nv12 16bit image format */
    MIALGO_IMG_NV21_16_AIO, /*!< nv21 16bit image format */
    MIALGO_IMG_I420_16_AIO, /*!< i420 16bit image format */
    MIALGO_IMG_P010_AIO,    /*!< p010 image format */

    // ch last rgb
    MIALGO_IMG_RGB_AIO =
        MIALGO_IMG_MAGIC_AIO + 300, /*!< nonplanar rgb image format */
    MIALGO_IMG_BGR_AIO,

    // ch first rgb
    MIALGO_IMG_PLANE_RGB_AIO =
        MIALGO_IMG_MAGIC_AIO + 400, /*!< planar rgb image format */

    // raw
    MIALGO_IMG_RAW_AIO = MIALGO_IMG_MAGIC_AIO + 500, /*!< raw image format */
    MIALGO_IMG_MIPIRAWPACK10_AIO,   /*!< mipi raw pack 10bit image format */
    MIALGO_IMG_MIPIRAWUNPACK8_AIO,  /*!< mipi raw unpack 8bit image format */
    MIALGO_IMG_MIPIRAWUNPACK16_AIO, /*!< mipi raw unpack 16bit image format */

    MIALGO_IMG_YUV444_AIO =
        MIALGO_IMG_PLANE_RGB_AIO, /*!< same as MIALGO_IMG_PLANE_RGB */
    MIALGO_IMG_NUM_AIO,
} MialgoImgFormat_AIO;

typedef struct MialgoImg_AIO {  // define a buffer struct to store image data
    int imgWidth;                 // the width of image
    int imgHeight;                // the height of image
    int imgChannel;               // the channel of image
    unsigned char *data;          // the data of image
    void *pYData;                 // the data of y image
    void *pUVData;                // the data of uv image
    int stride;
    MialgoMemInfo_AIO mem_info;
    MialgoImgFormat_AIO img_format;  // the format of image
} MialgoImg_AIO;

typedef enum {
    AI_VISION_RUNTIME_INVALID_AIO = 0,
    AI_VISION_RUNTIME_CPU_AIO = 1,
    AI_VISION_RUNTIME_GPU_AIO,
    AI_VISION_RUNTIME_DSP_AIO,
} MialgoAiVisionRuntime_AIO;

typedef enum {
    AI_VISION_PRIORITY_NORMAL_AIO = 0,
    AI_VISION_PRIORITY_HIGH_AIO,
    AI_VISION_PRIORITY_LOW_AIO,
} MialgoAiVisionPriority_AIO;

typedef enum {
    AI_VISION_PERFORMANCE_BALANCED_AIO = 0,
    AI_VISION_PERFORMANCE_HIGH_AIO,
    AI_VISION_PERFORMANCE_LOW_AIO,
    AI_VISION_PERFORMANCE_SYSTEM_AIO,
} MialgoAiVisionPerformance_AIO;

struct BBox {
    BBox() {}
    BBox(float x0_, float y0_, float x1_, float y1_) {
        x0 = x0_;
        y0 = y0_;
        x1 = x1_;
        y1 = y1_;
    }

    void print() const {
        std::cout << "x0:" << x0 << std::endl;
        std::cout << "y0:" << y0 << std::endl;
        std::cout << "x1:" << x1 << std::endl;
        std::cout << "y1:" << y1 << std::endl;
        std::cout << "label:" << label << std::endl;
        std::cout << "prob :" << prob << std::endl;
    }

    float cx() const { return (x1 + x0) / 2; }

    float cy() const { return (y1 + y0) / 2; }

    float w() const { return std::max(0.0f, x1 - x0); }

    float h() const { return std::max(0.0f, y1 - y0); }

    float area() const { return w() * h(); }

    float get_label() const { return label; }

    float x0 = 0;
    float y0 = 0;
    float x1 = 0;
    float y1 = 0;
    int label = 0;
    float prob = 0.0;
};

struct Point {
    Point() {}
    Point(float x_, float y_) {
        x = x_;
        y = y_;
    }
    float x = 0;
    float y = 0;
};

class Layer {
 private:
    float *layer_data;

 public:
    std::string layer_name;
    int h;
    int w;
    int c;
    int size;
    Layer();
    ~Layer();
    Layer(std::string layer_name_, int layer_height_, int layer_width_,
          int layer_channel_);
    void SetLayerInfo(std::string layer_name_, int layer_height_,
                      int layer_width_, int layer_channel_);
    int GetLayerSize();
    void SetData(float *data_in);
    float *GetData();
    std::string GetLayerName();
    void print();
};
