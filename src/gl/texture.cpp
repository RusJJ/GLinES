#include "gl_texture.h"
#include "gl_render.h"
#include <algorithm>
#include <limits>
#include <vector>
#include <memory>

extern "C"
{
    #include "thirdparty/DXTn.h"
}

static GLenum TextureTarget(GLenum target)
{
    return target == 0x0DE0 || target == 0x84F5 ? GL_TEXTURE_2D : target;
}

static texture_desc_t* BoundTexture(GLenum target)
{
    target = TextureTarget(target);
    GLenum binding = target == GL_TEXTURE_2D ? GL_TEXTURE_BINDING_2D :
        target == GL_TEXTURE_CUBE_MAP || (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z) ? GL_TEXTURE_BINDING_CUBE_MAP : 0;
    if(!binding) { SetError(GL_INVALID_ENUM); return nullptr; }
    GLint id = 0;
    glGetIntegerv(binding, &id);
    if(!id) return &globals->defaultTextures[binding];
    auto& texture = globals->textures[(GLuint)id];
    if(!texture) { texture = new texture_desc_t; texture->id = id; }
    return texture;
}

static unsigned long long LevelKey(GLenum target, GLint level)
{
    return ((unsigned long long)TextureTarget(target) << 32) | (GLuint)level;
}

template<class F> static bool TextureCall(F&& call)
{
    GLenum previous = glGetError();
    if(previous != GL_NO_ERROR) SetError(previous);
    call();
    GLenum error = glGetError();
    if(error != GL_NO_ERROR) SetError(error);
    return error == GL_NO_ERROR;
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

void WRAP(glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels))
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    bool color = format == GL_RED || format == 0x1904 || format == 0x1905 || format == GL_RG || format == GL_RGB || format == GL_RGBA || LegacyFormat(format);
    if(type != GL_FLOAT || !color || width <= 0 || height <= 0) { glReadPixels(x, y, width, height, format, type, pixels); return; }
    GLint readBuffer, component = 0;
    glGetIntegerv(GL_READ_BUFFER, &readBuffer);
    if(readBuffer != GL_NONE) glGetFramebufferAttachmentParameteriv(GL_READ_FRAMEBUFFER, readBuffer, GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, &component);
    if(component != GL_FLOAT) { glReadPixels(x, y, width, height, format, type, pixels); return; }
    pixel_store_t pack(true);
    size_t components = format == 0x1904 || format == 0x1905 ? 1 : Components(format);
    size_t pixelSize = components * sizeof(GLfloat), limit = (size_t)std::numeric_limits<GLsizeiptr>::max();
    size_t rowPixels = pack.row ? pack.row : width;
    if(rowPixels > (limit - pack.align + 1) / pixelSize || (size_t)width > limit / pixelSize) { SetError(GL_OUT_OF_MEMORY); return; }
    size_t stride = pack.Stride(width, pixelSize), rowBytes = (size_t)width * pixelSize;
    if((size_t)pack.rows > limit / stride || (size_t)pack.pixels > limit / pixelSize) { SetError(GL_OUT_OF_MEMORY); return; }
    size_t offset = (size_t)pack.rows * stride, skip = (size_t)pack.pixels * pixelSize;
    if(skip > limit - offset || rowBytes > limit - offset - skip) { SetError(GL_OUT_OF_MEMORY); return; }
    offset += skip;
    if((size_t)(height - 1) > (limit - offset - rowBytes) / stride) { SetError(GL_OUT_OF_MEMORY); return; }
    size_t bytes = offset + (size_t)(height - 1) * stride + rowBytes, base = (size_t)pixels;
    if(pack.buffer)
    {
        GLint mapped;
        GLint64 size;
        glGetBufferParameteriv(GL_PIXEL_PACK_BUFFER, GL_BUFFER_MAPPED, &mapped);
        glGetBufferParameteri64v(GL_PIXEL_PACK_BUFFER, GL_BUFFER_SIZE, &size);
        if(mapped || base % sizeof(GLfloat) || base > (size_t)size || bytes > (size_t)size - base) { SetError(GL_INVALID_OPERATION); return; }
    }
    else if(base > SIZE_MAX - bytes) { SetError(GL_INVALID_OPERATION); return; }
    if((size_t)width > limit / (4 * sizeof(GLfloat)) / (size_t)height) { SetError(GL_OUT_OF_MEMORY); return; }
    std::unique_ptr<GLfloat[]> rgba(new(std::nothrow) GLfloat[(size_t)width * height * 4]);
    if(!rgba) { SetError(GL_OUT_OF_MEMORY); return; }
    pack.Tight();
    if(!TextureCall([&]() { glReadPixels(x, y, width, height, GL_RGBA, GL_FLOAT, rgba.get()); })) return;
    unsigned char* destination = (unsigned char*)pixels;
    if(pack.buffer)
    {
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pack.buffer);
        destination = (unsigned char*)glMapBufferRange(GL_PIXEL_PACK_BUFFER, base, bytes, GL_MAP_WRITE_BIT);
        if(!destination) return;
    }
    bool clamp = globals->ff.clampReadColor == GL_TRUE;
    for(GLsizei row = 0; row < height; ++row) for(GLsizei column = 0; column < width; ++column)
    {
        const GLfloat* source = rgba.get() + ((size_t)row * width + column) * 4;
        GLfloat value[4] = {source[0],source[1],source[2],source[3]};
        if(format == GL_ALPHA) value[0] = source[3];
        else if(format == 0x1904) value[0] = source[1];
        else if(format == 0x1905) value[0] = source[2];
        else if(format == GL_LUMINANCE || format == GL_LUMINANCE_ALPHA) { value[0] = source[0] + source[1] + source[2]; value[1] = source[3]; }
        else if(format == 0x80E0 || format == 0x80E1) std::swap(value[0], value[2]);
        if(clamp) for(size_t i = 0; i < components; ++i) value[i] = std::min(std::max(value[i], 0.0f), 1.0f);
        memcpy(destination + offset + (size_t)row * stride + (size_t)column * pixelSize, value, pixelSize);
    }
    if(pack.buffer) glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
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
    if(internalformat == 0x8C40) return GL_SRGB8;
    if(internalformat == 0x8C42) return GL_SRGB8_ALPHA8;
    if(internalformat >= 0x8C44 && internalformat <= 0x8C47) return GL_SRGB8_ALPHA8;
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
    texture_desc_t* texture = BoundTexture(target);
    if(!texture) return;
    GLint original = internalformat;
    bool updated;
    internalformat = InternalFormat(internalformat);
    if(LegacyFormat(format) || type == 0x8367)
    {
        pixel_store_t store(false);
        std::vector<unsigned char> converted;
        if((data || store.buffer) && width && height && !ConvertPixels(width, height, format, type, data, store, converted)) return;
        store.Tight();
        if(internalformat == GL_RGB || internalformat == GL_RGBA) internalformat = GL_RGBA;
        if(internalformat == GL_SRGB8) internalformat = GL_SRGB8_ALPHA8;
        updated = TextureCall([&]() { glTexImage2D(target, level, internalformat, width, height, 0, GL_RGBA, type == 0x8367 ? GL_UNSIGNED_BYTE : type, converted.empty() ? nullptr : converted.data()); });
    }
    else updated = TextureCall([&]() { glTexImage2D(target, level, internalformat, width, height, 0, format, type, data); });
    if(updated)
    {
        texture->levels[LevelKey(target, level)] = texture_level_t((GLenum)original, width, height, {}, false);
        if(!level) texture->baseFormat = original;
    }
}

void WRAP(glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels))
{
    if(width < 0 || height < 0 || level < 0) { SetError(GL_INVALID_VALUE); return; }
    if(!width || !height) return;
    target = TextureTarget(target);
    texture_desc_t* texture = BoundTexture(target);
    if(!texture) return;
    bool updated;
    if(LegacyFormat(format) || type == 0x8367)
    {
        pixel_store_t store(false);
        std::vector<unsigned char> converted;
        if(!pixels && !store.buffer) { SetError(GL_INVALID_VALUE); return; }
        if(!ConvertPixels(width, height, format, type, pixels, store, converted)) return;
        store.Tight();
        updated = TextureCall([&]() { glTexSubImage2D(target, level, xoffset, yoffset, width, height, GL_RGBA, type == 0x8367 ? GL_UNSIGNED_BYTE : type, converted.data()); });
    }
    else updated = TextureCall([&]() { glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels); });
    auto stored = texture->levels.find(LevelKey(target, level));
    if(updated && stored != texture->levels.end()) stored->second.compressedValid = false;
}

void WRAP(glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border))
{
    target = TextureTarget(target);
    texture_desc_t* texture = BoundTexture(target);
    if(!texture) return;
    if(TextureCall([&]() { glCopyTexImage2D(target, level, InternalFormat(internalformat), x, y, width, height, border); }))
    {
        texture->levels[LevelKey(target, level)] = texture_level_t(internalformat, width, height, {}, false);
        if(!level) texture->baseFormat = internalformat;
    }
}

void WRAP(glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height))
{
    target = TextureTarget(target);
    texture_desc_t* texture = BoundTexture(target);
    if(!texture) return;
    bool updated = TextureCall([&]() { glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height); });
    auto stored = texture->levels.find(LevelKey(target, level));
    if(updated && width && height && stored != texture->levels.end()) stored->second.compressedValid = false;
}

void WRAP(glGenerateMipmap(GLenum target))
{
    target = TextureTarget(target);
    if(target != GL_TEXTURE_2D && target != GL_TEXTURE_CUBE_MAP) { glGenerateMipmap(target); return; }
    texture_desc_t* texture = BoundTexture(target);
    if(!texture || !TextureCall([&]() { glGenerateMipmap(target); })) return;
    GLint base = 0, maximum = 0;
    glGetTexParameteriv(target, GL_TEXTURE_BASE_LEVEL, &base);
    glGetTexParameteriv(target, GL_TEXTURE_MAX_LEVEL, &maximum);
    for(int face = 0; face < (target == GL_TEXTURE_CUBE_MAP ? 6 : 1); ++face)
    {
        GLenum imageTarget = target == GL_TEXTURE_CUBE_MAP ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + face : target;
        auto stored = texture->levels.find(LevelKey(imageTarget, base));
        if(stored == texture->levels.end()) continue;
        auto image = stored->second;
        image.compressed.clear();
        image.compressedValid = false;
        for(GLint level = base + 1; level <= maximum && (image.width > 1 || image.height > 1); ++level)
        {
            image.width = std::max(1, image.width / 2);
            image.height = std::max(1, image.height / 2);
            texture->levels[LevelKey(imageTarget, level)] = image;
        }
    }
}

static void InvalidateCompressed(GLuint id, GLint level)
{
    auto texture = globals->textures.find(id);
    if(texture == globals->textures.end()) return;
    texture->second->renderTarget = true;
    for(auto& image : texture->second->levels) if((GLuint)image.first == (GLuint)level) image.second.compressedValid = false;
}

void WRAP(glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level))
{
    if(TextureCall([&]() { glFramebufferTexture2D(target, attachment, TextureTarget(textarget), texture, level); })) InvalidateCompressed(texture, level);
}

void WRAP(glFramebufferTexture(GLenum target, GLenum attachment, GLuint texture, GLint level))
{
    if(TextureCall([&]() { glFramebufferTexture(target, attachment, texture, level); })) InvalidateCompressed(texture, level);
}

void WRAP(glFramebufferTextureLayer(GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer))
{
    if(TextureCall([&]() { glFramebufferTextureLayer(target, attachment, texture, level, layer); })) InvalidateCompressed(texture, level);
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

static bool DXTFormat(GLenum format)
{
    return (format >= 0x83F0 && format <= 0x83F3) || (format >= 0x8C4C && format <= 0x8C4F);
}

static GLenum DXTStorageFormat(GLenum format)
{
    return format >= 0x8C4C ? GL_SRGB8_ALPHA8 : GL_RGBA8;
}

void WRAP(glTexStorage2D(GLenum target, GLsizei levels, GLenum format, GLsizei width, GLsizei height))
{
    GLenum original = format;
    target = TextureTarget(target);
    if(DXTFormat(format) && !GLIN_HasCompressedFormat(format)) format = DXTStorageFormat(format);
    if(!TextureCall([&]() { glTexStorage2D(target, levels, format, width, height); })) return;
    texture_desc_t* texture = BoundTexture(target);
    if(!texture) return;
    texture->levels.clear();
    for(GLint level = 0; level < levels; ++level)
    {
        for(int face = 0; face < (target == GL_TEXTURE_CUBE_MAP ? 6 : 1); ++face)
            texture->levels[LevelKey(target == GL_TEXTURE_CUBE_MAP ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + face : target, level)] = texture_level_t(original, width, height);
        width = std::max(1, width / 2);
        height = std::max(1, height / 2);
    }
}

static bool DecompressDXT(GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void* data,
                          pixel_store_t& store, std::vector<unsigned char>& output, std::vector<unsigned char>& compressed, bool native)
{
    unsigned int kind = format >= 0x8C4C ? format - 0x8C4C : format - 0x83F0;
    size_t blockSize = kind < 2 ? 8 : 16;
    size_t blocksX = ((size_t)width + 3) / 4, blocksY = ((size_t)height + 3) / 4;
    size_t limit = std::numeric_limits<size_t>::max();
    if((blocksY && blocksX > limit / blockSize / blocksY) || (height && (size_t)width > limit / 4 / (size_t)height))
    {
        SetError(GL_INVALID_VALUE);
        return false;
    }
    size_t needed = blocksX * blocksY * blockSize;
    if((size_t)imageSize != needed) { SetError(GL_INVALID_VALUE); return false; }
    compressed.resize(needed);
    const unsigned char* source = (const unsigned char*)data;
    void* mapped = nullptr;
    if(store.buffer && needed)
    {
        GLint64 size;
        glGetBufferParameteri64v(GL_PIXEL_UNPACK_BUFFER, GL_BUFFER_SIZE, &size);
        size_t offset = (size_t)data;
        if(offset > (size_t)size || needed > (size_t)size - offset) { SetError(GL_INVALID_OPERATION); return false; }
        mapped = glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, offset, needed, GL_MAP_READ_BIT);
        if(!mapped) return false;
        source = (const unsigned char*)mapped;
    }
    if(source && needed)
    {
        memcpy(compressed.data(), source, needed);
        if(!native) output.resize((size_t)width * height * 4);
        for(GLsizei y = 0; !native && y < height; y += 4) for(GLsizei x = 0; x < width; x += 4)
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
    return true;
}

void WRAP(glCompressedTexImage2D(GLenum target, GLint level, GLenum format, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data))
{
    target = TextureTarget(target);
    if(!DXTFormat(format))
    {
        texture_desc_t* texture = BoundTexture(target);
        if(!texture) return;
        if(TextureCall([&]() { glCompressedTexImage2D(target, level, format, width, height, border, imageSize, data); }))
        {
            texture->levels[LevelKey(target, level)] = texture_level_t(format, width, height, {}, false);
            if(!level) texture->baseFormat = format;
        }
        return;
    }
    if(width < 0 || height < 0 || imageSize < 0 || border || level < 0) { SetError(GL_INVALID_VALUE); return; }
    texture_desc_t* texture = BoundTexture(target);
    if(!texture) return;
    bool native = GLIN_HasCompressedFormat(format);
    pixel_store_t store(false);
    std::vector<unsigned char> output, compressed;
    if(!DecompressDXT(format, width, height, imageSize, data, store, output, compressed, native)) return;
    store.Tight();
    bool updated = TextureCall([&]() {
        if(native) glCompressedTexImage2D(target, level, format, width, height, 0, imageSize, data || store.buffer ? compressed.data() : nullptr);
        else glTexImage2D(target, level, DXTStorageFormat(format), width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, output.empty() ? nullptr : output.data());
    });
    if(updated)
    {
        texture->levels[LevelKey(target, level)] = texture_level_t(format, width, height, std::move(compressed));
        if(!level) texture->baseFormat = format == 0x83F0 || format == 0x8C4C ? GL_RGB : GL_RGBA;
    }
}

void WRAP(glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data))
{
    target = TextureTarget(target);
    if(!DXTFormat(format))
    {
        glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
        return;
    }
    if(width < 0 || height < 0 || imageSize < 0 || level < 0 || xoffset < 0 || yoffset < 0) { SetError(GL_INVALID_VALUE); return; }
    texture_desc_t* texture = BoundTexture(target);
    if(!texture) return;
    auto stored = texture->levels.find(LevelKey(target, level));
    if(stored == texture->levels.end() || stored->second.format != format) { SetError(GL_INVALID_OPERATION); return; }
    bool native = GLIN_HasCompressedFormat(format);
    GLint textureWidth = 0, textureHeight = 0;
    glGetTexLevelParameteriv(target, level, GL_TEXTURE_WIDTH, &textureWidth);
    glGetTexLevelParameteriv(target, level, GL_TEXTURE_HEIGHT, &textureHeight);
    if(xoffset > textureWidth || width > textureWidth - xoffset || yoffset > textureHeight || height > textureHeight - yoffset)
    {
        SetError(GL_INVALID_VALUE);
        return;
    }
    if(xoffset % 4 || yoffset % 4 || (width % 4 && xoffset + width != textureWidth) || (height % 4 && yoffset + height != textureHeight))
    {
        SetError(GL_INVALID_OPERATION);
        return;
    }
    pixel_store_t store(false);
    std::vector<unsigned char> output, compressed;
    if(width && height && !data && !store.buffer) { SetError(GL_INVALID_VALUE); return; }
    if(!DecompressDXT(format, width, height, imageSize, data, store, output, compressed, native)) return;
    if(!width || !height) return;
    store.Tight();
    bool updated = TextureCall([&]() {
        if(native) glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, compressed.data());
        else glTexSubImage2D(target, level, xoffset, yoffset, width, height, GL_RGBA, GL_UNSIGNED_BYTE, output.data());
    });
    if(updated)
    {
        size_t blockSize = format == 0x83F0 || format == 0x83F1 || format == 0x8C4C || format == 0x8C4D ? 8 : 16;
        size_t rowBytes = ((size_t)width + 3) / 4 * blockSize;
        size_t stride = ((size_t)textureWidth + 3) / 4 * blockSize;
        size_t required = stride * (((size_t)textureHeight + 3) / 4);
        stored->second.compressed.resize(required);
        for(GLsizei y = 0; y < (height + 3) / 4; ++y)
            memcpy(stored->second.compressed.data() + (yoffset / 4 + y) * stride + (xoffset / 4) * blockSize, compressed.data() + y * rowBytes, rowBytes);
        if(!xoffset && !yoffset && width == textureWidth && height == textureHeight) stored->second.compressedValid = true;
    }
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
    WRAP(glFramebufferTextureLayer(target, attachment, texture, level, layer));
}

void WRAP(glActiveTexture(GLenum unit))
{
    GLint count;
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &count);
    if(unit < GL_TEXTURE0 || unit - GL_TEXTURE0 >= (GLuint)count) { SetError(GL_INVALID_ENUM); return; }
    glActiveTexture(unit);
    globals->gl.activeTexUnit = unit;
    globals->ff.activeTextureUnit = (GLint)(unit - GL_TEXTURE0);
    if(unit < GL_TEXTURE0 + 8) globals->matrix.textureUnit = unit - GL_TEXTURE0;
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

void WRAP(glCompressedTexImage1D(GLenum target, GLint level, GLenum format, GLsizei width, GLint border, GLsizei imageSize, const void* data))
{
    if(target != 0x0DE0 || DXTFormat(format)) { SetError(GL_INVALID_ENUM); return; }
    WRAP(glCompressedTexImage2D(target, level, format, width, 1, border, imageSize, data));
}

void WRAP(glCompressedTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void* data))
{
    if(target != 0x0DE0 || DXTFormat(format)) { SetError(GL_INVALID_ENUM); return; }
    WRAP(glCompressedTexSubImage2D(target, level, xoffset, 0, width, 1, format, imageSize, data));
}

void WRAP(glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint* params))
{
    if(level < 0) { SetError(GL_INVALID_VALUE); return; }
    target = TextureTarget(target);
    if(target == GL_TEXTURE_2D || (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z))
    {
        texture_desc_t* texture = BoundTexture(target);
        if(!texture) return;
        auto stored = texture->levels.find(LevelKey(target, level));
        if(stored != texture->levels.end())
        {
            const auto& image = stored->second;
            if(pname == GL_TEXTURE_INTERNAL_FORMAT) { *params = image.format; return; }
            if(pname == 0x86A1 && DXTFormat(image.format)) { *params = GL_TRUE; return; }
            if(pname == 0x86A0 && DXTFormat(image.format))
            {
                size_t blockSize = image.format == 0x83F0 || image.format == 0x83F1 || image.format == 0x8C4C || image.format == 0x8C4D ? 8 : 16;
                *params = (GLint)(((size_t)image.width + 3) / 4 * (((size_t)image.height + 3) / 4) * blockSize);
                return;
            }
        }
    }
    glGetTexLevelParameteriv(target, level, pname, params);
}

void WRAP(glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat* params))
{
    GLint value = 0;
    WRAP(glGetTexLevelParameteriv(target, level, pname, &value));
    *params = (GLfloat)value;
}

void WRAP(glGetCompressedTexImage(GLenum target, GLint level, GLvoid* pixels))
{
    if(level < 0) { SetError(GL_INVALID_VALUE); return; }
    texture_desc_t* texture = BoundTexture(target);
    if(!texture) return;
    auto stored = texture->levels.find(LevelKey(target, level));
    if(stored == texture->levels.end() || !DXTFormat(stored->second.format) || !stored->second.compressedValid || texture->renderTarget) { SetError(GL_INVALID_OPERATION); return; }
    auto& data = stored->second.compressed;
    GLint size = 0;
    WRAP(glGetTexLevelParameteriv(target, level, 0x86A0, &size));
    if(!size) return;
    if(data.empty()) data.resize(size);
    pixel_store_t store(true);
    if(store.buffer)
    {
        GLint64 bufferSize = 0;
        glGetBufferParameteri64v(GL_PIXEL_PACK_BUFFER, GL_BUFFER_SIZE, &bufferSize);
        size_t offset = (size_t)pixels;
        if(offset > (size_t)bufferSize || data.size() > (size_t)bufferSize - offset) { SetError(GL_INVALID_OPERATION); return; }
        void* mapped = glMapBufferRange(GL_PIXEL_PACK_BUFFER, offset, data.size(), GL_MAP_WRITE_BIT);
        if(!mapped) return;
        memcpy(mapped, data.data(), data.size());
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    }
    else if(pixels) memcpy(pixels, data.data(), data.size());
}

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
