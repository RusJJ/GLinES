#include "gl_texture.h"

void WRAP(glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* data))
{
    if(type == 0x140B /* OpenGL's GL_HALF_FLOAT (on PC) */) type = GL_HALF_FLOAT;

    switch(format)
    {
        case GL_RED:
        case GL_RG:
        case GL_RGB:
        case GL_RGBA:
        case GL_RED_INTEGER:
        case GL_RG_INTEGER:
        case GL_RGB_INTEGER:
        case GL_RGBA_INTEGER:
        case GL_STENCIL_INDEX:
        case GL_DEPTH_COMPONENT:
        case GL_DEPTH_STENCIL:
        case GL_LUMINANCE:
        case GL_LUMINANCE_ALPHA:
        case GL_ALPHA:
            glTexImage2D(target, level, internalformat, width, height, border, format, type, data);
            break;

        case 0x80E0: //GL_BGR:
        case 0x80E1: //GL_BGRA:
        case 0x8D9A: //GL_BGR_INTEGER:
        case 0x8D9B: //GL_BGRA_INTEGER:
            break;
    }
}