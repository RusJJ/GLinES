#include "gl_buffer.h"

void* WRAP(glMapBuffer(GLenum target, GLenum access))
{
    return glMapBufferRange(target, 0, 4, access);
}

void WRAP(glDrawBuffer(GLenum buf))
{
    return glDrawBuffers(1, &buf);
}
