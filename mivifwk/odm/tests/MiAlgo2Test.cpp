#undef LOG_TAG
#define LOG_TAG "MIA2TEST"

#include <MiAlgo2Test.h>
#include <unistd.h>
using namespace mialgo2;

void initCreateParams(PostProcCreateParams &createParams, StreamConfigMode mode)
{
    createParams.processMode = MiaPostProcMode::OfflineSnapshot;
    createParams.operationMode = mode;
    createParams.inputFormat.resize(1);
    createParams.outputFormat.resize(1);
    createParams.inputFormat[0].format = gTestFormat;
    createParams.inputFormat[0].width = gTestWidth;
    createParams.inputFormat[0].height = gTestHeight;
    createParams.outputFormat[0].format = gTestFormat;
    createParams.outputFormat[0].width = gTestWidth;
    createParams.outputFormat[0].height = gTestHeight;
    createParams.sessionCb = new MySessionCb2();
}

bool initEncParams(PostProcSessionParams &encParams)
{
    MiaFrame inFrame, outFrame;
    MiaBatchedImage *inImage = initMiaBatchedImage(fileNameIn, true);
    MiaBatchedImage *outImage = initMiaBatchedImage(fileNameOut, false);
    if ((!inImage) || (!outImage)) {
        MLOGE(Mia2LogGroupCore, "Failed to init input or output image");
        return false;
    }

    inFrame.imageVec.push_back(*inImage);
    outFrame.imageVec.push_back(*outImage);
    free(inImage);
    free(outImage);

    inFrame.frame_number = 1;
    inFrame.streamId = 0;
    outFrame.frame_number = 1;
    outFrame.streamId = 0;
    inFrame.callback = new MiFrameProcessorCallback2();
    outFrame.callback = new MiFrameProcessorCallback2();

    encParams.inHandle = inFrame;
    encParams.outHandle = outFrame;
    encParams.streamId = 0;
    encParams.frameNum = 1;
    return true;
}

int main(void)
{
    int res = sem_init(&sem, 0, 0);
    if (res == -1) {
        MLOGI(Mia2LogGroupCore, "semaphore intitialization failed");
        exit(EXIT_FAILURE);
    }
    funcInit();
    // MiFrameType type = SnapshotType;
    PostProcCreateParams createParams;
    PostProcSessionParams encParams;

    // change the test case here with the operation mode
    // for example StreamConfigModeNormal
    initCreateParams(createParams, StreamConfigModeTestMemcpy);

    void *offlineSession = pCameraPostProcCreate(&createParams);
    CDKResult procResult = MIAS_OK;

    if (offlineSession == NULL) {
        MLOGE(Mia2LogGroupCore, "Create CameraPostProc failed");
        return 0;
    } else {
        MLOGI(Mia2LogGroupCore, "Create CameraPostProc succeed");
    }

    if (!initEncParams(encParams)) {
        return 0;
    }
    procResult = pCameraPostProcProcess(offlineSession, &encParams);
    if (procResult == MIAS_OK) {
        MLOGI(Mia2LogGroupCore, "Send CameraPostProc succeed");
    } else {
        MLOGE(Mia2LogGroupCore, "Send CameraPostProc failed");
    }

    MLOGI(Mia2LogGroupCore, "Process wait.............");
    sem_wait(&sem);

    MLOGI(Mia2LogGroupCore, "CameraPostProcess succeed");
    // release
    MiaFrame inFrame = encParams.inHandle;
    MiaFrame outFrame = encParams.outHandle;
    MiaBatchedImage inImage = inFrame.imageVec[0];
    MiaBatchedImage outImage = outFrame.imageVec[0];

    if (inImage.pBuffer.pAddr[0]) {
        free(inImage.pBuffer.pAddr[0]);
        inImage.pBuffer.pAddr[0] = NULL;
        inImage.pBuffer.pAddr[1] = NULL;
    }

    FILE *p;
    p = fopen(fileNameOut, "wb");
    if (p == NULL) {
        MLOGE(Mia2LogGroupCore, "Open out file failed!");
    } else {
        miReadWritePlane(p, outImage.pBuffer.pAddr[0], outImage.format.width,
                         outImage.format.height, outImage.format.planeStride, 0);
        miReadWritePlane(p, outImage.pBuffer.pAddr[1], outImage.format.width,
                         outImage.format.height / 2, outImage.format.planeStride, 0);
        fclose(p);
        printf("to %s \n", fileNameOut);
    }
    free(outImage.pBuffer.pAddr[0]);
    outImage.pBuffer.pAddr[0] = NULL;
    outImage.pBuffer.pAddr[1] = NULL;
    delete inFrame.callback;
    delete outFrame.callback;

    pCameraPostProcDestroy(offlineSession);
    offlineSession = NULL;

    MLOGI(Mia2LogGroupCore, "Destroy CameraPostProc succeed");
    return 0;
}
