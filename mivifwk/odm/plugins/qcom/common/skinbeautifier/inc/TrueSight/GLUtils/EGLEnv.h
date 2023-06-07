#pragma once
#include <TrueSight/TrueSightDefine.hpp>
#include <TrueSight/TrueSightPlatform.hpp>
#include <cstdio>
#include "TSGLDefine.h"

#ifdef TRUESIGHT_PLATFORM_ANDROID
#define GL_CONTEXT EGLContext
#define GL_NO_CONTEXT EGL_NO_CONTEXT
#elif TRUESIGHT_PLATFORM_WINDOWS
#include <Windows.h>
#define GL_CONTEXT HGLRC
#define GL_NO_CONTEXT NULL
#elif TRUESIGHT_PLATFORM_IOS || TRUESIGHT_PLATFORM_MAC_OS
#define GL_CONTEXT void*
#define GL_NO_CONTEXT NULL
#else
#define GL_CONTEXT
#define GL_NO_CONTEXT
#endif

namespace TrueSight {

class EGLEnvInner;
class TrueSight_PUBLIC EGLEnv {
public:
    EGLEnv();
    ~EGLEnv();

    ///-------------------------------------------------------------------------------------------------
    /// @fn	bool EGLEnvionment::BeginRender(int nEnvWidth, int nEnvHeight, EGLEnvionment* pShareEnv = nullptr);
    ///
    /// @brief	Begins a render.
    ///
    /// @date	2020/2/19
    ///
    /// @param 		   	nEnvWidth 	Width of the environment.
    /// @param 		   	nEnvHeight	Height of the environment.
    /// @param [in,out]	pShareEnv 	(Optional) If non-null, the share environment.
    ///
    /// @return	True if it succeeds, false if it fails.
    ///-------------------------------------------------------------------------------------------------
    bool BeginRender(int nEnvWidth, int nEnvHeight, EGLEnv* pShareEnv = nullptr);
    bool BeginRender(int nEnvWidth, int nEnvHeight, GL_CONTEXT shareContext);

#if defined(ANDROID) || defined(__ANDROID__)
    EGLDisplay GetDisplay();
#endif

    bool EglMakeCurrent_Push();
    bool EglMakeCurrent_Pop();

    ///-------------------------------------------------------------------------------------------------
    /// @fn	void EGLEnvionment::EndRender();
    ///
    /// @brief	Ends a render.
    ///
    /// @date	2020/2/19
    ///-------------------------------------------------------------------------------------------------
    void EndRender();

    ///-------------------------------------------------------------------------------------------------
    /// @fn	static GL_CONTEXT EGLEnvionment::GetCurrentContext();
    ///
    /// @brief	Gets current context.
    ///
    /// @date	2020/2/19
    ///
    /// @return	The current context.
    ///-------------------------------------------------------------------------------------------------
    static GL_CONTEXT GetCurrentContext();

private:
    EGLEnvInner *inner_ = nullptr;
};

class EGLEnvInner {
    friend EGLEnv;
private:
    EGLEnvInner();
    ~EGLEnvInner();

private:
#ifdef TRUESIGHT_PLATFORM_ANDROID
    EGLDisplay m_Display = EGL_NO_DISPLAY;
    EGLSurface m_Surface = EGL_NO_SURFACE;
#elif TRUESIGHT_PLATFORM_WINDOWS
    HWND m_Windows = NULL;
    HDC m_dc;
#endif
    GL_CONTEXT m_Context;
    int m_nWidth = 0;
    int m_nHeight = 0;
};

} // TrueSight
