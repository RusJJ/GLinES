#include "gl_texture.h"
#include "gl_render.h"
#include "maths.h"

//#define STB_IMAGE_WRITE_IMPLEMENTATION
//#define STB_IMAGE_WRITE_STATIC
//#include "thirdparty/nothings/stb_image_write.h"
//#include "thirdparty/Benjamin_Dobell/s3tc.h"

extern "C"
{
    #include "thirdparty/DXTn.h"
}

inline bool isDXTc(GLenum format)
{
    switch (format)
    {
        case 0x83F0: //GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case 0x83F1: //GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case 0x83F2: //GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case 0x83F3: //GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case 0x8C4C: //GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
        case 0x8C4D: //GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
        case 0x8C4E: //GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
        case 0x8C4F: //GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
            return true;
    }
    return false;
}

inline bool isDXTcSRGB(GLenum format)
{
    switch (format)
    {
        case 0x8C4C: //GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
        case 0x8C4D: //GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
        case 0x8C4E: //GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
        case 0x8C4F: //GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
            return true;
    }
    return false;
}

inline bool isDXTcAlpha(GLenum format)
{
    switch (format)
    {
        case 0x83F1: //GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case 0x83F2: //GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case 0x83F3: //GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case 0x8C4D: //GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
        case 0x8C4E: //GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
        case 0x8C4F: //GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
            return true;
    }
    return false;
}

inline GLvoid *UncompressDXTn(GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, int transparent0, int* simpleAlpha, int* complexAlpha, const GLvoid *data)
{
    const int pixelsize = 4;
    if (imageSize == width * height * pixelsize || data == NULL) return (GLvoid*)data;
    GLvoid *pixels = (GLvoid *)(new char[( ( (width+3)&~3 )*( (height+3)&~3 )*pixelsize )]);
    int blocksize;
    switch (format)
    {
        case 0x83F0: //GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case 0x8C4C: //GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
        case 0x83F1: //GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case 0x8C4D: //GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
            blocksize = 8;
            break;
        case 0x83F2: //GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case 0x83F3: //GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case 0x8C4E: //GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
        case 0x8C4F: //GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
            blocksize = 16;
            break;
    }
    uintptr_t src = (uintptr_t) data;
    for (uint32_t y = 0; y < height; y += 4)
    {
        for (uint32_t x = 0; x < width; x += 4)
        {
            switch(format)
            {
                case 0x83F0: //GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
                case 0x8C4C: //GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
                case 0x83F1: //GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
                case 0x8C4D: //GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
                    DecompressBlockDXT1(x, y, width, (uint8_t*)src, transparent0, simpleAlpha, complexAlpha, (uint32_t*)pixels);
                    break;
                case 0x83F2: //GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
                case 0x8C4E: //GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
                    DecompressBlockDXT3(x, y, width, (uint8_t*)src, transparent0, simpleAlpha, complexAlpha, (uint32_t*)pixels);
                    break;
                case 0x83F3: //GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
                case 0x8C4F: //GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
                    DecompressBlockDXT5(x, y, width, (uint8_t*)src, transparent0, simpleAlpha, complexAlpha, (uint32_t*)pixels);
                    break;
            }
            src += blocksize;
        }
    }
    return pixels;
}

void WRAP(glGenTextures(GLsizei n, GLuint * textures))
{
    static int i = 0; static texture_desc_t* tex;
    glGenTextures(n, textures);
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

void WRAP(glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels))
{
    static GLuint fbo; static GLint read_fbo = 0, draw_fbo = 0;

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

void WRAP(glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* data))
{
    if(type == 0x8367) type = GL_UNSIGNED_BYTE; //GL_UNSIGNED_INT_8_8_8_8_REV
    if(format == 0x80E0) format = GL_RGB; //GL_BGR
    else if(format == 0x80E1) format = GL_RGBA; //GL_BGRA
    else if(format == GL_LUMINANCE || format == GL_LUMINANCE_ALPHA) internalformat = format;
    if(internalformat == GL_SRGB8 && format == GL_RGBA) internalformat = GL_SRGB8_ALPHA8;

    globals->gl.activeTexture->width = width;
    globals->gl.activeTexture->height = height;
    
    glTexImage2D(target, level, internalformat, width, height, border, format, type, data);
}

void WRAP(glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height))
{
    if(!levels) return;

}

void WRAP(glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void * pixels))
{
    if(type == 0x8367) type = GL_UNSIGNED_BYTE; //GL_UNSIGNED_INT_8_8_8_8_REV

    globals->gl.activeTexture->width = width - xoffset;
    globals->gl.activeTexture->height = height - yoffset;

    return glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void WRAP(glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data))
{
    if(!data || width < 1 || height < 1) return; //glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
    if(internalformat == GL_RGBA8) internalformat = 0x83F1; //GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; // GL4ES hack

    globals->gl.activeTexture->width = width;
    globals->gl.activeTexture->height = height;

    GLenum format = GL_RGBA;
	GLenum intformat = GL_RGBA;
    GLenum type = GL_UNSIGNED_BYTE;
	GLvoid *pixels = NULL;

    if (isDXTc(internalformat))
    {
        int simpleAlpha = 0;
        int complexAlpha = 0;
        bool transparent0 = (internalformat==0x83F1 || internalformat==0x8C4D)?true:false;
        if(data) pixels = UncompressDXTn(width, height, internalformat, imageSize, transparent0, &simpleAlpha, &complexAlpha, data);
		if(isDXTcSRGB(internalformat)) intformat = GL_SRGB8_ALPHA8;
	}
	WRAP(glTexImage2D( target, level, intformat, width, height, border, format, GL_UNSIGNED_BYTE, pixels?pixels:data ));
}

//void WRAP(glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum internalformat, GLsizei imageSize, const void * data))
//{
//    if(!data)
//    {
//        ERR("glCompressedTexImage2D can`t uncompress a texture with zero data!");
//        return;
//    }
//    if(width < 1 || height < 1)
//    {
//        ERR("glCompressedTexImage2D can`t uncompress a texture with width and/or height 0!");
//        return;
//    }
//    if(internalformat == GL_RGBA8) internalformat = 0x83F1; //GL_COMPRESSED_RGBA_S3TC_DXT1_EXT; // GL4ES hack
//
//    globals->gl.activeTexture->width = width;
//    globals->gl.activeTexture->height = height;
//
//    //return glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
//
//    bool bIsAlpha = (internalformat==0x8C4D || internalformat==0x83F1 || internalformat==0x83F2 || 
//                     internalformat==0x83F3 || internalformat==0x8C4E || internalformat==0x8C4F);
//    
//    switch (internalformat)
//    {
//        case 0x83F0: //GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
//        case 0x83F1: //GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
//        case 0x8C4C: //GL_COMPRESSED_SRGB_S3TC_DXT1_EXT:
//        case 0x8C4D: //GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
//        {
//            GLvoid* new_data = new char[4 * width * height];
//            BlockDecompressImageDXT1(width, height, (unsigned char*)data, (unsigned long*)new_data);
//            if(internalformat == 0x8C4C || internalformat == 0x8C4D) ConvertSRGBPixelsToRGB((char*)new_data, width, height);
//            char _path[64];
//            sprintf(_path, "/sdcard/srceng2/glines/c%u_l%d_dxt1.png", globals->gl.activeTexture->id, level);
//            stbi_write_png(_path, width, height, 4, new_data, width * 4);
//            //glTexImage2D(target, level, GL_RGBA, width, height, border, GL_RGBA, GL_UNSIGNED_BYTE, new_data);
//            glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, new_data);
//            delete[] (char*)new_data;
//            return;
//        }
//
//        case 0x83F2: //GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
//        case 0x83F3: //GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
//        case 0x8C4E: //GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
//        case 0x8C4F: //GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
//        {
//            GLvoid* new_data = new char[4 * width * height];
//            BlockDecompressImageDXT5(width, height, (unsigned char*)data, (unsigned long*)new_data);
//            if(internalformat == 0x8C4E || internalformat == 0x8C4F) ConvertSRGBPixelsToRGB((char*)new_data, width, height);
//            //char _path[64];
//            //sprintf(_path, "/sdcard/srceng2/glines/c%u_l%d_dxt5.png", globals->gl.activeTexture->id, level);
//            //stbi_write_png(_path, width, height, 4, new_data, width * 4);
//            glTexImage2D(target, level, GL_RGBA, width, height, border, GL_RGBA, GL_UNSIGNED_BYTE, new_data);
//            delete[] (char*)new_data;
//            return;
//        }
//        
//        default:
//            return glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
//    }
//}