#include "gl_shader.h"
#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>
#include <string>
#include <unordered_set>
#include "shader_translate.h"

static thread_local std::string convertedSource;
GLINAPI void* GLIN_GetBackendProc(const char* name);

void PreprocessShader(char*, bool) {}

const char* ConvertShader(const char* source, GLenum stage)
{
    bool vertex = stage == GL_VERTEX_SHADER, fragment = stage == GL_FRAGMENT_SHADER;
    int version = 110;
    std::string body, extensions;
    std::istringstream lines(source ? source : "");
    std::string line;
    while(std::getline(lines, line))
    {
        size_t first = line.find_first_not_of(" \t\r");
        size_t name = first != std::string::npos && line[first] == '#' ? line.find_first_not_of(" \t", first + 1) : std::string::npos;
        if(name != std::string::npos && line.compare(name, 7, "version") == 0 && name + 7 < line.size() && std::isspace((unsigned char)line[name+7]))
        {
            std::istringstream directive(line.substr(name + 7));
            directive >> version;
            continue;
        }
        if(name != std::string::npos && line.compare(name, 9, "extension") == 0 && name + 9 < line.size() && std::isspace((unsigned char)line[name+9]))
        {
            if(line.find("GL_ARB_") == std::string::npos) extensions += line + '\n';
            continue;
        }
        body += line + '\n';
    }
    std::unordered_set<std::string> identifiers;
    std::string rewritten;
    for(size_t i = 0; i < body.size();)
    {
        if(body.compare(i, 2, "//") == 0 || body.compare(i, 2, "/*") == 0)
        {
            bool block = body[i + 1] == '*';
            size_t end = body.find(block ? "*/" : "\n", i + 2);
            end = end == std::string::npos ? body.size() : end + (block ? 2 : 1);
            rewritten.append(body, i, end - i);
            i = end;
            continue;
        }
        if(std::isalpha((unsigned char)body[i]) || body[i] == '_')
        {
            size_t end = i + 1;
            while(end < body.size() && (std::isalnum((unsigned char)body[end]) || body[end] == '_')) ++end;
            std::string token = body.substr(i, end - i);
            identifiers.insert(token);
            if(token == "__VERSION__") token = "GLIN_VERSION";
            else if(token == "GL_ES") token = "GLIN_ES";
            else if(token == "GL_core_profile") token = "GLIN_core_profile";
            else if(token == "GL_compatibility_profile") token = "GLIN_compatibility_profile";
            else if(token == "attribute" && vertex) token = "in";
            else if(token == "varying" && (vertex || fragment)) token = vertex ? "out" : "in";
            else if(token == "texture2D" || token == "texture3D" || token == "textureCube") token = "texture";
            else if(token == "texture2DProj") token = "textureProj";
            else if(token == "texture2DLod" || token == "textureCubeLod") token = "textureLod";
            else if(token == "texture2DGradARB") token = "textureGrad";
            else if(token == "gl_Color" && (vertex || fragment)) token = vertex ? "GLIN_Color" : "GLIN_FrontColor";
            else if(token == "gl_FragColor" && fragment) token = "GLIN_FragData[0]";
            else if(token == "gl_FragData" && fragment) token = "GLIN_FragData";
            else if((vertex || fragment) && (token == "gl_Vertex" || token == "gl_Normal" || token == "gl_FrontColor" ||
                    token == "gl_BackColor" || token == "gl_MultiTexCoord0" ||
                    token == "gl_ModelViewProjectionMatrix" || token == "gl_ModelViewMatrix" ||
                    token == "gl_ProjectionMatrix" || token == "gl_NormalMatrix" || token == "gl_TexCoord"))
                token = "GLIN_" + token.substr(3);
            else if(vertex && token.compare(0, 15, "gl_MultiTexCoord") == 0) token = "GLIN_" + token.substr(3);
            rewritten += token;
            i = end;
        }
        else rewritten += body[i++];
    }
    auto uses = [&](const char* name) { return identifiers.count(name) != 0; };
    shader_rewrite_t rewrite(rewritten);
    if(fragment && rewrite.HasOutputIndex() && GLIN_HasExtension("GL_EXT_blend_func_extended")) extensions += "#extension GL_EXT_blend_func_extended : enable\n";
    convertedSource = "#version 320 es\n" + extensions + "precision highp float;\nprecision highp int;\nprecision highp sampler3D;\n";
    convertedSource += "#define GLIN_VERSION " + std::to_string(version) + "\n";
    if(version >= 150) convertedSource += "#define GLIN_core_profile 1\n#define GLIN_compatibility_profile 1\n";
    const char* samplerTypes[] = {"sampler2D", "samplerCube", "sampler2DArray", "sampler2DShadow", "samplerCubeShadow",
        "sampler2DArrayShadow", "samplerBuffer", "sampler2DMS", "sampler2DMSArray", "samplerCubeArray", "samplerCubeArrayShadow",
        "isampler2D", "isampler3D", "isamplerCube", "isampler2DArray", "isamplerBuffer", "isampler2DMS", "isampler2DMSArray", "isamplerCubeArray",
        "usampler2D", "usampler3D", "usamplerCube", "usampler2DArray", "usamplerBuffer", "usampler2DMS", "usampler2DMSArray", "usamplerCubeArray"};
    for(const char* type : samplerTypes)
        if(uses(type)) convertedSource += std::string("precision highp ") + type + ";\n";
    if(vertex)
    {
        if(uses("gl_Vertex") || uses("ftransform")) convertedSource += "layout(location=0) in vec4 GLIN_Vertex;\n";
        if(uses("gl_Normal")) convertedSource += "layout(location=2) in vec3 GLIN_Normal;\n";
        if(uses("gl_Color")) convertedSource += "layout(location=3) in vec4 GLIN_Color;\n";
        for(int i = 0; i < 8; ++i)
        {
            std::string name = "gl_MultiTexCoord" + std::to_string(i);
            if(identifiers.count(name)) convertedSource += "layout(location=" + std::to_string(8+i) + ") in vec4 GLIN_" + name.substr(3) + ";\n";
        }
    }
    if((vertex || fragment) && (uses("gl_ModelViewProjectionMatrix") || uses("ftransform"))) convertedSource += "uniform mat4 GLIN_ModelViewProjectionMatrix;\n";
    if((vertex || fragment) && uses("gl_ModelViewMatrix")) convertedSource += "uniform mat4 GLIN_ModelViewMatrix;\n";
    if((vertex || fragment) && uses("gl_ProjectionMatrix")) convertedSource += "uniform mat4 GLIN_ProjectionMatrix;\n";
    if((vertex || fragment) && uses("gl_NormalMatrix")) convertedSource += "uniform mat3 GLIN_NormalMatrix;\n";
    std::string qualifier = vertex ? "out " : "in ";
    if((vertex || fragment) && (uses("gl_FrontColor") || (fragment && uses("gl_Color")))) convertedSource += qualifier + "vec4 GLIN_FrontColor;\n";
    if((vertex || fragment) && uses("gl_BackColor")) convertedSource += qualifier + "vec4 GLIN_BackColor;\n";
    if((vertex || fragment) && uses("gl_TexCoord")) convertedSource += qualifier + "vec4 GLIN_TexCoord[8];\n";
    if(vertex && uses("ftransform")) convertedSource += "vec4 ftransform() { return GLIN_ModelViewProjectionMatrix * GLIN_Vertex; }\n";
    if(fragment && (uses("gl_FragColor") || uses("gl_FragData")))
    {
        int outputs = 1;
        std::regex output("gl_FragData\\s*\\[\\s*([0-9]+)\\s*\\]");
        for(std::sregex_iterator it(body.begin(), body.end(), output), end; it != end; ++it)
        {
            unsigned long index = std::strtoul((*it)[1].str().c_str(), nullptr, 10);
            if(index < 32) outputs = std::max(outputs, (int)index + 1);
        }
        convertedSource += "layout(location=0) out vec4 GLIN_FragData[" + std::to_string(outputs) + "];\n";
    }
    convertedSource += "#line 1\n" + rewrite.Run();
    return convertedSource.c_str();
}

void WRAP(glShaderSource(GLuint shader, GLsizei count, const GLchar* const* strings, const GLint* lengths))
{
    if(count < 0) { SetError(GL_INVALID_VALUE); return; }
    if(!glIsShader(shader)) { SetError(GL_INVALID_VALUE); return; }
    if(count && !strings) { SetError(GL_INVALID_VALUE); return; }
    std::string source;
    for(GLsizei i = 0; i < count; ++i)
    {
        if(!strings[i]) { SetError(GL_INVALID_VALUE); return; }
        source.append(strings[i], lengths && lengths[i] >= 0 ? (size_t)lengths[i] : strlen(strings[i]));
    }
    glShaderSource(shader, count, strings, lengths);
    auto& desc = globals->shaders[shader];
    if(!desc) { desc = new shader_desc_t; desc->shader = shader; }
    desc->source = std::move(source);
}

void WRAP(glGetShaderSource(GLuint shader, GLsizei size, GLsizei* length, GLchar* output))
{
    auto it = globals->shaders.find(shader);
    if(it == globals->shaders.end() || !it->second) { glGetShaderSource(shader, size, length, output); return; }
    if(size < 0) { SetError(GL_INVALID_VALUE); return; }
    const auto& source = it->second->source;
    GLsizei written = size > 0 ? (GLsizei)std::min(source.size(), (size_t)size - 1) : 0;
    if(size > 0 && output) { memcpy(output, source.data(), written); output[written] = 0; }
    if(length) *length = written;
}

void WRAP(glCompileShader(GLuint shader))
{
    GLint type = 0, size = 0;
    glGetShaderiv(shader, GL_SHADER_TYPE, &type);
    glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &size);
    if(size <= 0) { glCompileShader(shader); return; }
    std::vector<char> source((size_t)size);
    glGetShaderSource(shader, size, nullptr, source.data());
    auto tracked = globals->shaders.find(shader);
    if(tracked != globals->shaders.end() && tracked->second && !tracked->second->source.empty())
    {
        source.assign(tracked->second->source.begin(), tracked->second->source.end());
        source.push_back(0);
    }
    std::string original(source.data());
    bool native = std::regex_search(original, std::regex("#[ \\t]*version[ \\t]+[0-9]+[ \\t]+es"));
    if(!native)
    {
        const char* converted = ConvertShader(source.data(), type);
        glShaderSource(shader, 1, &converted, nullptr);
    }
    glCompileShader(shader);
}

GLuint WRAP(glCreateShader(GLenum type))
{
    GLuint shader = glCreateShader(type);
    if(shader)
    {
        auto& desc = globals->shaders[shader];
        if(!desc) desc = new shader_desc_t;
        desc->shader = shader;
        desc->vertexShader = type == GL_VERTEX_SHADER;
    }
    return shader;
}

void WRAP(glDeleteShader(GLuint shader))
{
    glDeleteShader(shader);
    auto it = globals->shaders.find(shader);
    if(it != globals->shaders.end()) { delete it->second; globals->shaders.erase(it); }
}

void WRAP(glLinkProgram(GLuint program))
{
    glLinkProgram(program);
    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if(!linked) return;
    GLint count = 0;
    glGetProgramiv(program, GL_ATTACHED_SHADERS, &count);
    std::vector<GLuint> shaders(count);
    glGetAttachedShaders(program, count, nullptr, shaders.data());
    bool geometry = false;
    for(GLuint shader : shaders)
    {
        GLint type;
        glGetShaderiv(shader, GL_SHADER_TYPE, &type);
        geometry |= type == GL_GEOMETRY_SHADER;
    }
    globals->objects->geometryPrograms[program] = geometry;
}

void WRAP(glProgramBinary(GLuint program, GLenum format, const void* binary, GLsizei length))
{
    globals->objects->geometryPrograms.erase(program);
    glProgramBinary(program, format, binary, length);
}

void WRAP(glBindFragDataLocationIndexed(GLuint program, GLuint color, GLuint index, const GLchar* name))
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(!glIsProgram(program)) { SetError(glIsShader(program) ? GL_INVALID_OPERATION : GL_INVALID_VALUE); return; }
    if(index > 1) { SetError(GL_INVALID_VALUE); return; }
    if(!name) { SetError(GL_INVALID_VALUE); return; }
    if(strncmp(name, "gl_", 3) == 0) { SetError(GL_INVALID_OPERATION); return; }
    bool extended = GLIN_HasExtension("GL_EXT_blend_func_extended");
    GLint limit = 1;
    if(!index || extended) glGetIntegerv(index ? 0x88FC : GL_MAX_DRAW_BUFFERS, &limit);
    if(color >= (GLuint)limit) { SetError(GL_INVALID_VALUE); return; }
    auto native = extended ? (void(*)(GLuint, GLuint, GLuint, const GLchar*))GLIN_GetBackendProc("glBindFragDataLocationIndexedEXT") : nullptr;
    if(!native) { SetError(GL_INVALID_OPERATION); return; }
    native(program, color, index, name);
}

void WRAP(glBindFragDataLocation(GLuint program, GLuint color, const GLchar* name))
{
    WRAP(glBindFragDataLocationIndexed(program, color, 0, name));
}

GLint WRAP(glGetFragDataIndex(GLuint program, const GLchar* name))
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return -1; }
    if(!glIsProgram(program)) { SetError(glIsShader(program) ? GL_INVALID_OPERATION : GL_INVALID_VALUE); return -1; }
    GLint linked;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if(!linked) { SetError(GL_INVALID_OPERATION); return -1; }
    auto native = GLIN_HasExtension("GL_EXT_blend_func_extended") ? (GLint(*)(GLuint, const GLchar*))GLIN_GetBackendProc("glGetFragDataIndexEXT") : nullptr;
    if(native) return native(program, name);
    return glGetFragDataLocation(program, name) < 0 ? -1 : 0;
}

char* ConvertARBShader(const char* source, bool vertex)
{
    const char* header = vertex ? "!!ARBvp1.0" : "!!ARBfp1.0";
    const char* marker = source ? strstr(source, vertex ? "//GLSLvp" : "//GLSLfp") : nullptr;
    if(!source || strncmp(source, header, 10) || !marker)
    {
        globals->arb.errorPtr = 0;
        delete[] globals->arb.errorStr;
        const char* message = "ARB assembly is unsupported; supply a GLSL payload";
        globals->arb.errorStr = new char[strlen(message) + 1];
        strcpy(globals->arb.errorStr, message);
        SetError(GL_INVALID_OPERATION);
        return nullptr;
    }
    globals->arb.errorPtr = -1;
    char* result = new char[strlen(marker) + 1];
    strcpy(result, marker);
    return result;
}

void WRAP(glGetActiveUniformName(GLuint program, GLuint index, GLsizei size, GLsizei* length, char* name))
{
    GLint components;
    GLenum type;
    glGetActiveUniform(program, index, size, length, &components, &type, name);
}

void WRAP(glUseProgram(GLuint program))
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(program)
    {
        if(!glIsProgram(program)) { SetError(glIsShader(program) ? GL_INVALID_OPERATION : GL_INVALID_VALUE); return; }
        GLint linked = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &linked);
        if(!linked) { SetError(GL_INVALID_OPERATION); return; }
    }
    GLuint previous = globals->gl.activeProgram;
    glUseProgram(program);
    globals->gl.activeProgram = program;
    if(previous && previous != program && !glIsProgram(previous)) globals->objects->geometryPrograms.erase(previous);
}

void WRAP(glDeleteProgram(GLuint program))
{
    glDeleteProgram(program);
    if(!glIsProgram(program)) globals->objects->geometryPrograms.erase(program);
}
