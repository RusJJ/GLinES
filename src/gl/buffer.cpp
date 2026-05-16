#include "gl_buffer.h"

void* WRAP(glMapBuffer(GLenum target, GLenum access))
{
    if(access == 0x88B8) access = GL_MAP_READ_BIT;
    else if(access == 0x88B9) access = GL_MAP_WRITE_BIT;
    else if(access == 0x88BA) access = GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
    // target == GL_QUERY_BUFFER ?

    GLint size = 0;
    glGetBufferParameteriv(target, GL_BUFFER_SIZE, &size);
    
    return glMapBufferRange(target, 0, size, access);
}

void WRAP(glDrawBuffer(GLenum buf))
{
    GLenum bbuf = buf;
    return glDrawBuffers(1, &bbuf);
}

void WRAP(glGetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, void* dst))
{
    void* p = glMapBufferRange(target, offset, size, GL_MAP_READ_BIT);
    if(p == NULL) return;
    
    memcpy(dst, p, (size_t)size);
    glUnmapBuffer(target);
}

void WRAP(glBindFramebuffer(GLenum target, GLuint framebuffer))
{
    if(target == GL_FRAMEBUFFER)
    {
        globals->gl.activeDrawBuffer = globals->gl.activeReadBuffer = framebuffer;
    }
    else if(target == GL_DRAW_FRAMEBUFFER)
    {
        globals->gl.activeDrawBuffer = framebuffer;
    }
    else if(target == GL_READ_FRAMEBUFFER)
    {
        globals->gl.activeReadBuffer = framebuffer;
    }
    glBindFramebuffer(target, framebuffer);
}

GLenum WRAP(glCheckFramebufferStatus(GLenum target))
{
    GLenum ret = glCheckFramebufferStatus(target);
    switch(ret)
    {
        case GL_FRAMEBUFFER_COMPLETE:
            MSG("glCheckFramebufferStatus(0x%X) = GL_FRAMEBUFFER_COMPLETE", target);
            break;
            
        case GL_FRAMEBUFFER_UNDEFINED:
            MSG("glCheckFramebufferStatus(0x%X) = GL_FRAMEBUFFER_UNDEFINED", target);
            break;
            
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            MSG("glCheckFramebufferStatus(0x%X) = GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT", target);
            break;
            
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            MSG("glCheckFramebufferStatus(0x%X) = GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT", target);
            break;
            
        case GL_FRAMEBUFFER_UNSUPPORTED:
            MSG("glCheckFramebufferStatus(0x%X) = GL_FRAMEBUFFER_UNSUPPORTED", target);
            break;
            
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            MSG("glCheckFramebufferStatus(0x%X) = GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE", target);
            break;

        default:
            MSG("glCheckFramebufferStatus(0x%X) = UNKNOWN", target);
            break;
    }
    return ret;
}

void WRAP(glBindBuffer(GLenum target, GLuint buffer))
{
    if (target == GL_ARRAY_BUFFER) globals->client.boundArrayBuffer = buffer;
    else if (target == GL_ELEMENT_ARRAY_BUFFER) globals->client.boundElementBuffer = buffer;
    else if (target == GL_PIXEL_UNPACK_BUFFER) globals->client.boundPixelUnpackBuffer = buffer;
    else if (target == GL_PIXEL_PACK_BUFFER) globals->client.boundPixelPackBuffer = buffer;
    else if (target == GL_UNIFORM_BUFFER) globals->client.boundUniformBuffer = buffer;

    glBindBuffer(target, buffer);
}

void WRAP(glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size))
{
    if(target == GL_UNIFORM_BUFFER) globals->client.boundUniformBuffer = buffer;

    glBindBufferRange(target, index, buffer, offset, size);
}

void WRAP(glBindBufferBase(GLenum target, GLuint index, GLuint buffer))
{
    if(target == GL_UNIFORM_BUFFER) globals->client.boundUniformBuffer = buffer;

    glBindBufferBase(target, index, buffer);
}
