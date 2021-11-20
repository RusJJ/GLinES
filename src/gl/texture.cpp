#include "gl_texture.h"
#include "gl_render.h"

#include "thirdparty/Benjamin_Dobell/s3tc.h"

void WRAP(glGenTextures(GLsizei n, GLuint * textures))
{
    glGenTextures(n, textures);

    static int i = 0;
    static texture_desc_t* tex;
    for(i = 0; i < n; ++i)
    {
        tex = globals->textures[textures[i]];
        if(!tex)
        {
            tex = new texture_desc_t;
            globals->textures[textures[i]] = tex;
            tex->id = textures[i];
        }
    }
}

void WRAP(glDeleteTextures(GLsizei n, GLuint * textures))
{
    static int i = 0;
    for(i = 0; i < n; ++i)
    {
        delete[] globals->textures[textures[i]];
    }
    glDeleteTextures(n, textures);
}

void WRAP(glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* data))
{
    DBG("glTexImage2D");

    if(type == GL_HALF_FLOAT) type = GL_HALF_FLOAT_OES;
    else if(type == 0x8367) type = GL_UNSIGNED_BYTE; //GL_UNSIGNED_INT_8_8_8_8_REV

    globals->gl.activeTexture->width = width;
    globals->gl.activeTexture->height = height;

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
            glTexImage2D(target, level, internalformat, width, height, border, format, type, data);
            break;
    }
}

void WRAP(glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void * pixels))
{
    DBG("glTexSubImage2D");

    if(type == GL_HALF_FLOAT) type = GL_HALF_FLOAT_OES;
    else if(type == 0x8367) type = GL_UNSIGNED_BYTE; //GL_UNSIGNED_INT_8_8_8_8_REV

    globals->gl.activeTexture->width = width - xoffset;
    globals->gl.activeTexture->height = height - yoffset;

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
            glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
            break;

        case 0x80E0: //GL_BGR:
        case 0x80E1: //GL_BGRA:
        case 0x8D9A: //GL_BGR_INTEGER:
        case 0x8D9B: //GL_BGRA_INTEGER:
            break;
    }
}

void WRAP(glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels))
{
    DBG("glGetTexImage");

    GLuint fbo; GLint read_fbo = 0, draw_fbo = 0;

    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &draw_fbo);
	glGetIntegerv(GL_RENDERBUFFER_BINDING, &read_fbo);

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, globals->gl.activeTexture->id, 0);
    glReadPixels(0, 0, globals->gl.activeTexture->width, globals->gl.activeTexture->height, format, type, pixels);
    glBindFramebuffer(GL_FRAMEBUFFER, nCurrentFB);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, read_fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, draw_fbo);
    glDeleteFramebuffers(1, &fbo);
}

void WRAP(glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data))
{
    DBG("glCompressedTexImage2D");
    
    if(!data)
    {
        ERR("glCompressedTexImage2D can`t uncompress a texture with zero data!");
        return;
    }
    if(width < 1 || height < 1)
    {
        ERR("glCompressedTexImage2D can`t uncompress a texture with width and/or height 0!");
        return;
    }
    if(internalformat == GL_RGBA8) internalformat = 0x83F1; //GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;

    globals->gl.activeTexture->width = width;
    globals->gl.activeTexture->height = height;
    
    switch (internalformat)
    {
        //case 0x8C4C: //GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
        //case 0x8C4D: //GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
        //case 0x83F0: //GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        //case 0x83F1: //GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        //{
        //    GLvoid* new_data = new char[4 * width * height];
        //    BlockDecompressImageDXT1(width, height, (unsigned char*)data, (unsigned long*)new_data);
        //    glTexImage2D(target, level, 4, width, height, border, GL_RGBA, GL_UNSIGNED_BYTE, new_data);
        //    //delete[] (unsigned long*)new_data;
        //    break;
        //}

        //case 0x83F2: //GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        //case 0x83F3: //GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        //case 0x8C4E: //GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
        //case 0x8C4F: //GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
        //{
        //    GLvoid* new_data = new char[4 * width * height];
        //    BlockDecompressImageDXT5(width, height, (unsigned char*)data, (unsigned long*)new_data);
        //    glTexImage2D(target, level, 4, width, height, border, GL_RGBA, GL_UNSIGNED_BYTE, new_data);
        //    //delete[] (unsigned long*)new_data;
        //    break;
        //}
        
        default:
            glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
            break;
    }
}