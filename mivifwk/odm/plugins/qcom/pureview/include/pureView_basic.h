//********************************************************************************************
//  Filename    : pureView_basic.h
//  Date        : Mar 25, 2020
//  Author      : Zheng Guoqing(zhengguoqing@xiaomi.com)
//  Desciption  :
//  others      :
//********************************************************************************************
//      Copyright 2020 Xiaomi Mobile Software Co., Ltd. All Rights reserved.
//********************************************************************************************

#ifndef __PUREVIEW_BASIC_H__
#define __PUREVIEW_BASIC_H__

typedef long long int PV_UINT64;
typedef unsigned int PV_UINT32;
typedef unsigned short PV_UINT16;
typedef unsigned char PV_UINT8;
typedef char PV_INT8;
typedef short PV_INT16;
typedef int PV_INT32;
typedef signed long long PV_INT64;
typedef int PV_BOOL;
typedef float PV_FLOAT32;
typedef double PV_FLOAT64;
typedef void *PV_ENGINE;
typedef void *PV_HANDLE;
typedef void PV_VOID;

#define PV_OK       (0)  // everithing is ok
#define PV_ERROR    (-1) // unknown/unspecified error
#define PV_IO_ERROR (-2) // io error
#define PV_NO_MEM   (-3) // insufficient memory
#define PV_NULL_PTR (-4) // null pointer
#define PV_BAD_ARG  (-5) // arg/param is bad
#define PV_BAD_OPT  (-6) // bad operation

#define IMG_MASK  (0xff)
#define IMG_SHIFT (12)
#define IMG_VAL   (0x11)
#define IMG_MAGIC (IMG_VAL << IMG_SHIFT)

#define IMG_MAX_PLANE (4)
#define MAX_FACE_NUM  (10)

#define PV_MAX_FRAME_NUM (8)

typedef enum PV_IMG_FORMAT_E {

    IMG_INVALID = 0, /*!< invalid image format */

    // pureview used, and these enum value should be the same as integration group
    IMG_NV21 = 17,            /*!< nv21 image format */
    IMG_MIPIRAWUNPACK16 = 32, /*!< mipi raw unpack 16bit image format */
    IMG_NV12 = 35,            /*!< nv12 image format */

    // numeric
    IMG_NUMERIC, /*!< numeric image format */

    // yuv
    IMG_GRAY,    /*!< gray image format */
    IMG_I420,    /*!< i420 image format */
    IMG_NV12_16, /*!< nv12 16bit image format */
    IMG_NV21_16, /*!< nv21 16bit image format */
    IMG_I420_16, /*!< i420 16bit image format */
    IMG_P010,    /*!< p010 image format */
    IMG_GRAY_16, /*!< 16bit gray image format*/

    // ch last rgb
    IMG_RGB, /*!< nonplanar rgb image format */

    // ch first rgb
    IMG_PLANE_RGB, /*!< planar rgb image format */
    IMG_YUV444 = IMG_PLANE_RGB,

    // raw
    IMG_RAW,            /*!< raw image format */
    IMG_MIPIRAWPACK10,  /*!< mipi raw pack 10bit image format */
    IMG_MIPIRAWUNPACK8, /*!< mipi raw unpack 8bit image format */
    IMG_NUM
} PV_IMG_FORMAT;

typedef enum {
    PV_MEM_NONE = 0, /*!< invalid mem type */
    PV_MEM_HEAP,     /*!< heap mem */
    PV_MEM_ION,      /*!< ion mem */
    PV_MEM_CL,       /*!< opencl svm */
    PV_MEM_NUM,      /*!< mem type num */
} PV_MEM_TYPE;

typedef enum {
    PV_LOCAL_PPC = 0, /*!< ppc */
    PV_LOCAL_DIS,     /*!< dis */
} PV_LOCAL_ALIGN_TYPE;

/**
 * @brief memory info.
 *   For the implementation of HVX, We have to know the fd of ion buf, MialgoMemInfo is used to
 * record this information.
 *
 * Description of type
 *   @see MialgoMemType.
 *
 * Description of size
 *   for alignment reason, the actual allocated memory is greater than the applied memory.
 *   size is the actual allocated memory size.
 *
 * Description of phy_addr
 *   the memory physical address, currently not in use.
 *
 * Description of fd
 *   fd of ion memory, for heap memoy, fd is 0 and will not used.
 */
typedef struct
{
    PV_MEM_TYPE type;  /*!< memory type, @see MialgoMemType */
    PV_UINT64 size;    /*!< memory size */
    PV_UINT32 phyAddr; /*!< physical address */
    PV_INT32 fd;       /*!< fd of ion memory */
} PV_MEM_INFO;

typedef struct PV_IMG_ST
{
    PV_IMG_FORMAT format;          /*!< image format, @see AMBT_IMG_FORMAT*/
    PV_INT32 w;                    /*!< image width in pixels */
    PV_INT32 h;                    /*!< image hight in pixels */
    PV_INT32 pitch[IMG_MAX_PLANE]; /*!< INT32 an array, stored four elements*/
    void *data[IMG_MAX_PLANE];     /*!< void* an array pointer, stored image data header address */
    PV_MEM_INFO memInfo[IMG_MAX_PLANE]; /*!<>*/
} PV_IMG;

typedef struct PV_RECT_ST
{
    int x; // topLeft.x
    int y; // topLeft.y
    int w; // width
    int h; // height
} PV_RECT;

typedef struct PV_FACE_POSTION_ST
{
    int valid;
    int roll;
    int yawn;
    int pitch;
} PV_FACE_POSTION;

typedef struct PV_FACE_INFO_ST
{
    PV_INT32 faceNum;
    PV_INT32 faceAngle[MAX_FACE_NUM];        // 0, 90, 180, 270 degree
    PV_FACE_POSTION face3dPos[MAX_FACE_NUM]; // maybe used in future
    PV_RECT faceRect[MAX_FACE_NUM];          // face rectangle for each face
} PV_FACE_INFO;

typedef struct PV_SR_ROI_RECT_ST
{
    int x; // topLeft.x
    int y; // topLeft.y
    int w; // width
    int h; // height
} PV_SR_ROI_RECT;

#include <functional>
typedef std::function<void(PV_IMG *)> PureViewFrameCallBack;
// external parameter for pure shot algorithm
typedef struct PV_CONFIG_ST
{
    PV_INT32 imgWidth;            // image width
    PV_INT32 imgHeight;           // image height
    PV_INT32 iso;                 // iso speed of current shot
    PV_IMG_FORMAT inputImgFormat; // image format, basically shoulde be NV12 or NV21
    PV_INT32 inputFrameNum;       // inoput frame number for pure shot algorithm

    PV_SR_ROI_RECT srRoiRect; // sr roi information, must be allocate by function caller
    PV_INT32 baseAnchor;      // baseFrameIndex computed from raw image
    PV_FACE_INFO faceList;    // face information by caller, must be allocate by function caller

    PV_INT32 frameReadyFlags[PV_MAX_FRAME_NUM]; // flags notify if the frame data is sent
    PureViewFrameCallBack
        callback;           // callback func notify that the input frame data is not required
    int cameraAbnormalQuit; // 0: normal quit; 1: force quit

    char *pXmlFileName;          // parameter file name
    char inputFileName[256];     // this is for debug
    char *pPortraitMaskFileName; // this for windows running only

    PV_HANDLE pureViewParam; // this pointer to internal parameters,
                             // main function no need to allocate this buffer;
                             // this buffer will be allocated in pureViewInit() function
    PV_ENGINE pureViewAlgo;  // this pointer to mialgo speed up to use
} PV_CONFIG;

#endif
