#pragma once

#include <TrueSight/TrueSightDefine.hpp>

namespace TrueSight {

typedef int(*FuncAssetReadData)(const char *filePath, unsigned char **ptr, int *dataLen);
typedef void(*FuncAssetFreeBuffer)(unsigned char **ptr);
typedef bool(*FuncAssetCheckFileExist)(const char *filePath);

/**
 * @clase 反射调用文件读取方式，目前主要用于支持android assets读取
 */
class TrueSight_PUBLIC FileManager {
public:
    static void SetCallBackAssetsReadData(FuncAssetReadData ptr);
    static void SetCallBackAssetsFreeBuffer(FuncAssetFreeBuffer ptr);
    static void SetCallBackAssetsCheckFileExist(FuncAssetCheckFileExist ptr);

    static FuncAssetReadData GetCallBackAssetsReadData();
    static FuncAssetFreeBuffer GetCallBackAssetsFreeBuffer();
    static FuncAssetCheckFileExist GetCallBackAssetsCheckFileExist();

private:
    static FuncAssetReadData g_pAssetsReadData;
    static FuncAssetFreeBuffer g_pAssetsFreeBuffer;
    static FuncAssetCheckFileExist g_pAssetsCheckFileExist;
};
}
