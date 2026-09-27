#include "gl_texture.h"
#include "gl_render.h"
#include <algorithm>
#include <limits>
#include <vector>

extern "C"
{
    #include "thirdparty/DXTn.h"
}

static GLenum TextureTarget(GLenum target)
{
    return target == 0x0DE0 || target == 0x84F5 ? GL_TEXTURE_2D : target;
}

struct pixel_store_t
{
    GLenum alignment, rowLength, skipRows, skipPixels, bufferTarget;
    GLint align = 4, row = 0, rows = 0, pixels = 0, buffer = 0;
    pixel_store_t(bool pack)
    {
        alignment = pack ? GL_PACK_ALIGNMENT : GL_UNPACK_ALIGNMENT;
        rowLength = pack ? GL_PACK_ROW_LENGTH : GL_UNPACK_ROW_LENGTH;
        skipRows = pack ? GL_PACK_SKIP_ROWS : GL_UNPACK_SKIP_ROWS;
        skipPixels = pack ? GL_PACK_SKIP_PIXELS : GL_UNPACK_SKIP_PIXELS;
        bufferTarget = pack ? GL_PIXEL_PACK_BUFFER : GL_PIXEL_UNPACK_BUFFER;
        glGetIntegerv(alignment, &align);
        glGetIntegerv(rowLength, &row);
        glGetIntegerv(skipRows, &rows);
        glGetIntegerv(skipPixels, &pixels);
        glGetIntegerv(pack ? GL_PIXEL_PACK_BUFFER_BINDING : GL_PIXEL_UNPACK_BUFFER_BINDING, &buffer);
    }
    size_t Stride(GLsizei width, size_t pixelSize) const
    {
        size_t bytes = (size_t)(row ? row : width) * pixelSize;
        return (bytes + align - 1) & ~(size_t)(align - 1);
    }
    size_t Offset(GLsizei width, size_t pixelSize) const { return (size_t)rows * Stride(width, pixelSize) + (size_t)pixels * pixelSize; }
    void Tight()
    {
        glBindBuffer(bufferTarget, 0);
        glPixelStorei(alignment, 1);
        glPixelStorei(rowLength, 0);
        glPixelStorei(skipRows, 0);
        glPixelStorei(skipPixels, 0);
    }
    ~pixel_store_t()
    {
        glBindBuffer(bufferTarget, buffer);
        glPixelStorei(alignment, align);
        glPixelStorei(rowLength, row);
        glPixelStorei(skipRows, rows);
        glPixelStorei(skipPixels, pixels);
    }
};

static bool LegacyFormat(GLenum format)
{
    return format == GL_ALPHA || format == GL_LUMINANCE || format == GL_LUMINANCE_ALPHA || format == 0x80E0 || format == 0x80E1;
}

static size_t Components(GLenum format)
{
    if(format == GL_ALPHA || format == GL_LUMINANCE || format == GL_RED) return 1;
    if(format == GL_LUMINANCE_ALPHA || format == GL_RG) return 2;
    if(format == GL_RGB || format == 0x80E0) return 3;
    return 4;
}

static size_t ScalarSize(GLenum type)
{
    if(type == GL_UNSIGNED_BYTE || type == GL_BYTE) return 1;
    if(type == GL_UNSIGNED_SHORT || type == GL_SHORT || type == GL_HALF_FLOAT) return 2;
    if(type == GL_FLOAT || type == GL_UNSIGNED_INT || type == GL_INT || type == 0x8367) return 4;
    return 0;
}

static bool ConvertPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const void* data,
                          pixel_store_t& store, std::vector<unsigned char>& output)
{
    size_t scalar = type == 0x8367 ? 1 : ScalarSize(type);
    if(!scalar) { SetError(GL_INVALID_ENUM); return false; }
    if(type == 0x8367 && Components(format) != 4) { SetError(GL_INVALID_OPERATION); return false; }
    size_t components = Components(format);
    size_t stride = store.Stride(width, components * scalar);
    size_t offset = store.Offset(width, components * scalar);
    size_t required = offset + (height ? (size_t)(height - 1) * stride + (size_t)width * components * scalar : 0);
    const unsigned char* source = (const unsigned char*)data;
    void* mapped = nullptr;
    if(store.buffer)
    {
        GLint64 size = 0;
        glGetBufferParameteri64v(GL_PIXEL_UNPACK_BUFFER, GL_BUFFER_SIZE, &size);
        size_t start = (size_t)data;
        if(start > (size_t)size || required > (size_t)size - start) { SetError(GL_INVALID_OPERATION); return false; }
        mapped = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, (GLintptr)start, required, GL_MAP_READ_BIT);
        if(!mapped) return false;
        source = (const unsigned char*)mapped;
    }
    if(!source) return true;
    output.resize((size_t)width * height * 4 * scalar);
    for(GLsizei y = 0; y < height; ++y)
    {
        const unsigned char* row = source + offset + (size_t)y * stride;
        for(GLsizei x = 0; x < width; ++x)
        {
            const unsigned char* p = row + (size_t)x * components * scalar;
            unsigned char* out = output.data() + ((size_t)y * width + x) * 4 * scalar;
            memset(out, 0, 4 * scalar);
            uint32_t one = type == GL_FLOAT ? 0x3F800000 : type == GL_HALF_FLOAT ? 0x3C00 :
                           type == GL_BYTE ? 0x7F : type == GL_SHORT ? 0x7FFF : type == GL_INT ? 0x7FFFFFFF : 0xFFFFFFFF;
            memcpy(out + 3 * scalar, &one, scalar);
            if(format == GL_ALPHA) memcpy(out + 3 * scalar, p, scalar);
            else if(format == GL_LUMINANCE || format == GL_LUMINANCE_ALPHA)
            {
                for(int c = 0; c < 3; ++c) memcpy(out + c * scalar, p, scalar);
                if(format == GL_LUMINANCE_ALPHA) memcpy(out + 3 * scalar, p + scalar, scalar);
            }
            else
            {
                bool bgr = format == 0x80E0 || format == 0x80E1;
                memcpy(out, p + (bgr ? 2 : 0) * scalar, scalar);
                memcpy(out + scalar, p + scalar, scalar);
                memcpy(out + 2 * scalar, p + (bgr ? 0 : 2) * scalar, scalar);
                if(components == 4) memcpy(out + 3 * scalar, p + 3 * scalar, scalar);
            }
        }
    }
    if(mapped) glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    return true;
}

static GLint InternalFormat(GLint internalformat)
{
    if(internalformat == 1 || internalformat == 2 || internalformat == GL_ALPHA || internalformat == GL_LUMINANCE || internalformat == GL_LUMINANCE_ALPHA) return GL_RGBA;
    if(internalformat == 3) return GL_RGB;
    if(internalformat == 4) return GL_RGBA;
    return internalformat;
}

void WRAP(glGenTextures(GLsizei n, GLuint* textures)) { glGenTextures(n, textures); }

void WRAP(glDeleteTextures(GLsizei n, GLuint* textures))
{
    if(n < 0) { SetError(GL_INVALID_VALUE); return; }
    glDeleteTextures(n, textures);
    for(GLsizei i = 0; i < n; ++i)
    {
        auto it = globals->textures.find(textures[i]);
        if(it == globals->textures.end()) continue;
        if(globals->gl.activeTexture == it->second) globals->gl.activeTexture = nullptr;
        delete it->second;
        globals->textures.erase(it);
    }
}

void WRAP(glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* data))
{
    if(width < 0 || height < 0 || level < 0 || border) { SetError(GL_INVALID_VALUE); return; }
    target = TextureTarget(target);
    GLint binding = 0;
    if(target == GL_TEXTURE_2D && level == 0)
    {
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding);
        auto it = globals->textures.find((GLuint)binding);
        if(it != globals->textures.end()) it->second->baseFormat = internalformat;
    }
    internalformat = InternalFormat(internalformat);
    if(LegacyFormat(format) || type == 0x8367)
    {
        pixel_store_t store(false);
        std::vector<unsigned char> converted;
        if((data || store.buffer) && width && height && !ConvertPixels(width, height, format, type, data, store, converted)) return;
        store.Tight();
        if(internalformat == GL_RGB || internalformat == GL_RGBA) internalformat = GL_RGBA;
        glTexImage2D(target, level, internalformat, width, height, 0, GL_RGBA, type == 0x8367 ? GL_UNSIGNED_BYTE : type, converted.empty() ? nullptr : converted.data());
    }
    else glTexImage2D(target, level, internalformat, width, height, 0, format, type, data);
}

void WRAP(glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels))
{
    if(width < 0 || height < 0 || level < 0) { SetError(GL_INVALID_VALUE); return; }
    if(!width || !height) return;
    target = TextureTarget(target);
    if(LegacyFormat(format) || type == 0x8367)
    {
        pixel_store_t store(false);
        std::vector<unsigned char> converted;
        if(!pixels && !store.buffer) { SetError(GL_INVALID_VALUE); return; }
        if(!ConvertPixels(width, height, format, type, pixels, store, converted)) return;
        store.Tight();
        glTexSubImage2D(target, level, xoffset, yoffset, width, height, GL_RGBA, type == 0x8367 ? GL_UNSIGNED_BYTE : type, converted.data());
    }
    else glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);
}

void WRAP(glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, void* pixels))
{
    if(level < 0) { SetError(GL_INVALID_VALUE); return; }
    target = TextureTarget(target);
    GLint binding = 0, width = 0, height = 0, previous = 0;
    GLenum bindTarget = target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z ? GL_TEXTURE_BINDING_CUBE_MAP : GL_TEXTURE_BINDING_2D;
    if(bindTarget == GL_TEXTURE_BINDING_2D && target != GL_TEXTURE_2D) { SetError(GL_INVALID_ENUM); return; }
    glGetIntegerv(bindTarget, &binding);
    glGetTexLevelParameteriv(target, level, GL_TEXTURE_WIDTH, &width);
    glGetTexLevelParameteriv(target, level, GL_TEXTURE_HEIGHT, &height);
    if(!width || !height) return;
    pixel_store_t store(true);
    if(!pixels && !store.buffer) { SetError(GL_INVALID_VALUE); return; }
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous);
    GLuint fbo;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    GLenum attachment = format == GL_DEPTH_COMPONENT ? GL_DEPTH_ATTACHMENT : format == GL_DEPTH_STENCIL ? GL_DEPTH_STENCIL_ATTACHMENT : GL_COLOR_ATTACHMENT0;
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, attachment, target, binding, level);
    if(attachment == GL_COLOR_ATTACHMENT0) glReadBuffer(GL_COLOR_ATTACHMENT0);
    else glReadBuffer(GL_NONE);
    if(glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) SetError(GL_INVALID_OPERATION);
    else if(type == GL_UNSIGNED_BYTE && (format == GL_RGB || format == GL_RGBA || LegacyFormat(format)))
    {
        std::vector<unsigned char> rgba((size_t)width * height * 4);
        store.Tight();
        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
        size_t components = Components(format), stride = store.Stride(width, components), offset = store.Offset(width, components);
        size_t size = offset + (size_t)(height - 1) * stride + (size_t)width * components;
        unsigned char* out = (unsigned char*)pixels;
        void* mapped = nullptr;
        if(store.buffer)
        {
            glBindBuffer(GL_PIXEL_PACK_BUFFER, store.buffer);
            GLint64 bufferSize;
            glGetBufferParameteri64v(GL_PIXEL_PACK_BUFFER, GL_BUFFER_SIZE, &bufferSize);
            size_t start = (size_t)pixels;
            if(start <= (size_t)bufferSize && size <= (size_t)bufferSize - start)
                mapped = glMapBufferRange(GL_PIXEL_PACK_BUFFER, start, size, GL_MAP_WRITE_BIT);
            else SetError(GL_INVALID_OPERATION);
            out = (unsigned char*)mapped;
        }
        if(out) for(GLint y = 0; y < height; ++y) for(GLint x = 0; x < width; ++x)
        {
            const unsigned char* in = rgba.data() + ((size_t)y * width + x) * 4;
            unsigned char* p = out + offset + (size_t)y * stride + (size_t)x * components;
            if(format == GL_ALPHA) p[0] = in[3];
            else if(format == GL_LUMINANCE || format == GL_LUMINANCE_ALPHA)
            {
                p[0] = in[0];
                if(components == 2) p[1] = in[3];
            }
            else
            {
                bool bgr = format == 0x80E0 || format == 0x80E1;
                p[0] = in[bgr ? 2 : 0]; p[1] = in[1]; p[2] = in[bgr ? 0 : 2];
                if(components == 4) p[3] = in[3];
            }
        }
        if(mapped) glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    }
    else glReadPixels(0, 0, width, height, format, type, pixels);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, previous);
    glDeleteFramebuffers(1, &fbo);
}

void WRAP(glTexStorage2D(GLenum target, GLsizei levels, GLenum format, GLsizei width, GLsizei height))
{
    glTexStorage2D(TextureTarget(target), levels, format, width, height);
}

void WRAP(glCompressedTexImage2D(GLenum target, GLint level, GLenum format, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data))
{
    bool dxt = (format >= 0x83F0 && format <= 0x83F3) || (format >= 0x8C4C && format <= 0x8C4F);
    if(!dxt) { glCompressedTexImage2D(TextureTarget(target), level, format, width, height, border, imageSize, data); return; }
    if(width < 0 || height < 0 || imageSize < 0 || border || level < 0) { SetError(GL_INVALID_VALUE); return; }
    unsigned kind = format >= 0x8C4C ? format - 0x8C4C : format - 0x83F0;
    size_t blockSize = kind < 2 ? 8 : 16;
    size_t needed = ((size_t)width + 3) / 4 * (((size_t)height + 3) / 4) * blockSize;
    if((size_t)imageSize != needed) { SetError(GL_INVALID_VALUE); return; }
    pixel_store_t store(false);
    const unsigned char* source = (const unsigned char*)data;
    void* mapped = nullptr;
    if(store.buffer && needed)
    {
        GLint64 size;
        glGetBufferParameteri64v(GL_PIXEL_UNPACK_BUFFER, GL_BUFFER_SIZE, &size);
        size_t offset = (size_t)data;
        if(offset > (size_t)size || needed > (size_t)size - offset) { SetError(GL_INVALID_OPERATION); return; }
        mapped = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, offset, needed, GL_MAP_READ_BIT);
        if(!mapped) return;
        source = (const unsigned char*)mapped;
    }
    std::vector<unsigned char> output;
    if(source && needed)
    {
        output.resize((size_t)width * height * 4);
        for(GLsizei y = 0; y < height; y += 4) for(GLsizei x = 0; x < width; x += 4)
        {
            uint32_t block[16] = {};
            int simple = 0, complex = 0;
            if(kind < 2) DecompressBlockDXT1(0, 0, 4, source, kind == 1, &simple, &complex, block);
            else if(kind == 2) DecompressBlockDXT3(0, 0, 4, source, 0, &simple, &complex, block);
            else DecompressBlockDXT5(0, 0, 4, source, 0, &simple, &complex, block);
            for(GLsizei row = 0; row < std::min(4, height-y); ++row)
                memcpy(output.data() + ((size_t)(y+row)*width+x)*4, block+row*4, (size_t)std::min(4, width-x)*4);
            source += blockSize;
        }
    }
    if(mapped) glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    store.Tight();
    glTexImage2D(TextureTarget(target), level, format >= 0x8C4C ? GL_SRGB8_ALPHA8 : GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, output.empty() ? nullptr : output.data());
}

void WRAP(glTexImage2DMultisample(GLenum target, GLsizei samples, GLenum format, GLsizei width, GLsizei height, GLboolean fixed))
{
    glTexStorage2DMultisample(target, samples, format, width, height, fixed);
}

void WRAP(glTexImage3DMultisample(GLenum target, GLsizei samples, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixed))
{
    glTexStorage3DMultisample(target, samples, format, width, height, depth, fixed);
}

void WRAP(glFramebufferTexture3D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint layer))
{
    if(textarget != GL_TEXTURE_3D) { SetError(GL_INVALID_ENUM); return; }
    glFramebufferTextureLayer(target, attachment, texture, level, layer);
}

void WRAP(glActiveTexture(GLenum unit))
{
    GLint count;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &count);
    if(unit < GL_TEXTURE0 || unit - GL_TEXTURE0 >= (GLuint)count) { SetError(GL_INVALID_ENUM); return; }
    glActiveTexture(unit);
    globals->gl.activeTexUnit = unit;
    globals->ff.activeTextureUnit = (GLint)(unit - GL_TEXTURE0);
}

void WRAP(glBindMultiTexture(GLenum unit, GLenum target, GLuint texture))
{
    GLenum previous = globals->gl.activeTexUnit;
    WRAP(glActiveTexture(unit));
    if(globals->gl.activeTexUnit == unit) WRAP(glBindTexture(target, texture));
    WRAP(glActiveTexture(previous));
}

void WRAP(glBindTextureUnit(GLuint unit, GLuint texture))
{
    auto it = globals->textures.find(texture);
    if(texture && (it == globals->textures.end() || !it->second)) { SetError(GL_INVALID_OPERATION); return; }
    if(texture) WRAP(glBindMultiTexture(GL_TEXTURE0 + unit, it->second->target, texture));
    else for(GLenum target : {GL_TEXTURE_2D, GL_TEXTURE_3D, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_2D_ARRAY})
        WRAP(glBindMultiTexture(GL_TEXTURE0 + unit, target, 0));
}

void WRAP(glGetCompressedTexImage(GLenum, GLint, GLvoid*)) { SetError(GL_INVALID_OPERATION); }

void WRAP(glTexParameteri(GLenum target, GLenum pname, GLint value))
{
    glTexParameteri(TextureTarget(target), pname, value);
}

void WRAP(glTexParameterf(GLenum target, GLenum pname, GLfloat value))
{
    glTexParameterf(TextureTarget(target), pname, value);
}

static bool ValidSwizzle(const GLint* values)
{
    for(int i = 0; i < 4; ++i)
    {
        GLint v = values[i];
        if(v != GL_RED && v != GL_GREEN && v != GL_BLUE && v != GL_ALPHA && v != GL_ZERO && v != GL_ONE)
        {
            SetError(GL_INVALID_ENUM);
            return false;
        }
    }
    return true;
}

void WRAP(glTexParameteriv(GLenum target, GLenum pname, const GLint* values))
{
    target = TextureTarget(target);
    if(pname == 0x8E46)
    {
        if(!ValidSwizzle(values)) return;
        for(int i = 0; i < 4; ++i) glTexParameteri(target, GL_TEXTURE_SWIZZLE_R + i, values[i]);
    }
    else glTexParameteriv(target, pname, values);
}

void WRAP(glTexParameterfv(GLenum target, GLenum pname, const GLfloat* values))
{
    if(pname == 0x8E46)
    {
        GLint converted[4];
        for(int i = 0; i < 4; ++i) converted[i] = (GLint)values[i];
        WRAP(glTexParameteriv(target, pname, converted));
    }
    else glTexParameterfv(TextureTarget(target), pname, values);
}

void WRAP(glGetTexParameteriv(GLenum target, GLenum pname, GLint* values))
{
    target = TextureTarget(target);
    if(pname == 0x8E46)
        for(int i = 0; i < 4; ++i) glGetTexParameteriv(target, GL_TEXTURE_SWIZZLE_R + i, values + i);
    else glGetTexParameteriv(target, pname, values);
}

void WRAP(glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* values))
{
    target = TextureTarget(target);
    if(pname == 0x8E46)
        for(int i = 0; i < 4; ++i) glGetTexParameterfv(target, GL_TEXTURE_SWIZZLE_R + i, values + i);
    else glGetTexParameterfv(target, pname, values);
}
