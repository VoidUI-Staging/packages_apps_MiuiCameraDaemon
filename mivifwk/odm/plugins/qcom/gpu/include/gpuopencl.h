#ifndef _GPUOPENCL_H_
#define _GPUOPENCL_H_
#include <CL/cl_ext_qcom.h>
#include <MiaPluginWraper.h>

#include <mutex>

#include "MiaPluginUtils.h"

using namespace mialgo2;

enum RotationAngle {
    Rotate0Degrees,   ///< Rotation of 0 Degrees in the Clockwise Direction
    Rotate90Degrees,  ///< Rotation of 90 Degrees in the Clockwise Direction
    Rotate180Degrees, ///< Rotation of 180 Degress in the Clockwise Direction
    Rotate270Degrees  ///< Rotation of 270 Degrees in the Clockwise Direction
};

enum FlipDirection {
    FlipNone,      ///< No Flip
    FlipLeftRight, ///< Left-Right Flip (Vertical mirror)
    FlipTopBottom, ///< Top-Bottom Flip (Horizontal mirror)
    FlipBoth       ///< Left-Right and Top-Bottom Flip (Vertical and Horizontal mirror)
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Class containing functions to use OpenCL as part of the GPU Node.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class GPUOpenCL
{
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// FlipImage
    ///
    /// @brief  Does a flip (mirror) to the image
    ///
    /// @param  hOutput            Handle to the output buffer.
    /// @param  hInput             Handle to the input buffer.
    /// @param  targetFlip         The direction of the flip.
    ///
    /// @return int32_tSuccess if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int32_t FlipImage(std::vector<ImageParams> &hOutput, std::vector<ImageParams> &hInput,
                      FlipDirection targetFlip);

    enum CLInitStatus {
        CLInitInvalid = 0,
        CLInitRunning = 1,
        CLInitDone = 2,
    };

    CLInitStatus m_initStatus;

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Initialize
    ///
    /// @brief  Initialize the resources required to use OpenCL with the GPU Node
    ///
    /// @return MIA_RETURN_OK if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int32_t Initialize();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// Uninitialize
    ///
    /// @brief  Cleans up the resources allocated to use OpenCL with the GPU Node
    ///
    /// @return MIA_RETURN_OK if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int32_t Uninitialize();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// GPUOpenCL
    ///
    /// @brief  Constructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    GPUOpenCL();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ~GPUOpenCL
    ///
    /// @brief  Destructor
    ///
    /// @return None
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ~GPUOpenCL();

    GPUOpenCL(const GPUOpenCL &) = delete;            ///< Disallow the copy constructor
    GPUOpenCL &operator=(const GPUOpenCL &) = delete; ///< Disallow assignment operator

private:
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeFuncPtrs
    ///
    /// @brief  Loads the OpenCL implementation and initializes the required function pointers
    ///
    /// @return MIA_RETURN_OK if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int32_t InitializeFuncPtrs();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeKernel
    ///
    /// @brief  Initialize the OpenCL kernels for the supported use cases
    ///
    /// @return MIA_RETURN_OK if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int32_t InitializeKernel();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// InitializeResources
    ///
    /// @brief  Initialize the OpenCL resources for the supported use cases
    ///
    /// @return MIA_RETURN_OK if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int32_t InitializeResources();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// ExecuteFlipImage
    ///
    /// @brief  Execute the selected kernel
    ///
    /// @param  dst         Destination image for the flip.
    /// @param  src         Source image for the flip.
    /// @param  width       Image width in pixels.
    /// @param  height      Image height in pixels.
    /// @param  inStride    Stride of the input buffer
    /// @param  outStride   Stride of the output buffer
    /// @param  inUVOffset  Offset of the input buffer to where the UV starts, 0 if flipping Luma
    /// plane
    /// @param  outUVOffset Offset of the output buffer to where the UV starts, 0 if flipping Luma
    /// plane
    /// @param  direction   Direction of the flip operation
    ///
    /// @return MIA_RETURN_OK if success or appropriate error code.
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    int32_t ExecuteFlipImage(cl_mem dst, cl_mem src, uint32_t width, uint32_t height,
                             uint32_t inStride, uint32_t outStride, uint32_t inUVOffset,
                             uint32_t outUVOffset, FlipDirection direction);

    // Typedefs for OpenCL functions. Refer to the OpenCL documentation on Khronos website for
    // parameter definitions

    /// @todo (CAMX-2286) Improvements to GPU Node Support: Add all the OpenCL 2.0 functions, not
    /// just the ones used.
    typedef CL_API_ENTRY cl_int(CL_API_CALL *PFNCLGETPLATFORMIDS)(cl_uint, cl_platform_id *,
                                                                  cl_uint *);
    typedef CL_API_ENTRY cl_int(CL_API_CALL *PFNCLGETDEVICEIDS)(cl_platform_id, cl_device_type,
                                                                cl_uint, cl_device_id *, cl_uint *);

    typedef CL_API_ENTRY cl_context(CL_API_CALL *PFNCLCREATECONTEXT)(
        const cl_context_properties *, cl_uint, const cl_device_id *,
        void(CL_CALLBACK *)(const char *, const void *, size_t, void *), void *, cl_int *);

    typedef CL_API_ENTRY cl_int(CL_API_CALL *PFNCLRELEASECONTEXT)(cl_context);

    typedef CL_API_ENTRY cl_command_queue(CL_API_CALL *PFNCLCREATECOMMANDQUEUE)(
        cl_context, cl_device_id, cl_command_queue_properties, cl_int *);

    typedef CL_API_ENTRY cl_int(CL_API_CALL *PFNCLRELEASECOMMANDQUEUE)(cl_command_queue);

    typedef CL_API_ENTRY cl_mem(CL_API_CALL *PFNCLCREATEIMAGE2D)(cl_context, cl_mem_flags,
                                                                 const cl_image_format *, size_t,
                                                                 size_t, size_t, void *, cl_int *);

    typedef CL_API_ENTRY cl_mem(CL_API_CALL *PFNCLCREATEIMAGE)(cl_context, cl_mem_flags,
                                                               const cl_image_format *,
                                                               const cl_image_desc *, void *,
                                                               cl_int *);

    typedef CL_API_ENTRY cl_int(CL_API_CALL *PFNCLRELEASEMEMOBJECT)(cl_mem);

    typedef CL_API_ENTRY cl_program(CL_API_CALL *PFNCLCREATEPROGRAMWITHSOURCE)(cl_context, cl_uint,
                                                                               const char **,
                                                                               const size_t *,
                                                                               cl_int *);

    typedef CL_API_ENTRY cl_int(CL_API_CALL *PFNCLBUILDPROGRAM)(
        cl_program, cl_uint, const cl_device_id *, const char *,
        void(CL_CALLBACK *)(cl_program, void *), void *);

    typedef CL_API_ENTRY cl_int(CL_API_CALL *PFNCLRELEASEPROGRAM)(cl_program);

    typedef CL_API_ENTRY cl_int(CL_API_CALL *PFNCLGETPROGRAMBUILDINFO)(cl_program, cl_device_id,
                                                                       cl_program_build_info,
                                                                       size_t, void *, size_t *);

    typedef CL_API_ENTRY cl_kernel(CL_API_CALL *PFNCLCREATEKERNEL)(cl_program, const char *,
                                                                   cl_int *);
    typedef CL_API_ENTRY cl_int(CL_API_CALL *PFNRELEASEKERNEL)(cl_kernel);
    typedef CL_API_ENTRY cl_int(CL_API_CALL *PFNCLSETKERNELARG)(cl_kernel, cl_uint, size_t,
                                                                const void *);

    typedef CL_API_ENTRY cl_int(CL_API_CALL *PFNCLENQUEUENDRANGEKERNEL)(
        cl_command_queue, cl_kernel, cl_uint, const size_t *, const size_t *, const size_t *,
        cl_uint, const cl_event *, cl_event *);

    typedef CL_API_ENTRY cl_int(CL_API_CALL *PFNCLENQUEUECOPYBUFFER)(
        cl_command_queue command_queue, cl_mem src_buffer, cl_mem dst_buffer, size_t src_offset,
        size_t dst_offset, size_t cb, cl_uint num_events_in_wait_list,
        const cl_event *event_wait_list, cl_event *event);

    typedef CL_API_ENTRY cl_int(CL_API_CALL *PFNCLFLUSH)(cl_command_queue);
    typedef CL_API_ENTRY cl_int(CL_API_CALL *PFNCLFINISH)(cl_command_queue);

    typedef CL_API_ENTRY cl_int(CL_API_CALL *PFNCLGETDEVICEINFO)(cl_device_id, cl_device_info,
                                                                 size_t, void *, size_t *);

    typedef CL_API_ENTRY cl_mem(CL_API_CALL *PFNCLCREATEBUFFER)(cl_context, cl_mem_flags, size_t,
                                                                void *, cl_int *);

    typedef CL_API_ENTRY cl_sampler(CL_API_CALL *PFNCLCREATESAMPLER)(cl_context, cl_bool,
                                                                     cl_addressing_mode,
                                                                     cl_filter_mode, cl_int *);

    typedef CL_API_ENTRY cl_int(CL_API_CALL *PFNCLRELEASESAMPLER)(cl_sampler);

    void *m_hOpenCLLib; ///< handle for OpenCL library.

    PFNCLGETPLATFORMIDS m_pfnCLGetPlatformIDs; ///< Function pointer for clGetPlatformIDs
    PFNCLGETDEVICEIDS m_pfnCLGetDeviceIDs;     ///< Function pointer for clGetDeviceIDs
    PFNCLCREATECONTEXT m_pfnCLCreateContext;   ///< Function pointer for clCreateContext
    PFNCLRELEASECONTEXT m_pfnCLReleaseContext; ///< Function pointer for clReleaseContext
    PFNCLCREATECOMMANDQUEUE
    m_pfnCLCreateCommandQueue; ///< Function pointer for clCreateCommandQueue
    PFNCLRELEASECOMMANDQUEUE
    m_pfnCLReleaseCommandQueue;                    ///< Function pointer for clReleaseCommandQueue
    PFNCLCREATEIMAGE2D m_pfnCLCreateImage2D;       ///< Function pointer for clCreateImage2D
    PFNCLCREATEIMAGE m_pfnCLCreateImage;           ///< Function pointer for clCreateImage
    PFNCLRELEASEMEMOBJECT m_pfnCLReleaseMemObject; ///< Function pointer for clReleaseMemObject
    PFNCLCREATEPROGRAMWITHSOURCE
    m_pfnCLCreateProgramWithSource;            ///< Function pointer for clCreateProgramWithSource
    PFNCLBUILDPROGRAM m_pfnCLBuildProgram;     ///< Function pointer for clBuildProgram
    PFNCLRELEASEPROGRAM m_pfnCLReleaseProgram; ///< Function pointer for clReleaseProgram
    PFNCLGETPROGRAMBUILDINFO
    m_pfnCLGetProgramBuildInfo;            ///< Function pointer for clGetProgramBuildInfo
    PFNCLCREATEKERNEL m_pfnCLCreateKernel; ///< Function pointer for clCreateKernel
    PFNRELEASEKERNEL m_pfnCLReleaseKernel; ///< Function pointer for clReleaseKernel
    PFNCLSETKERNELARG m_pfnCLSetKernelArg; ///< Function pointer for clSetKernelArg
    PFNCLENQUEUENDRANGEKERNEL
    m_pfnCLEnqueueNDRangeKernel;               ///< Function pointer for clEnqueueNDRangeKernel
    PFNCLFLUSH m_pfnCLFlush;                   ///< Function pointer for clFlush
    PFNCLFINISH m_pfnCLFinish;                 ///< Function pointer for clFinish
    PFNCLGETDEVICEINFO m_pfnCLGetDeviceInfo;   ///< Function pointer for clGetDeviceInfo
    PFNCLCREATEBUFFER m_pfnCLCreateBuffer;     ///< Function pointer for clCreateBuffer
    PFNCLCREATESAMPLER m_pfnCLCreateSampler;   ///< Function pointer for clCreateSampler
    PFNCLRELEASESAMPLER m_pfnCLReleaseSampler; ///< Function pointer for clReleaseSampler
    PFNCLENQUEUECOPYBUFFER m_pfnCLEnqueueCopyBuffer; ///< Function pointer for clEnqueueCopyBuffer

    cl_device_id m_device;                  ///< OpenCL GPU Device
    cl_context m_context;                   ///< OpenCL GPU Context
    cl_command_queue m_queue;               ///< OpenCL GPU Queue for Submissions
    cl_program m_program;                   ///< OpenCL Program with multiple Kernels
    cl_kernel m_copyImageKernel;            ///< Image Copy Kernel
    cl_kernel m_rotateImageKernel;          ///< Image Rotate Kernel
    cl_kernel m_flipImageKernel;            ///< Image Flip Kernel
    cl_kernel m_ds4SinglePlaneKernel;       ///< DS4 Single Plane Kernel
    cl_kernel m_boxFilterSinglePlaneKernel; ///< Box Filter Single Plane Kernel
    cl_mem m_ds4WeightsImage;               ///< DS4 Weights Image
    cl_sampler m_ds4Sampler;                ///< DS4 Sampler

    std::mutex *m_pOpenCLMutex; ///< Mutex to protect the opecl buffer creation

    /// @todo (CAMX-2286) Improvements to GPU Node Support: Need to optimize these kernels and
    /// dispatch size.
    ///                   Also, allow loading program source from a file.
    // Program Source containing multiple kernels
    const char *m_pProgramSource =
        "__kernel void copyImage(__write_only image2d_t dst, __read_only image2d_t src)\n"
        "{\n"
        "   int2   coord;\n"
        "   float4 color; \n"
        "   coord.x = get_global_id(0);  \n"
        "   coord.y = get_global_id(1);  \n"
        "   const sampler_t sampler1 = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP | "
        "CLK_FILTER_NEAREST; \n"
        "   color = read_imagef(src, sampler1, coord);\n"
        "   write_imagef(dst, coord, color);\n"
        "}\n"

        "__kernel void rotateImage(__write_only image2d_t dst,"
        "                          __read_only image2d_t src,"
        "                          int dstWidth,"
        "                          int dstHeight,"
        "                          int srcWidth,"
        "                          int srcHeight,"
        "                          float sinTheta,"
        "                          float cosTheta)\n"
        "{\n"
        "    const int ix = get_global_id(0);\n"
        "    const int iy = get_global_id(1);\n"
        "    const int halfWidth = dstWidth / 2;\n"
        "    const int halfHeight = dstHeight / 2;\n"
        "    int2 dstCoord = (int2)(ix, iy);\n"
        "    int2 srcCoord;\n"
        "    int translatedX = ix - halfWidth;\n"
        "    int translatedY = iy - halfHeight;\n"
        "    float rotatedX = (((float) translatedX) * cosTheta + ((float) translatedY) * "
        "sinTheta);\n"
        "    float rotatedY = (((float) translatedX) * -sinTheta + ((float) translatedY) * "
        "cosTheta);\n"
        "    srcCoord.x = rotatedX + (srcWidth / 2);\n"
        "    srcCoord.y = rotatedY + (srcHeight / 2);\n"
        "    const sampler_t sampler1 = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_REPEAT | "
        "CLK_FILTER_NEAREST; \n"
        "    float4 color; \n"
        "    color = read_imagef(src, sampler1, srcCoord);\n"
        "    write_imagef(dst, dstCoord, color);\n"
        "}\n"

        "__kernel void flipImage(__global unsigned char* dst,\n"
        "                        __global unsigned char* src,\n"
        "                        int width,\n"
        "                        int height,\n"
        "                        unsigned int inStride,\n"
        "                        unsigned int outStride,\n"
        "                        unsigned int inUVOffset,\n"
        "                        unsigned int outUVOffset,\n"
        "                        int direction)\n"
        "{\n"
        "    int2   srcCoord;\n"
        "    int2   dstCoord;\n"
        "    float4 color; \n"
        "    dstCoord.x = get_global_id(0);  \n"
        "    dstCoord.y = get_global_id(1);  \n"
        "    const sampler_t sampler1 = CLK_NORMALIZED_COORDS_TRUE | CLK_ADDRESS_CLAMP | "
        "CLK_FILTER_NEAREST; \n"
        "   if (0 == direction)\n"
        "   {\n"
        "       srcCoord.x = dstCoord.x;\n"
        "       srcCoord.y = dstCoord.y;\n"
        "   }\n"
        "   else if (1 == direction)\n"
        "   {\n"
        "       srcCoord.x = width - dstCoord.x - 1;\n"
        "        srcCoord.y = dstCoord.y;\n"
        "    }\n"
        "    else if (2 == direction)\n"
        "    {\n"
        "        srcCoord.x = dstCoord.x;\n"
        "        srcCoord.y = height - dstCoord.y - 1;\n"
        "    }\n"
        "    else\n"
        "    {\n"
        "        srcCoord.x = width - dstCoord.x - 1;\n"
        "        srcCoord.y = height - dstCoord.y - 1;\n"
        "    }\n"
        "   if (0 == inUVOffset)\n"
        "   {\n"
        "        dst[(dstCoord.y * outStride) + dstCoord.x] = src[(srcCoord.y * inStride) + "
        "srcCoord.x];\n"
        "   }\n"
        "   else\n"
        "   {\n"
        "       dst[(dstCoord.y * outStride) + (dstCoord.x * 2) + outUVOffset] = \n"
        "           src[(srcCoord.y * inStride) + (srcCoord.x * 2) + inUVOffset];\n"
        "       dst[(dstCoord.y * outStride) + (dstCoord.x * 2) + 1 + outUVOffset] = \n"
        "           src[(srcCoord.y * inStride) + (srcCoord.x * 2) + 1 + inUVOffset];\n"
        "   }\n"
        "}\n"

        "__kernel void ds4_single_plane (  __read_only     image2d_t       src_image,\n"
        "                                  __write_only    image2d_t       downscaled_image,"
        "                                  __read_only     qcom_weight_image_t Kvalues,\n"
        "                                  sampler_t       sampler,\n"
        "                                  float           src_y_max\n"
        "                                                                                  )\n"
        "{\n"
        "  int     wid_x               = get_global_id(0);\n"
        "  int     wid_y               = get_global_id(1);\n"
        "  int2    dst_coord  = (int2)(wid_x, wid_y); \n"
        "  float4  result; \n"
        "  float2  src_coord            = convert_float2(dst_coord) * 4.0f + 0.5f;\n"
        "  result = qcom_convolve_imagef(src_image, sampler, src_coord, Kvalues);\n"
        "  src_coord.y = fmin(src_coord.y, src_y_max);\n"
        "  write_imagef(downscaled_image, dst_coord, result); \n"
        "}\n"

        "__kernel void boxfilter_single_plane(  __read_only     image2d_t               "
        "src_image,\n"
        "                                       __write_only    image2d_t               "
        "downscaled_image,\n"
        "                                                       sampler_t               sampler,\n"
        "                                                       qcom_box_size_t         scale,\n"
        "                                                       float2                  "
        "unpacked_scale,\n"
        "                                                       float                   src_y_max\n"
        "                                                                                          "
        "    )\n"
        "{\n"
        "  int     wid_x           = get_global_id(0);\n"
        "  int     wid_y           = get_global_id(1);\n"
        "  float2  src_coord       = (float2)(((float)wid_x+0.5f)* unpacked_scale.x , "
        "((float)wid_y+0.5f) * unpacked_scale.y);\n"
        "  int2    downscaled_coord= (int2)(wid_x, wid_y);\n"
        "  float4  downscaled_pixel= (float4)(0.0f, 0.0f, 0.0f, 0.0f);\n"
        "  src_coord.y = fmin(src_coord.y, src_y_max);\n"
        "  // Do down_scaling\n"
        "  downscaled_pixel = qcom_box_filter_imagef(src_image, sampler, src_coord, scale);\n"
        "  // Writeout the down_scaled pixel\n"
        "  write_imagef(downscaled_image, downscaled_coord, downscaled_pixel);\n"
        "}\n";
};

#endif // _GPUOPENCL_H_