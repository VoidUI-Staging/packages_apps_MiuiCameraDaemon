#pragma once

#include <TrueSight/TrueSightPlatform.hpp>
#include <TrueSight/TrueSightConfig.hpp>

#if TRUESIGHT_PLATFORM_ANDROID

#define GL_GLEXT_PROTOTYPES
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#if TrueSight_DYNAMIC_GLESv3
#include "GLUtils/android/gl3stub.h"
#else
#if __ANDROID_API__ >= 24
#include <GLES3/gl32.h>
#elif __ANDROID_API__ >= 21
#include <GLES3/gl31.h>
#else
#include <GLES3/gl3.h>
#endif
#include <GLES3/gl3ext.h>
#endif

#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <EGL/eglplatform.h>

#define GL_OPENGLES 1

#elif TRUESIGHT_PLATFORM_WINDOWS
#include <gl/glew.h>
#elif TRUESIGHT_PLATFORM_MAC_OS
#define GL_SILENCE_DEPRECATION
#import <OpenGL/gl.h>
#import <OpenGL/gl3.h>
#import <OpenGL/OpenGL.h>
#elif TRUESIGHT_PLATFORM_IOS
#define GL_SILENCE_DEPRECATION
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import <OpenGLES/ES3/gl.h>
#import <OpenGLES/ES3/glext.h>
#elif TRUESIGHT_PLATFORM_LINUX
#include <GLES3/gl3.h>
#elif TRUESIGHT_PLATFORM_EMSCRIPTEN
#include <GLES2/gl2.h>
#endif

// 安全释放纹理
#define GL_DELETE_TEXTURE(x) if(x) { glDeleteTextures(1,&x); x = 0; }
// 安全释放framebuffer
#define GL_DELETE_FRAMEBUFFER(x) if(x) { glDeleteFramebuffers(1,&x); x = 0;}