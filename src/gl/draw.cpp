#include "wrapped.h"
#include "glhelper.h"
#include "draw_state.h"
#include <algorithm>
#include <limits>
#include <memory>

GLINAPI void* GLIN_GetBackendProc(const char* name);

static bool UploadAttribute(GLuint location, GLuint staging, GLint size, GLenum type, GLsizei stride,
                            GLuint buffer, const void* pointer, size_t count, bool normalized)
{
    bool bgra = size == 0x80E1;
    size_t packed = GetArrayElementSize(size, type), step = stride ? (size_t)stride : packed;
    if(bgra || IsPackedVertex(type)) size = 4;
    if(size < 1 || size > 4 || !packed || stride < 0) { SetError(GL_INVALID_VALUE); return false; }
    glEnableVertexAttribArray(location);
    if(buffer && type != 0x140A && !bgra)
    {
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glVertexAttribPointer(location, size, type, normalized, stride, pointer);
        return true;
    }
    if(!pointer && !buffer) { SetError(GL_INVALID_OPERATION); return false; }
    if(count > (size_t)std::numeric_limits<GLsizeiptr>::max() / step) { SetError(GL_OUT_OF_MEMORY); return false; }
    glBindBuffer(GL_ARRAY_BUFFER, staging);
    if(type == 0x140A || bgra)
    {
        GLint previous = 0;
        const void* source = pointer;
        if(buffer)
        {
            glGetIntegerv(GL_COPY_READ_BUFFER_BINDING, &previous);
            glBindBuffer(GL_COPY_READ_BUFFER, buffer);
            source = glMapBufferRange(GL_COPY_READ_BUFFER, (GLintptr)pointer, count ? (count - 1) * step + packed : 0, GL_MAP_READ_BIT);
            if(!source) { glBindBuffer(GL_COPY_READ_BUFFER, previous); return false; }
        }
        std::vector<float> converted(count * size);
        for(size_t i = 0; i < count; ++i)
        {
            const char* element = (const char*)source + i * step;
            float* output = converted.data() + i * size;
            if(IsPackedVertex(type))
            {
                GLuint value;
                memcpy(&value, element, sizeof(value));
                UnpackVertex(type, value, normalized, output);
            }
            else for(GLint j = 0; j < size; ++j)
            {
                if(type == GL_UNSIGNED_BYTE) output[j] = (unsigned char)element[j] / 255.0f;
                else
                {
                    double value;
                    memcpy(&value, element + j * sizeof(double), sizeof(value));
                    output[j] = (float)value;
                }
            }
            if(bgra) std::swap(output[0], output[2]);
        }
        if(buffer) { glUnmapBuffer(GL_COPY_READ_BUFFER); glBindBuffer(GL_COPY_READ_BUFFER, previous); }
        glBufferData(GL_ARRAY_BUFFER, converted.size() * sizeof(float), converted.data(), GL_STREAM_DRAW);
        glVertexAttribPointer(location, size, GL_FLOAT, GL_FALSE, 0, nullptr);
    }
    else
    {
        glBufferData(GL_ARRAY_BUFFER, count ? (count - 1) * step + packed : 0, pointer, GL_STREAM_DRAW);
        glVertexAttribPointer(location, size, type, normalized, stride, nullptr);
    }
    return true;
}

static bool PrepareArrays(size_t count)
{
    auto& state = globals->client;
    auto& render = globals->render;
    if(!render.fixedVAO)
    {
        glGenVertexArrays(1, &render.fixedVAO);
        glGenBuffers(13, render.fixedVBO);
    }
    glBindVertexArray(render.fixedVAO);
    if(!UploadAttribute(0, render.fixedVBO[0], state.vertexSize, state.vertexType, state.vertexStride, state.vertexBuffer, state.vertexPtr, count, false)) return false;
    if(state.colorArrayEnabled)
    {
        if(!UploadAttribute(3, render.fixedVBO[1], state.colorSize, state.colorType, state.colorStride, state.colorBuffer, state.colorPtr, count, true)) return false;
    }
    else { glDisableVertexAttribArray(3); glVertexAttrib4fv(3, &render.color.x); }
    if(state.normalArrayEnabled)
    {
        if(!UploadAttribute(2, render.fixedVBO[2], 3, state.normalType, state.normalStride, state.normalBuffer, state.normalPtr, count, true)) return false;
    }
    else { glDisableVertexAttribArray(2); glVertexAttrib3f(2, render.normal.x, render.normal.y, render.normal.z); }
    if(state.secondaryColorArrayEnabled)
    {
        if(!UploadAttribute(4, render.fixedVBO[11], state.secondaryColorSize, state.secondaryColorType, state.secondaryColorStride, state.secondaryColorBuffer, state.secondaryColorPtr, count, true)) return false;
    }
    else { glDisableVertexAttribArray(4); glVertexAttrib3f(4, render.secondaryColor.x, render.secondaryColor.y, render.secondaryColor.z); }
    if(state.fogCoordArrayEnabled)
    {
        if(!UploadAttribute(5, render.fixedVBO[12], 1, state.fogCoordType, state.fogCoordStride, state.fogCoordBuffer, state.fogCoordPtr, count, false)) return false;
    }
    else { glDisableVertexAttribArray(5); glVertexAttrib1f(5, render.fogCoord); }
    for(int i = 0; i < 8; ++i)
    {
        auto& tex = state.texCoord[i];
        if(tex.enabled)
        {
            if(!UploadAttribute(8+i, render.fixedVBO[3+i], tex.texCoordSize, tex.texCoordType, tex.texCoordStride, tex.texCoordBuffer, tex.texCoordPtr, count, false)) return false;
        }
        else { glDisableVertexAttribArray(8+i); glVertexAttrib4fv(8+i, i ? &render.multiTexcoord[i-1].x : &render.texcoord.x); }
    }
    return true;
}

static bool ReadIndices(GLsizei count, GLenum type, const void* indices, GLint base, std::vector<GLuint>& output)
{
    size_t size = type == GL_UNSIGNED_BYTE ? 1 : type == GL_UNSIGNED_SHORT ? 2 : type == GL_UNSIGNED_INT ? 4 : 0;
    if(!size) { SetError(GL_INVALID_ENUM); return false; }
    GLint buffer;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &buffer);
    const unsigned char* data = (const unsigned char*)indices;
    void* mapped = nullptr;
    if(buffer)
    {
        GLint64 bytes;
        glGetBufferParameteri64v(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &bytes);
        size_t offset = (size_t)indices, required = (size_t)count * size;
        if(offset > (size_t)bytes || required > (size_t)bytes - offset) { SetError(GL_INVALID_OPERATION); return false; }
        mapped = glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, offset, required, GL_MAP_READ_BIT);
        if(!mapped) return false;
        data = (const unsigned char*)mapped;
    }
    if(!data) { SetError(GL_INVALID_OPERATION); return false; }
    output.resize(count);
    for(GLsizei i = 0; i < count; ++i)
    {
        GLuint value = 0;
        memcpy(&value, data + (size_t)i * size, size);
        int64_t adjusted = (int64_t)value + base;
        if(adjusted < 0 || adjusted > std::numeric_limits<GLuint>::max())
        {
            if(mapped) glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
            SetError(GL_INVALID_OPERATION);
            return false;
        }
        output[i] = (GLuint)adjusted;
    }
    if(mapped) glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
    return true;
}

static auto NativeProvokingVertex() -> void(*)(GLenum)
{
    if(GLIN_HasExtension("GL_EXT_provoking_vertex")) return (void(*)(GLenum))GLIN_GetBackendProc("glProvokingVertexEXT");
    if(GLIN_HasExtension("GL_ANGLE_provoking_vertex")) return (void(*)(GLenum))GLIN_GetBackendProc("glProvokingVertexANGLE");
    return nullptr;
}

void WRAP(glProvokingVertex(GLenum mode))
{
    DLREC(WRAP(glProvokingVertex(mode)));
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(mode != 0x8E4D && mode != 0x8E4E) { SetError(GL_INVALID_ENUM); return; }
    if(auto native = NativeProvokingVertex()) native(mode);
    globals->ff.provokingVertex = mode;
}

static void ConvertPrimitive(GLenum& mode, std::vector<GLuint>& indices, bool nativeFirst)
{
    bool first = globals->ff.provokingVertex == 0x8E4D;
    bool reorder = first && !nativeFirst;
    char polygonMode = globals->gl.lastPolygonMode;
    if(mode == GL_POINTS || mode >= GL_LINES_ADJACENCY) return;
    if(mode <= GL_LINE_STRIP)
    {
        if(!reorder) return;
        std::vector<GLuint> lines;
        size_t step = mode == GL_LINES ? 2 : 1;
        for(size_t i = 0; i + 1 < indices.size(); i += step) lines.insert(lines.end(), {indices[i+1], indices[i]});
        if(mode == GL_LINE_LOOP && indices.size() > 1) lines.insert(lines.end(), {indices[0], indices.back()});
        indices.swap(lines);
        mode = GL_LINES;
        return;
    }
    if(polygonMode == 1) { mode = GL_POINTS; return; }
    if(!polygonMode && mode <= GL_TRIANGLE_FAN && !reorder) return;
    std::vector<GLuint> converted;
    auto polygon = [&](std::vector<GLuint> v, GLuint provoking) {
        if(polygonMode == 2)
        {
            for(size_t i = 0; i < v.size(); ++i) converted.insert(converted.end(), {v[i], v[(i+1)%v.size()]});
        }
        else
        {
            auto selected = std::find(v.begin(), v.end(), provoking);
            std::rotate(v.begin(), selected, v.end());
            for(size_t i = 1; i + 1 < v.size(); ++i)
            {
                if(nativeFirst) converted.insert(converted.end(), {v[0], v[i], v[i+1]});
                else converted.insert(converted.end(), {v[i], v[i+1], v[0]});
            }
        }
    };
    if(mode == GL_TRIANGLES)
        for(size_t i = 0; i + 2 < indices.size(); i += 3) polygon({indices[i], indices[i+1], indices[i+2]}, indices[i + (first ? 0 : 2)]);
    else if(mode == GL_TRIANGLE_STRIP)
        for(size_t i = 0; i + 2 < indices.size(); ++i) polygon({indices[i + (i & 1)], indices[i + 1 - (i & 1)], indices[i+2]}, indices[i + (first ? 0 : 2)]);
    else if(mode == GL_TRIANGLE_FAN)
        for(size_t i = 1; i + 1 < indices.size(); ++i) polygon({indices[0], indices[i], indices[i+1]}, indices[i + (first ? 0 : 1)]);
    else if(mode == GL_QUADS)
        for(size_t i = 0; i + 3 < indices.size(); i += 4) polygon({indices[i], indices[i+1], indices[i+2], indices[i+3]}, indices[i + (first ? 0 : 3)]);
    else if(mode == 8)
        for(size_t i = 0; i + 3 < indices.size(); i += 2) polygon({indices[i], indices[i+1], indices[i+3], indices[i+2]}, indices[i + (first ? 0 : 3)]);
    else if(indices.size() >= 3) polygon(indices, indices[0]);
    mode = polygonMode == 2 ? GL_LINES : GL_TRIANGLES;
    indices.swap(converted);
}

struct captured_arrays_t
{
    client_state_t state;
    std::vector<unsigned char> data[13];
    std::vector<GLuint> indices;
};

static bool CaptureAttribute(std::vector<unsigned char>& output, GLuint buffer, const void* pointer,
                             GLint components, GLenum type, GLsizei stride, size_t count)
{
    size_t packed = GetArrayElementSize(components, type), step = stride ? (size_t)stride : packed;
    if(components == 0x80E1) components = 4;
    if(!packed || stride < 0 || components < 1 || components > 4) { SetError(GL_INVALID_VALUE); return false; }
    size_t bytes = (count - 1) * step + packed;
    output.resize(bytes);
    if(buffer)
    {
        GLint previous;
        glGetIntegerv(GL_COPY_READ_BUFFER_BINDING, &previous);
        glBindBuffer(GL_COPY_READ_BUFFER, buffer);
        GLint64 size;
        glGetBufferParameteri64v(GL_COPY_READ_BUFFER, GL_BUFFER_SIZE, &size);
        size_t offset = (size_t)pointer;
        if(offset > (size_t)size || bytes > (size_t)size - offset)
        {
            glBindBuffer(GL_COPY_READ_BUFFER, previous);
            SetError(GL_INVALID_OPERATION);
            return false;
        }
        void* mapped = glMapBufferRange(GL_COPY_READ_BUFFER, offset, bytes, GL_MAP_READ_BIT);
        if(mapped) { memcpy(output.data(), mapped, bytes); glUnmapBuffer(GL_COPY_READ_BUFFER); }
        glBindBuffer(GL_COPY_READ_BUFFER, previous);
        return mapped != nullptr;
    }
    if(!pointer) { SetError(GL_INVALID_OPERATION); return false; }
    memcpy(output.data(), pointer, bytes);
    return true;
}

static void Draw(GLenum mode, GLint first, GLsizei count, GLenum type, const void* indices, GLint base, GLsizei instances = 1)
{
    if(instances < 0) { SetError(GL_INVALID_VALUE); return; }
    if(!instances) return;
    if(count < 0 || first < 0) { SetError(GL_INVALID_VALUE); return; }
    if(mode > GL_TRIANGLE_STRIP_ADJACENCY) { SetError(GL_INVALID_ENUM); return; }
    if(!count) return;
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(!globals->currentList && globals->gl.conditionalDiscard) return;
    if(type && globals->gl.primitiveRestart)
    {
        std::vector<GLuint> values;
        if(!ReadIndices(count, type, indices, 0, values)) return;
        GLint previous = 0;
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &previous);
        GLuint staging = 0;
        glGenBuffers(1, &staging);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, staging);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, values.size() * sizeof(GLuint), values.data(), GL_STREAM_DRAW);
        globals->gl.primitiveRestart = false;
        size_t firstIndex = 0;
        for(size_t i = 0; i <= values.size(); ++i)
        {
            if(i != values.size() && values[i] != globals->gl.restartIndex) continue;
            if(i > firstIndex)
                Draw(mode, 0, (GLsizei)(i - firstIndex), GL_UNSIGNED_INT,
                     (const void*)(firstIndex * sizeof(GLuint)), base, instances);
            firstIndex = i + 1;
        }
        globals->gl.primitiveRestart = true;
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, previous);
        glDeleteBuffers(1, &staging);
        return;
    }
    bool nativeFirst = globals->ff.provokingVertex == 0x8E4D && NativeProvokingVertex();
    bool reorder = globals->ff.provokingVertex == 0x8E4D && !nativeFirst && mode != GL_POINTS;
    if(reorder && !globals->currentList)
    {
        GLboolean feedback = GL_FALSE, paused = GL_FALSE;
        glGetBooleanv(GL_TRANSFORM_FEEDBACK_ACTIVE, &feedback);
        glGetBooleanv(GL_TRANSFORM_FEEDBACK_PAUSED, &paused);
        if(feedback && !paused) { SetError(GL_INVALID_OPERATION); return; }
        if(globals->gl.activeProgram)
        {
            auto program = globals->objects->geometryPrograms.find(globals->gl.activeProgram);
            if(program == globals->objects->geometryPrograms.end() || program->second) { SetError(GL_INVALID_OPERATION); return; }
        }
    }
    bool legacy = globals->client.vertexArrayEnabled;
    if(!legacy && !reorder && (mode <= GL_TRIANGLE_FAN || mode >= GL_LINES_ADJACENCY) && !globals->currentList && !globals->gl.lastPolygonMode)
    {
        if(type) glDrawElementsInstancedBaseVertex(mode, count, type, indices, instances, base);
        else glDrawArraysInstanced(mode, first, count, instances);
        return;
    }
    std::vector<GLuint> elements;
    if(type)
    {
        if(!ReadIndices(count, type, indices, base, elements)) return;
    }
    else
    {
        elements.resize(count);
        for(GLsizei i = 0; i < count; ++i) elements[i] = (GLuint)first + (GLuint)i;
    }
    size_t vertices = (size_t)*std::max_element(elements.begin(), elements.end()) + 1;
    if(globals->currentList)
    {
        if(!legacy) { SetError(GL_INVALID_OPERATION); return; }
        auto captured = std::make_shared<captured_arrays_t>();
        captured->state = globals->client;
        captured->indices = elements;
        auto& state = captured->state;
        if(!CaptureAttribute(captured->data[0], state.vertexBuffer, state.vertexPtr, state.vertexSize, state.vertexType, state.vertexStride, vertices)) return;
        state.vertexBuffer = 0; state.vertexPtr = captured->data[0].data();
        if(state.colorArrayEnabled)
        {
            if(!CaptureAttribute(captured->data[1], state.colorBuffer, state.colorPtr, state.colorSize, state.colorType, state.colorStride, vertices)) return;
            state.colorBuffer = 0; state.colorPtr = captured->data[1].data();
        }
        if(state.normalArrayEnabled)
        {
            if(!CaptureAttribute(captured->data[2], state.normalBuffer, state.normalPtr, 3, state.normalType, state.normalStride, vertices)) return;
            state.normalBuffer = 0; state.normalPtr = captured->data[2].data();
        }
        if(state.secondaryColorArrayEnabled)
        {
            if(!CaptureAttribute(captured->data[11], state.secondaryColorBuffer, state.secondaryColorPtr, state.secondaryColorSize, state.secondaryColorType, state.secondaryColorStride, vertices)) return;
            state.secondaryColorBuffer = 0; state.secondaryColorPtr = captured->data[11].data();
        }
        if(state.fogCoordArrayEnabled)
        {
            if(!CaptureAttribute(captured->data[12], state.fogCoordBuffer, state.fogCoordPtr, 1, state.fogCoordType, state.fogCoordStride, vertices)) return;
            state.fogCoordBuffer = 0; state.fogCoordPtr = captured->data[12].data();
        }
        for(int i = 0; i < 8; ++i)
        {
            auto& tex = state.texCoord[i];
            if(!tex.enabled) continue;
            if(!CaptureAttribute(captured->data[3+i], tex.texCoordBuffer, tex.texCoordPtr, tex.texCoordSize, tex.texCoordType, tex.texCoordStride, vertices)) return;
            tex.texCoordBuffer = 0; tex.texCoordPtr = captured->data[3+i].data();
        }
        globals->lists[globals->currentList]->commands.push_back([=]() {
            client_state_t previous = globals->client;
            globals->client = captured->state;
            GLint element;
            glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &element);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            Draw(mode, 0, (GLsizei)captured->indices.size(), GL_UNSIGNED_INT, captured->indices.data(), 0);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element);
            globals->client = previous;
        });
        if(globals->currentListMode == GL_COMPILE) return;
    }
    list_record_guard_t recording;
    ConvertPrimitive(mode, elements, nativeFirst);
    if(elements.empty()) return;
    draw_state_t saved;
    if(legacy)
    {
        UseFixedProgram(mode);
        if(!PrepareArrays(vertices)) return;
    }
    GLint oldElement;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &oldElement);
    GLuint element;
    glGenBuffers(1, &element);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size() * sizeof(GLuint), elements.data(), GL_STREAM_DRAW);
    glDrawElementsInstanced(mode, (GLsizei)elements.size(), GL_UNSIGNED_INT, nullptr, instances);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, oldElement);
    glDeleteBuffers(1, &element);
}

void WRAP(glDrawArrays(GLenum mode, GLint first, GLsizei count)) { Draw(mode, first, count, 0, nullptr, 0); }
void WRAP(glDrawArraysInstanced(GLenum mode, GLint first, GLsizei count, GLsizei instances)) { Draw(mode, first, count, 0, nullptr, 0, instances); }
void WRAP(glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices)) { Draw(mode, 0, count, type, indices, 0); }
void WRAP(glDrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type, const void* indices, GLint base)) { Draw(mode, 0, count, type, indices, base); }
void WRAP(glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void* indices))
{
    if(end < start) { SetError(GL_INVALID_VALUE); return; }
    Draw(mode, 0, count, type, indices, 0);
}

void WRAP(glPrimitiveRestartIndex(GLuint index))
{
    DLREC(WRAP(glPrimitiveRestartIndex(index)));
    globals->gl.restartIndex = index;
}

void WRAP(glDrawElementsInstanced(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instances))
{
    Draw(mode, 0, count, type, indices, 0, instances);
}

void WRAP(glDrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type, const void* indices, GLsizei instances, GLint base))
{
    Draw(mode, 0, count, type, indices, base, instances);
}
