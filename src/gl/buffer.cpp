#include "gl_buffer.h"

void* WRAP(glMapBuffer(GLenum target, GLenum access))
{
    if(access == 0x88B8) access = GL_MAP_READ_BIT;
    else if(access == 0x88B9) access = GL_MAP_WRITE_BIT;
    else if(access == 0x88BA) access = GL_MAP_READ_BIT|GL_MAP_WRITE_BIT;
    // target == GL_QUERY_BUFFER ?
    return glMapBufferRange(target, 0, sizeof(void*), access);
}

void WRAP(glDrawBuffer(GLenum buf))
{
    GLenum bbuf = buf;
    return glDrawBuffers(1, &bbuf);
}
