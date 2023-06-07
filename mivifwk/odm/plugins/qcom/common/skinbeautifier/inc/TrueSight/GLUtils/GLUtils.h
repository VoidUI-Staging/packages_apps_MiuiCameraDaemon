#pragma once
#include <TrueSight/TrueSightDefine.hpp>
#include "TSGLDefine.h"

namespace TrueSight {

class TrueSight_PUBLIC GLUtils {
public:
    static GLuint CreateTexure(GLenum nFormat, int nWidth, int nHeight, unsigned char* pData);
    static bool BindFBO(GLuint nTexture, GLuint nFrameBuffer);
    static void GetTextureData(GLuint nTextureID, GLuint nFrameBufferID, GLsizei nWidth, GLsizei nHeight, GLbyte* pData);
    static bool IsGLTexture(GLuint nTextureID);
    static void ReBindGL();
    static void write_texture_2d_ragb(GLuint input_tex, int width, int height, char dst_path[]);
};

} // namespace TrueSight