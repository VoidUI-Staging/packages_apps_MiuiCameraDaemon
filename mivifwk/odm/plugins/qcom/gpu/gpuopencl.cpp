#include "gpuopencl.h"

#if defined(_LP64)
static const char OpenCLLibPath[] = "/vendor/lib64/";
#else
static const char OpenCLLibPath[] = "/vendor/lib/";
#endif // defined(_LP64)

#undef LOG_TAG
#define LOG_TAG "GpuOpenCL"

const char *pDefaultOpenCLLibraryName = "libOpenCL";

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::GPUOpenCL
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
GPUOpenCL::GPUOpenCL() : m_initStatus(CLInitInvalid), m_ds4Sampler(NULL)
{
    Initialize();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::~GPUOpenCL
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
GPUOpenCL::~GPUOpenCL()
{
    Uninitialize();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::InitializeFuncPtrs
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t GPUOpenCL::InitializeFuncPtrs()
{
    int32_t result = MIA_RETURN_OK;
    int numCharWritten = 0;

    m_hOpenCLLib = PluginUtils::libMap(pDefaultOpenCLLibraryName, OpenCLLibPath);

    if (NULL == m_hOpenCLLib) {
        MLOGE(Mia2LogGroupPlugin, "ERR! load %s%s.%s filed!", OpenCLLibPath,
              pDefaultOpenCLLibraryName, PluginSharedLibraryExtension);
        result = MIA_RETURN_UNABLE_LOAD;
    } else {
        m_pfnCLGetPlatformIDs = reinterpret_cast<PFNCLGETPLATFORMIDS>(
            PluginUtils::libGetAddr(m_hOpenCLLib, "clGetPlatformIDs"));
        m_pfnCLGetDeviceIDs = reinterpret_cast<PFNCLGETDEVICEIDS>(
            PluginUtils::libGetAddr(m_hOpenCLLib, "clGetDeviceIDs"));
        m_pfnCLCreateContext = reinterpret_cast<PFNCLCREATECONTEXT>(
            PluginUtils::libGetAddr(m_hOpenCLLib, "clCreateContext"));
        m_pfnCLReleaseContext = reinterpret_cast<PFNCLRELEASECONTEXT>(
            PluginUtils::libGetAddr(m_hOpenCLLib, "clReleaseContext"));

        m_pfnCLCreateCommandQueue = reinterpret_cast<PFNCLCREATECOMMANDQUEUE>(
            PluginUtils::libGetAddr(m_hOpenCLLib, "clCreateCommandQueue"));
        m_pfnCLReleaseCommandQueue = reinterpret_cast<PFNCLRELEASECOMMANDQUEUE>(
            PluginUtils::libGetAddr(m_hOpenCLLib, "clReleaseCommandQueue"));

        m_pfnCLCreateImage2D = reinterpret_cast<PFNCLCREATEIMAGE2D>(
            PluginUtils::libGetAddr(m_hOpenCLLib, "clCreateImage2D"));
        m_pfnCLCreateImage = reinterpret_cast<PFNCLCREATEIMAGE>(
            PluginUtils::libGetAddr(m_hOpenCLLib, "clCreateImage"));
        m_pfnCLReleaseMemObject = reinterpret_cast<PFNCLRELEASEMEMOBJECT>(
            PluginUtils::libGetAddr(m_hOpenCLLib, "clReleaseMemObject"));

        m_pfnCLCreateProgramWithSource = reinterpret_cast<PFNCLCREATEPROGRAMWITHSOURCE>(
            PluginUtils::libGetAddr(m_hOpenCLLib, "clCreateProgramWithSource"));

        m_pfnCLBuildProgram = reinterpret_cast<PFNCLBUILDPROGRAM>(
            PluginUtils::libGetAddr(m_hOpenCLLib, "clBuildProgram"));
        m_pfnCLReleaseProgram = reinterpret_cast<PFNCLRELEASEPROGRAM>(
            PluginUtils::libGetAddr(m_hOpenCLLib, "clReleaseProgram"));

        m_pfnCLGetProgramBuildInfo = reinterpret_cast<PFNCLGETPROGRAMBUILDINFO>(
            PluginUtils::libGetAddr(m_hOpenCLLib, "clGetProgramBuildInfo"));

        m_pfnCLCreateKernel = reinterpret_cast<PFNCLCREATEKERNEL>(
            PluginUtils::libGetAddr(m_hOpenCLLib, "clCreateKernel"));
        m_pfnCLReleaseKernel = reinterpret_cast<PFNRELEASEKERNEL>(
            PluginUtils::libGetAddr(m_hOpenCLLib, "clReleaseKernel"));
        m_pfnCLSetKernelArg = reinterpret_cast<PFNCLSETKERNELARG>(
            PluginUtils::libGetAddr(m_hOpenCLLib, "clSetKernelArg"));

        m_pfnCLEnqueueNDRangeKernel = reinterpret_cast<PFNCLENQUEUENDRANGEKERNEL>(
            PluginUtils::libGetAddr(m_hOpenCLLib, "clEnqueueNDRangeKernel"));

        m_pfnCLFlush =
            reinterpret_cast<PFNCLFLUSH>(PluginUtils::libGetAddr(m_hOpenCLLib, "clFlush"));
        m_pfnCLFinish =
            reinterpret_cast<PFNCLFINISH>(PluginUtils::libGetAddr(m_hOpenCLLib, "clFinish"));
        m_pfnCLGetDeviceInfo = reinterpret_cast<PFNCLGETDEVICEINFO>(
            PluginUtils::libGetAddr(m_hOpenCLLib, "clGetDeviceInfo"));
        m_pfnCLCreateBuffer = reinterpret_cast<PFNCLCREATEBUFFER>(
            PluginUtils::libGetAddr(m_hOpenCLLib, "clCreateBuffer"));
        m_pfnCLCreateSampler = reinterpret_cast<PFNCLCREATESAMPLER>(
            PluginUtils::libGetAddr(m_hOpenCLLib, "clCreateSampler"));
        m_pfnCLReleaseSampler = reinterpret_cast<PFNCLRELEASESAMPLER>(
            PluginUtils::libGetAddr(m_hOpenCLLib, "clReleaseSampler"));

        m_pfnCLEnqueueCopyBuffer = reinterpret_cast<PFNCLENQUEUECOPYBUFFER>(
            PluginUtils::libGetAddr(m_hOpenCLLib, "clEnqueueCopyBuffer"));
    }

    if ((NULL == m_pfnCLGetPlatformIDs) || (NULL == m_pfnCLGetDeviceIDs) ||
        (NULL == m_pfnCLCreateContext) || (NULL == m_pfnCLReleaseContext) ||
        (NULL == m_pfnCLCreateCommandQueue) || (NULL == m_pfnCLReleaseCommandQueue) ||
        (NULL == m_pfnCLCreateImage) || (NULL == m_pfnCLCreateImage2D) ||
        (NULL == m_pfnCLReleaseMemObject) || (NULL == m_pfnCLCreateProgramWithSource) ||
        (NULL == m_pfnCLBuildProgram) || (NULL == m_pfnCLReleaseProgram) ||
        (NULL == m_pfnCLGetProgramBuildInfo) || (NULL == m_pfnCLCreateKernel) ||
        (NULL == m_pfnCLReleaseKernel) || (NULL == m_pfnCLSetKernelArg) ||
        (NULL == m_pfnCLEnqueueNDRangeKernel) || (NULL == m_pfnCLFlush) ||
        (NULL == m_pfnCLFinish) || (NULL == m_pfnCLGetDeviceInfo) ||
        (NULL == m_pfnCLCreateBuffer) || (NULL == m_pfnCLCreateSampler) ||
        (NULL == m_pfnCLReleaseSampler)) {
        MLOGE(Mia2LogGroupPlugin,
              "Error Initializing one or more function pointers in Library: %s%s.%s", OpenCLLibPath,
              pDefaultOpenCLLibraryName, PluginSharedLibraryExtension);
        result = MIA_RETURN_INVALID_PARAM;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::Initialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t GPUOpenCL::Initialize()
{
    m_pOpenCLMutex = NULL;
    m_initStatus = CLInitRunning;

    int32_t result = MIA_RETURN_OK;
    cl_platform_id platform = NULL;
    cl_int error = CL_SUCCESS;
    cl_uint numPlatforms = 0;

    result = InitializeFuncPtrs();

    if (MIA_RETURN_OK == result) {
        // Get the platform ID
        error = m_pfnCLGetPlatformIDs(1, &platform, &numPlatforms);

        if (CL_SUCCESS != error) {
            MLOGE(Mia2LogGroupPlugin, "Error getting the platform ID: %d", error);
            result = MIA_RETURN_UNKNOWN_ERROR;
        }

        // Get the requested device
        if (MIA_RETURN_OK == result) {
            error = m_pfnCLGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &m_device, NULL);
            if (CL_SUCCESS != error) {
                MLOGE(Mia2LogGroupPlugin, "Error getting the requested Device: %d", error);
                result = MIA_RETURN_UNKNOWN_ERROR;
            }
        }

        if (MIA_RETURN_OK == result) {
            m_context = m_pfnCLCreateContext(NULL, 1, &m_device, NULL, NULL, &error);

            if ((NULL == m_context) || (CL_SUCCESS != error)) {
                MLOGE(Mia2LogGroupPlugin, "Error creating an OpenCL context: %d", error);
                result = MIA_RETURN_INVALID_CALL;
            }
        }

        if (MIA_RETURN_OK == result) {
            m_queue = m_pfnCLCreateCommandQueue(m_context, m_device, 0, &error);

            if ((NULL == m_queue) || (CL_SUCCESS != error)) {
                MLOGE(Mia2LogGroupPlugin, "Error creating the OpenCL Command Queue: %d", error);
                result = MIA_RETURN_INVALID_CALL;
            }
        }
    }

    if (MIA_RETURN_OK == result) {
        result = InitializeKernel();
    }

    if (MIA_RETURN_OK == result) {
        result = InitializeResources();
    }

    if (MIA_RETURN_OK == result) {
        // m_pOpenCLMutex = Mutex::Create("MIA OpenCLObject");
        m_pOpenCLMutex = new std::mutex();

        if (NULL == m_pOpenCLMutex) {
            result = MIA_RETURN_NO_MEM;
        }
    }

    if (MIA_RETURN_OK == result) {
        m_initStatus = CLInitDone;
    } else {
        m_initStatus = CLInitInvalid;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::InitializeResources
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t GPUOpenCL::InitializeResources()
{
    cl_weight_image_desc_qcom weightDesc;
    cl_image_format weightFmt;
    cl_half ds4Weights[2][8];
    cl_mem_ion_host_ptr ionMem = {{0}};
    cl_image_format input_1x1_image_format = {CL_RGBA, CL_UNSIGNED_INT32};
    cl_image_desc image_desc = {0};
    int32_t result = MIA_RETURN_OK;
    cl_int error = CL_FALSE;

    weightFmt.image_channel_data_type = CL_HALF_FLOAT;
    weightFmt.image_channel_order = CL_R;

    memset(&weightDesc, 0, sizeof(cl_weight_image_desc_qcom));
    weightDesc.image_desc.image_type = CL_MEM_OBJECT_WEIGHT_IMAGE_QCOM;

    weightDesc.image_desc.image_width = 8;
    weightDesc.image_desc.image_height = 8;
    weightDesc.image_desc.image_array_size = 1;
    // Specify separable filter, default (flags=0) is 2D convolution filter
    weightDesc.weight_desc.flags = CL_WEIGHT_IMAGE_SEPARABLE_QCOM;
    weightDesc.weight_desc.center_coord_x = 3;
    weightDesc.weight_desc.center_coord_y = 3;

    // Initialize weights
    static float ds4WeightsFloat[] = {(125.f / 1024.f), (91.f / 1024.f),  (144.f / 1024.f),
                                      (152.f / 1024.f), (152.f / 1024.f), (144.f / 1024.f),
                                      (91.f / 1024.f),  (125.f / 1024.f)};

    for (int i = 0; i < 8; i++) {
        ds4Weights[0][i] = (cl_half)PluginUtils::floatToHalf(ds4WeightsFloat[i]);
        ds4Weights[1][i] = (cl_half)PluginUtils::floatToHalf(ds4WeightsFloat[i]);
    }

    m_ds4WeightsImage =
        m_pfnCLCreateImage(m_context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, &weightFmt,
                           (cl_image_desc *)&weightDesc, (void *)ds4Weights, &error);

    if (CL_SUCCESS != error) {
        MLOGE(Mia2LogGroupPlugin, "Failed to create ds4 weights image with error %d", error);
        result = MIA_RETURN_UNKNOWN_ERROR;
    }

    if (MIA_RETURN_OK == result) {
        // Create sampler
        m_ds4Sampler = m_pfnCLCreateSampler(m_context, CL_FALSE, CL_ADDRESS_CLAMP_TO_EDGE,
                                            CL_FILTER_NEAREST, &error);

        if (CL_SUCCESS != error) {
            MLOGE(Mia2LogGroupPlugin, "Failed to create ds4 sampler with error %d", error);
            result = MIA_RETURN_UNKNOWN_ERROR;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::InitializeKernel
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t GPUOpenCL::InitializeKernel()
{
    int32_t result = MIA_RETURN_OK;
    cl_int err = CL_FALSE;

    m_program = m_pfnCLCreateProgramWithSource(
        m_context, 1, reinterpret_cast<const char **>(&m_pProgramSource), NULL, &err);

    if (CL_SUCCESS != err) {
        MLOGE(Mia2LogGroupPlugin, "clCreateProgramWithSource failed: error: %d", err);
        result = MIA_RETURN_UNKNOWN_ERROR;
    }

    if (MIA_RETURN_OK == result) {
        err = m_pfnCLBuildProgram(m_program, 1, &m_device, NULL, NULL, NULL);

        if (CL_SUCCESS != err) {
            char log[512] = "\0";
            m_pfnCLGetProgramBuildInfo(m_program, m_device, CL_PROGRAM_BUILD_LOG, sizeof(log), log,
                                       NULL);
            MLOGE(Mia2LogGroupPlugin, "clBuildProgram failed: error: %d, Log: %s", err, log);
            result = MIA_RETURN_UNKNOWN_ERROR;
        }
    }

    if (MIA_RETURN_OK == result) {
        // Create the copy kernel
        m_copyImageKernel = m_pfnCLCreateKernel(m_program, "copyImage", &err);

        if (CL_SUCCESS != err) {
            if (NULL != m_copyImageKernel) {
                m_pfnCLReleaseKernel(m_copyImageKernel);
                m_copyImageKernel = NULL;
            }

            MLOGE(Mia2LogGroupPlugin, "clCreateKernel for the CopyImage Kernel failed: error: %d",
                  err);
            result = MIA_RETURN_UNKNOWN_ERROR;
        }
    }

    if (MIA_RETURN_OK == result) {
        // Create the rotation kernel
        m_rotateImageKernel = m_pfnCLCreateKernel(m_program, "rotateImage", &err);

        if (CL_SUCCESS != err) {
            if (NULL != m_rotateImageKernel) {
                m_pfnCLReleaseKernel(m_rotateImageKernel);
                m_rotateImageKernel = NULL;
            }

            MLOGE(Mia2LogGroupPlugin, "clCreateKernel for the RotateImage Kernel failed: error: %d",
                  err);
            result = MIA_RETURN_UNKNOWN_ERROR;
        }
    }

    if (MIA_RETURN_OK == result) {
        // Create the flip kernel
        m_flipImageKernel = m_pfnCLCreateKernel(m_program, "flipImage", &err);

        if (CL_SUCCESS != err) {
            if (NULL != m_flipImageKernel) {
                m_pfnCLReleaseKernel(m_flipImageKernel);
                m_flipImageKernel = NULL;
            }

            MLOGE(Mia2LogGroupPlugin, "clCreateKernel for the FlipImage Kernel failed: error: %d",
                  err);
            result = MIA_RETURN_UNKNOWN_ERROR;
        }
    }

    if (MIA_RETURN_OK == result) {
        // Create the ds4 kernel
        m_ds4SinglePlaneKernel = m_pfnCLCreateKernel(m_program, "ds4_single_plane", &err);

        if (CL_SUCCESS != err) {
            if (NULL != m_ds4SinglePlaneKernel) {
                m_pfnCLReleaseKernel(m_ds4SinglePlaneKernel);
                m_ds4SinglePlaneKernel = NULL;
            }

            MLOGE(Mia2LogGroupPlugin,
                  "clCreateKernel for the DS4SinglePlane Kernel failed: error: %d", err);
            result = MIA_RETURN_UNKNOWN_ERROR;
        }
    }

    if (MIA_RETURN_OK == result) {
        // Create the box filter kernel
        m_boxFilterSinglePlaneKernel =
            m_pfnCLCreateKernel(m_program, "boxfilter_single_plane", &err);

        if (CL_SUCCESS != err) {
            if (NULL != m_boxFilterSinglePlaneKernel) {
                m_pfnCLReleaseKernel(m_boxFilterSinglePlaneKernel);
                m_boxFilterSinglePlaneKernel = NULL;
            }

            MLOGE(Mia2LogGroupPlugin,
                  "clCreateKernel for the BoxFilterSinglePlane Kernel failed: error: %d", err);
            result = MIA_RETURN_UNKNOWN_ERROR;
        }
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::ExecuteFlipImage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t GPUOpenCL::ExecuteFlipImage(cl_mem dst, cl_mem src, uint32_t width, uint32_t height,
                                    uint32_t inStride, uint32_t outStride, uint32_t inUVOffset,
                                    uint32_t outUVOffset, FlipDirection direction)
{
    int32_t result = MIA_RETURN_OK;
    cl_int err = CL_SUCCESS;
    size_t globalSize[2] = {width, height};

    if (CL_SUCCESS == err) {
        err = m_pfnCLSetKernelArg(m_flipImageKernel, 0, sizeof(cl_mem), &dst);
    }

    if (CL_SUCCESS == err) {
        err = m_pfnCLSetKernelArg(m_flipImageKernel, 1, sizeof(cl_mem), &src);
    }

    if (CL_SUCCESS == err) {
        err = m_pfnCLSetKernelArg(m_flipImageKernel, 2, sizeof(uint32_t), &width);
    }

    if (CL_SUCCESS == err) {
        err = m_pfnCLSetKernelArg(m_flipImageKernel, 3, sizeof(uint32_t), &height);
    }

    if (CL_SUCCESS == err) {
        err = m_pfnCLSetKernelArg(m_flipImageKernel, 4, sizeof(uint32_t), &inStride);
    }

    if (CL_SUCCESS == err) {
        err = m_pfnCLSetKernelArg(m_flipImageKernel, 5, sizeof(uint32_t), &outStride);
    }

    if (CL_SUCCESS == err) {
        err = m_pfnCLSetKernelArg(m_flipImageKernel, 6, sizeof(uint32_t), &inUVOffset);
    }

    if (CL_SUCCESS == err) {
        err = m_pfnCLSetKernelArg(m_flipImageKernel, 7, sizeof(uint32_t), &outUVOffset);
    }

    if (CL_SUCCESS == err) {
        err = m_pfnCLSetKernelArg(m_flipImageKernel, 8, sizeof(uint32_t), &direction);
    }

    if (CL_SUCCESS == err) {
        err = m_pfnCLEnqueueNDRangeKernel(m_queue, m_flipImageKernel, 2, NULL, &globalSize[0], NULL,
                                          0, NULL, NULL);
    }

    if (CL_SUCCESS != err) {
        MLOGE(Mia2LogGroupPlugin, "ExecuteFlipImage failed: error: %d", err);
        result = MIA_RETURN_UNKNOWN_ERROR;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::FlipImage
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t GPUOpenCL::FlipImage(std::vector<ImageParams> &hOutput, std::vector<ImageParams> &hInput,
                             FlipDirection direction)
{
    int32_t result = MIA_RETURN_OK;
    cl_int err = CL_SUCCESS;

    /// @note This code only works for Linear NV12 at the moment.
    for (unsigned int i = 0; i < hOutput.size(); i++) {
        cl_mem dstYBuffer = 0;
        cl_mem srcYBuffer = 0;
        cl_mem dstUVBuffer = 0;
        cl_mem srcUVBuffer = 0;
        uint32_t width = hInput.at(i).format.width;
        uint32_t height = hInput.at(i).format.height;
        uint32_t inStride = hInput.at(i).planeStride;
        uint32_t outStride = hOutput.at(i).planeStride;
        uint32_t inUVOffset = 0;
        uint32_t outUVOffset = 0;

        MLOGI(Mia2LogGroupPlugin, "Input width: %d, inStride is %d, output stride is %d ", width,
              inStride, outStride);

        cl_mem_ion_host_ptr ionmemDstY = {{0}};
        ionmemDstY.ext_host_ptr.allocation_type = CL_MEM_ION_HOST_PTR_QCOM;
        ionmemDstY.ext_host_ptr.host_cache_policy = CL_MEM_HOST_UNCACHED_QCOM;
        ionmemDstY.ion_filedesc = hOutput.at(i).fd[0];
        ionmemDstY.ion_hostptr = hOutput.at(i).pAddr[0];

        /// @todo (CAMX-2286) Improvements to GPU Node Support: Need to avoid calling Create and
        /// Release
        ///                   ION buffers per-process request
        dstYBuffer =
            m_pfnCLCreateBuffer(m_context, (CL_MEM_USE_HOST_PTR | CL_MEM_EXT_HOST_PTR_QCOM),
                                (inStride * height), reinterpret_cast<void *>(&ionmemDstY), &err);
        if (CL_SUCCESS != err) {
            MLOGE(Mia2LogGroupPlugin,
                  "GPUOpenCL::FlipImage failed to create clBuffer dstY: error: %d", err);
        }

        cl_mem_ion_host_ptr ionmemDstUV = {{0}};
        ionmemDstUV.ext_host_ptr.allocation_type = CL_MEM_ION_HOST_PTR_QCOM;
        ionmemDstUV.ext_host_ptr.host_cache_policy = CL_MEM_HOST_UNCACHED_QCOM;
        ionmemDstUV.ion_filedesc = hOutput.at(i).fd[1];
        ionmemDstUV.ion_hostptr = hOutput.at(i).pAddr[1];

        dstUVBuffer = m_pfnCLCreateBuffer(
            m_context, (CL_MEM_USE_HOST_PTR | CL_MEM_EXT_HOST_PTR_QCOM), (inStride * height / 2),
            reinterpret_cast<void *>(&ionmemDstUV), &err);
        if (CL_SUCCESS != err) {
            MLOGE(Mia2LogGroupPlugin,
                  "GPUOpenCL::FlipImage failed to create clBuffer dstUV: error: %d", err);
        }

        MLOGI(Mia2LogGroupPlugin, "dstYBuffer = %p, dstUVBuffer = %p, fd: %d,%d", dstYBuffer,
              dstUVBuffer, hOutput.at(i).fd[0], hOutput.at(i).fd[1]);

        cl_mem_ion_host_ptr ionmemSrcY = {{0}};
        ionmemSrcY.ext_host_ptr.allocation_type = CL_MEM_ION_HOST_PTR_QCOM;
        ionmemSrcY.ext_host_ptr.host_cache_policy = CL_MEM_HOST_UNCACHED_QCOM;
        ionmemSrcY.ion_filedesc = hInput.at(i).fd[0];
        ionmemSrcY.ion_hostptr = hInput.at(i).pAddr[0];

        srcYBuffer =
            m_pfnCLCreateBuffer(m_context, (CL_MEM_USE_HOST_PTR | CL_MEM_EXT_HOST_PTR_QCOM),
                                (inStride * height), reinterpret_cast<void *>(&ionmemSrcY), &err);
        if (CL_SUCCESS != err) {
            MLOGE(Mia2LogGroupPlugin,
                  "GPUOpenCL::FlipImage failed to create clBuffer srcY: error: %d", err);
        }

        cl_mem_ion_host_ptr ionmemSrcUV = {{0}};
        ionmemSrcUV.ext_host_ptr.allocation_type = CL_MEM_ION_HOST_PTR_QCOM;
        ionmemSrcUV.ext_host_ptr.host_cache_policy = CL_MEM_HOST_UNCACHED_QCOM;
        ionmemSrcUV.ion_filedesc = hInput.at(i).fd[1];
        ionmemSrcUV.ion_hostptr = hInput.at(i).pAddr[1];

        srcUVBuffer = m_pfnCLCreateBuffer(
            m_context, (CL_MEM_USE_HOST_PTR | CL_MEM_EXT_HOST_PTR_QCOM), (inStride * height / 2),
            reinterpret_cast<void *>(&ionmemSrcUV), &err);
        if (CL_SUCCESS != err) {
            MLOGE(Mia2LogGroupPlugin,
                  "GPUOpenCL::FlipImage failed to create clBuffer srcUV: error: %d", err);
            result = MIA_RETURN_UNKNOWN_ERROR;
        }

        MLOGI(Mia2LogGroupPlugin, "srcYBuffer = %p, srcUVBuffer = %p, fd: %d,%d", srcYBuffer,
              srcUVBuffer, hInput.at(i).fd[0], hInput.at(i).fd[1]);

        m_pOpenCLMutex->lock();
        if (MIA_RETURN_OK == result) {
            if ((NULL != dstYBuffer) && (NULL != srcYBuffer)) {
                result = ExecuteFlipImage(dstYBuffer, srcYBuffer, width, height, inStride,
                                          outStride, 0, 0, direction);
            }
        }

        if (MIA_RETURN_OK == result) {
            if ((NULL != dstUVBuffer) && (NULL != srcUVBuffer)) {
                inUVOffset = hInput.at(i).pAddr[1] - hInput.at(i).pAddr[0];
                outUVOffset = hOutput.at(i).pAddr[1] - hOutput.at(i).pAddr[0];
                result = ExecuteFlipImage(dstUVBuffer, srcUVBuffer, (width >> 1), (height >> 1),
                                          inStride, outStride, inUVOffset, outUVOffset, direction);
            }
        }

        /// @todo (CAMX-2286) Improvements to GPU Node Support: We shouldn't be calling clFinish
        /// here. Instead, we need to use
        ///                   events and spawn a thread to wait on those events, and then signal the
        ///                   buffer to unblock the next node in the pipeline.
        if (MIA_RETURN_OK == result) {
            err = m_pfnCLFinish(m_queue);
        }
        m_pOpenCLMutex->unlock();

        if (CL_SUCCESS != err) {
            result = MIA_RETURN_UNKNOWN_ERROR;
        }

        if (0 != dstYBuffer) {
            m_pfnCLReleaseMemObject(dstYBuffer);
        }
        if (0 != dstUVBuffer) {
            m_pfnCLReleaseMemObject(dstUVBuffer);
        }
        if (0 != srcYBuffer) {
            m_pfnCLReleaseMemObject(srcYBuffer);
        }
        if (0 != srcUVBuffer) {
            m_pfnCLReleaseMemObject(srcUVBuffer);
        }
    }

    if (MIA_RETURN_OK != result) {
        MLOGE(Mia2LogGroupPlugin, "GPUOpenCL::FlipImage failed: error: %d", result);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// GPUOpenCL::Uninitialize
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t GPUOpenCL::Uninitialize()
{
    int32_t result = MIA_RETURN_OK;

    cl_int err = CL_SUCCESS;

    if (NULL != m_queue) {
        err = m_pfnCLReleaseCommandQueue(m_queue);
        MLOGE(Mia2LogGroupPlugin, "GPUOpenCL ReleaseCommandQueue error -0 means success-: %d", err);
        m_queue = NULL;
    }

    if (NULL != m_context) {
        err = m_pfnCLReleaseContext(m_context);
        MLOGE(Mia2LogGroupPlugin, "GPUOpenCL ReleaseContext error -0 means success-: %d", err);
        m_context = NULL;
    }

    if (NULL != m_copyImageKernel) {
        m_pfnCLReleaseKernel(m_copyImageKernel);
        m_copyImageKernel = NULL;
    }

    if (NULL != m_rotateImageKernel) {
        m_pfnCLReleaseKernel(m_rotateImageKernel);
        m_rotateImageKernel = NULL;
    }

    if (NULL != m_flipImageKernel) {
        m_pfnCLReleaseKernel(m_flipImageKernel);
        m_flipImageKernel = NULL;
    }

    if (NULL != m_ds4SinglePlaneKernel) {
        m_pfnCLReleaseKernel(m_ds4SinglePlaneKernel);
        m_ds4SinglePlaneKernel = NULL;
    }

    if (NULL != m_boxFilterSinglePlaneKernel) {
        m_pfnCLReleaseKernel(m_boxFilterSinglePlaneKernel);
        m_boxFilterSinglePlaneKernel = NULL;
    }

    if (NULL != m_ds4WeightsImage) {
        m_pfnCLReleaseMemObject(m_ds4WeightsImage);
        m_ds4WeightsImage = NULL;
    }

    if (NULL != m_ds4Sampler) {
        m_pfnCLReleaseSampler(m_ds4Sampler);
        m_ds4Sampler = NULL;
    }

    if (NULL != m_program) {
        m_pfnCLReleaseProgram(m_program);
        m_program = NULL;
    }

    if (NULL != m_hOpenCLLib) {
        result = PluginUtils::libUnmap(m_hOpenCLLib);
        m_hOpenCLLib = NULL;
    }

    if (NULL != m_pOpenCLMutex) {
        delete m_pOpenCLMutex;
        m_pOpenCLMutex = NULL;
    }

    if (MIA_RETURN_OK != result) {
        MLOGE(Mia2LogGroupPlugin, "GPUOpenCL::Uninitialize Failed to unmap lib: %d", result);
    }

    m_initStatus = CLInitInvalid;

    return result;
}

// =============================================================================================================================
// END OpenCL Stuff
// =============================================================================================================================
