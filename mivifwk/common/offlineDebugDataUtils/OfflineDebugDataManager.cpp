#include "OfflineDebugDataManager.h"

void *OfflineDebugDataManager::getDebugDataBufferAndAddReference(size_t oriSize)
{
    if (!mIsMihalHasSession)
        setSessionStatus(true);

    std::lock_guard<std::mutex> lock(mArrayMutex);
    if (!mReclaimThreadActive) {
        mReclaimThread = std::thread([this]() {
            MLOGW(Mia2LogGroupDebug, "[Offline DebugData]: Reclaim thread %p start",
                  &mReclaimThread);
            offlineDebugDataReclaim();
        });

        mReclaimThreadActive = true;
        mReclaimThread.detach();
    }

    for (int index = 0; index < MaxNumOfflineDebugData; ++index) {
        OfflineDebugData *offlineDebugData = mOfflineDebugDataArr + index;
        if (offlineDebugData->debugData.pData != NULL &&
            offlineDebugData->debugData.size == oriSize && offlineDebugData->referenceCount == 0) {
            // get free buffer
            memset(offlineDebugData->debugData.pData, 0, oriSize);
            ++(offlineDebugData->referenceCount);

            MLOGI(Mia2LogGroupDebug,
                  "[Offline DebugData]: array[%d/%d] debugData = %p pData = %p,"
                  " bufferSize = %zu, referenceCount = %d",
                  index, MaxNumOfflineDebugData, &(offlineDebugData->debugData),
                  offlineDebugData->debugData.pData, oriSize, offlineDebugData->referenceCount);
            return &(offlineDebugData->debugData);
        } else if (offlineDebugData->debugData.pData == NULL &&
                   offlineDebugData->referenceCount == 0) {
            offlineDebugData->debugData.pData = malloc(oriSize);

            if (offlineDebugData->debugData.pData != NULL) {
                memset(offlineDebugData->debugData.pData, 0, oriSize);
                offlineDebugData->debugData.size = oriSize;
                ++(offlineDebugData->referenceCount);

                MLOGI(Mia2LogGroupDebug,
                      "[Offline DebugData]: array[%d/%d] debugData = %p new alloc pData = %p,"
                      " bufferSize = %zu, referenceCount = %d",
                      index, MaxNumOfflineDebugData, &(offlineDebugData->debugData),
                      offlineDebugData->debugData.pData, oriSize, offlineDebugData->referenceCount);

                return &(offlineDebugData->debugData);
            } else {
                offlineDebugData->debugData.size = 0;
                MLOGW(Mia2LogGroupDebug,
                      "[Offline DebugData]:No memory alloc for xiaomi offline debugdata");
            }
        }
    }

    // If no same size free buffer or no enough space for alloc, reclaim
    // ref 0 buffer space.
    for (int index = 0; index < MaxNumOfflineDebugData; ++index) {
        OfflineDebugData *offlineDebugData = mOfflineDebugDataArr + index;
        if (offlineDebugData->debugData.pData != NULL && offlineDebugData->referenceCount == 0) {
            // reclaim last debugdata pdata space]
            free(offlineDebugData->debugData.pData);
            offlineDebugData->debugData.pData = NULL;
            offlineDebugData->debugData.size = 0;

            offlineDebugData->debugData.pData = malloc(oriSize);

            if (offlineDebugData->debugData.pData != NULL) {
                memset(offlineDebugData->debugData.pData, 0, oriSize);
                offlineDebugData->debugData.size = oriSize;
                ++(offlineDebugData->referenceCount);
                MLOGI(Mia2LogGroupDebug,
                      "[Offline DebugData]: array[%d/%d] debugData = %p new alloc pData = %p,"
                      " bufferSize = %zu, referenceCount = %d",
                      index, MaxNumOfflineDebugData, &(offlineDebugData->debugData),
                      offlineDebugData->debugData.pData, oriSize, offlineDebugData->referenceCount);

                return &(offlineDebugData->debugData);
            } else {
                MLOGW(Mia2LogGroupDebug, "No memory alloc for xiaomi offline debugdata");
            }
        }
    }

    return NULL;
}

uint32_t OfflineDebugDataManager::subDebugDataReference(void *pDebugData)
{
    uint32_t result = ResultSuccess;
    if (pDebugData == nullptr) {
        MLOGE(Mia2LogGroupDebug, "pDebugData is Null");
        return ResultEFailed;
    }

    DebugData *temptr = reinterpret_cast<DebugData *>(pDebugData);

    MLOGV(Mia2LogGroupDebug, "[Offline DebugData]: debugData.pData %p", temptr->pData);

    std::lock_guard<std::mutex> lock(mArrayMutex);

    for (int index = 0; index < MaxNumOfflineDebugData; ++index) {
        if (temptr->pData == mOfflineDebugDataArr[index].debugData.pData) {
            OfflineDebugData *offlineDebugData = mOfflineDebugDataArr + index;
            if (0 < offlineDebugData->referenceCount) {
                --(offlineDebugData->referenceCount);

                MLOGI(Mia2LogGroupDebug,
                      "[Offline DebugData]: sub referenceCount of array[%d/%d] debugData = "
                      "%p, pData = %p, after sub is %d",
                      index, MaxNumOfflineDebugData, &(offlineDebugData->debugData),
                      offlineDebugData->debugData.pData, offlineDebugData->referenceCount);
            } else {
                MLOGE(Mia2LogGroupDebug,
                      "[Offline DebugData]: array[%d/%d] debugData = %p, pData = %p"
                      "referenceCount is %d, cannot sub",
                      index, MaxNumOfflineDebugData, &(offlineDebugData->debugData),
                      offlineDebugData->debugData.pData, offlineDebugData->referenceCount);
                result = ResultEFailed;
            }
            break;
        } else if (MaxNumOfflineDebugData - 1 == index) {
            MLOGE(Mia2LogGroupDebug,
                  "[Offline DebugData]: cannot find debugData = %p, pData = %p, sub failed", temptr,
                  temptr->pData);
            result = ResultEFailed;
        }
    }

    return result;
}

void OfflineDebugDataManager::offlineDebugDataReclaim()
{
    while (true) {
        std::unique_lock<std::mutex> condLock(mCondMutex);
        std::cv_status waitstatus =
            mReclaimCond.wait_for(condLock, std::chrono::seconds(WaitForReclaimDebugDataInSeconds));

        if (waitstatus != std::cv_status::timeout && !mIsMihalHasSession && !mIsVTOpen) {
            continue;
        }

        if (waitstatus == std::cv_status::timeout && !mIsMihalHasSession && !mIsVTOpen) {
            std::lock_guard<std::mutex> arraylock(mArrayMutex);
            for (int index = 0; index < MaxNumOfflineDebugData; ++index) {
                OfflineDebugData *offlineDebugData = mOfflineDebugDataArr + index;
                if (nullptr != offlineDebugData->debugData.pData) {
                    if (offlineDebugData->referenceCount <= 0) {
                        MLOGI(Mia2LogGroupDebug,
                              "[Offline DebugData]: array[%d/%d] Reclaim pData %p", index,
                              MaxNumOfflineDebugData, offlineDebugData->debugData.pData);
                    } else {
                        MLOGW(Mia2LogGroupDebug,
                              "[Offline DebugData]: array[%d/%d] Reclaim pData %p, referenceCount "
                              "%d, maybe thumbnail",
                              index, MaxNumOfflineDebugData, offlineDebugData->debugData.pData,
                              offlineDebugData->referenceCount);
                    }
                    free(offlineDebugData->debugData.pData);
                    offlineDebugData->debugData.pData = nullptr;
                    offlineDebugData->debugData.size = 0;
                    offlineDebugData->referenceCount = 0;
                }
            }
            MLOGW(Mia2LogGroupDebug, "[Offline DebugData]: Reclaim thread join");
            mReclaimThreadActive = false;
            return;
        }

        std::lock_guard<std::mutex> arraylock(mArrayMutex);
        for (int index = 0; index < MaxNumOfflineDebugData; ++index) {
            OfflineDebugData *offlineDebugData = mOfflineDebugDataArr + index;
            if (0 == offlineDebugData->referenceCount &&
                nullptr != offlineDebugData->debugData.pData) {
                MLOGI(Mia2LogGroupDebug, "[Offline DebugData]: array[%d/%d] Reclaim pData %p",
                      index, MaxNumOfflineDebugData, offlineDebugData->debugData.pData);
                free(offlineDebugData->debugData.pData);
                offlineDebugData->debugData.pData = nullptr;
                offlineDebugData->debugData.size = 0;
            } else if (0 < offlineDebugData->referenceCount &&
                       offlineDebugData->debugData.pData == nullptr) {
                MLOGI(Mia2LogGroupDebug,
                      "[Offline DebugData]: Exception case, set reference "
                      "%d to zero",
                      offlineDebugData->referenceCount);
                offlineDebugData->referenceCount = 0;
                offlineDebugData->debugData.size = 0;
            }
        }
    }
}
