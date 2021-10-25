#include "gl_buffer.h"

void* GLIN_Wrap_glMapBuffer(GLenum target, GLenum access)
{
    static GLsizei len = 1;
    return glMapBufferRange(target, 0, len, access);
}

void GLIN_Wrap_glDrawBuffer(GLenum buf)
{
    return glDrawBuffers(1, &buf);
}