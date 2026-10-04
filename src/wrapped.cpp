#include "GLES.h"
#include "wrapped.h"
#include <array>
#include <algorithm>
#include <cmath>
#include "glhelper.h"
#include "gl_render.h"
#include "gl_shader.h"
#include "gl_texture.h"

GLuint WRAP(glGenLists(GLsizei range))
{
    if(range < 0) { SetError(GL_INVALID_VALUE); return 0; }
    if(!range) return 0;
    uint64_t start = 1;
    while(start + (uint64_t)range <= (uint64_t)UINT32_MAX + 1)
    {
        bool available = true;
        for(uint64_t i = start; i < start + (uint64_t)range; ++i)
        {
            if(globals->lists.count((GLuint)i)) { start = i + 1; available = false; break; }
        }
        if(!available) continue;
        for(uint64_t i = start; i < start + (uint64_t)range; ++i)
        {
            auto* list = new display_list_t;
            list->id = (GLuint)i;
            globals->lists[(GLuint)i] = list;
        }
        return (GLuint)start;
    }
    SetError(GL_OUT_OF_MEMORY);
    return 0;
}

void WRAP(glNewList(GLuint list, GLenum mode))
{
    if(list == 0) { SetError(GL_INVALID_VALUE); return; }
    if(globals->currentList || globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(mode != GL_COMPILE && mode != 0x1301) { SetError(GL_INVALID_ENUM); return; }
    if(globals->lists.find(list) == globals->lists.end())
    {
        display_list_t* dl = new display_list_t;
        dl->id = list;
        globals->lists[list] = dl;
    }
    display_list_t* dl = globals->lists[list];
    dl->compiled = false;
    dl->commands.clear();
    globals->currentList = list;
    globals->currentListMode = mode;
}

void WRAP(glEndList())
{
    if(globals->currentList == 0) return;
    auto it = globals->lists.find(globals->currentList);
    if(it != globals->lists.end()) it->second->compiled = true;
    globals->currentList = 0;
    globals->currentListMode = 0;
}

void WRAP(glCallList(GLuint list))
{
    if(globals->currentList != 0)
    {
        auto _it = globals->lists.find(globals->currentList);
        if(_it != globals->lists.end() && !_it->second->compiled)
        {
            _it->second->commands.push_back([=](){ WRAP(glCallList(list)); });
        }
        if(globals->currentListMode == GL_COMPILE) return;
    }
    auto it = globals->lists.find(list);
    if(it == globals->lists.end()) return;
    display_list_t* dl = it->second;
    if(!dl->compiled) return;
    if(globals->listDepth >= 64) return;
    GLuint compiling = globals->currentList;
    globals->currentList = 0;
    ++globals->listDepth;
    for(auto& cmd : dl->commands) cmd();
    --globals->listDepth;
    globals->currentList = compiling;
}

void WRAP(glCallLists(GLsizei n, GLenum type, const GLvoid* lists))
{
    if(n < 0) { SetError(GL_INVALID_VALUE); return; }
    if(n && !lists) { SetError(GL_INVALID_VALUE); return; }
    if(globals->currentList != 0)
    {
        std::vector<GLuint> ids(n);
        for(GLsizei i = 0; i < n; ++i)
        {
            switch(type)
            {
                case GL_UNSIGNED_BYTE:  ids[i] = ((const GLubyte*)lists)[i];  break;
                case GL_UNSIGNED_SHORT: ids[i] = ((const GLushort*)lists)[i]; break;
                case GL_UNSIGNED_INT:   ids[i] = ((const GLuint*)lists)[i];   break;
                case GL_BYTE:           ids[i] = ((const GLbyte*)lists)[i];   break;
                case GL_SHORT:          ids[i] = ((const GLshort*)lists)[i];  break;
                case GL_INT:            ids[i] = ((const GLint*)lists)[i];    break;
                case GL_FLOAT:          ids[i] = (GLuint)((const GLfloat*)lists)[i]; break;
                default: ids[i] = 0; break;
            }
        }
        auto _it = globals->lists.find(globals->currentList);
        if(_it != globals->lists.end() && !_it->second->compiled)
        {
            _it->second->commands.push_back([=](){
                for(GLuint id : ids) WRAP(glCallList(id + globals->listBase));
            });
        }
        if(globals->currentListMode == GL_COMPILE) return;
    }
    list_record_guard_t recording;
    for(GLsizei i = 0; i < n; ++i)
    {
        GLuint id = 0;
        switch(type)
        {
            case GL_UNSIGNED_BYTE:  id = ((const GLubyte*)lists)[i];  break;
            case GL_UNSIGNED_SHORT: id = ((const GLushort*)lists)[i]; break;
            case GL_UNSIGNED_INT:   id = ((const GLuint*)lists)[i];   break;
            case GL_BYTE:           id = ((const GLbyte*)lists)[i];   break;
            case GL_SHORT:          id = ((const GLshort*)lists)[i];  break;
            case GL_INT:            id = ((const GLint*)lists)[i];    break;
            case GL_FLOAT:          id = (GLuint)((const GLfloat*)lists)[i]; break;
            default: break;
        }
        WRAP(glCallList(id + globals->listBase));
    }
}

void WRAP(glDeleteLists(GLuint list, GLsizei range))
{
    if(range < 0) { SetError(GL_INVALID_VALUE); return; }
    uint64_t end = (uint64_t)list + (GLuint)range;
    for(auto it = globals->lists.begin(); it != globals->lists.end();)
    {
        if(it->first >= list && (uint64_t)it->first < end)
        {
            delete it->second;
            it = globals->lists.erase(it);
        }
        else ++it;
    }
}

GLboolean WRAP(glIsList(GLuint list))
{
    auto it = globals->lists.find(list);
    return (it != globals->lists.end() && it->second->compiled) ? GL_TRUE : GL_FALSE;
}

void WRAP(glListBase(GLuint base))
{
    DLREC(WRAP(glListBase(base)));
    globals->listBase = base;
}

void WRAP(glPushAttrib(GLbitfield mask))
{
    DLREC(WRAP(glPushAttrib(mask)));
    GLIN_InitExtensions();

    if(globals->attribStack.size() >= 16) { SetError(GL_STACK_OVERFLOW); return; }
    attrib_snapshot_t snap;
    snap.mask = mask;
    snap.framebufferSRGB = globals->gl.framebufferSRGB;

    // GL_CURRENT_BIT
    snap.color    = globals->render.color;
    snap.texcoord = globals->render.texcoord;
    memcpy(snap.multiTexcoord, globals->render.multiTexcoord, sizeof(snap.multiTexcoord));
    snap.normal   = globals->render.normal;
    snap.secondaryColor = globals->render.secondaryColor;
    snap.fogCoord = globals->render.fogCoord;

    // GL_ENABLE_BIT
    snap.lightingEnabled = globals->ff.lightingEnabled;
    snap.normalizeEnabled= globals->ff.normalizeEnabled;
    snap.fogEnabled      = globals->ff.fogEnabled;
    snap.logicOpEnabled  = globals->ff.logicOpEnabled;
    snap.alphaTestEnabled= globals->ff.alphaTestEnabled;
    snap.colorMaterial   = globals->render.colorMaterial;
    snap.texture         = globals->render.texture;
    memcpy(snap.lightEnabled, globals->ff.lightEnabled, sizeof(snap.lightEnabled));
    memcpy(snap.clipPlaneOn, globals->ff.clipPlaneOn, sizeof(snap.clipPlaneOn));

    // GL_FOG_BIT
    snap.fogMode    = globals->ff.fogMode;
    snap.fogColor   = globals->ff.fogColor;
    snap.fogDensity = globals->ff.fogDensity;
    snap.fogStart   = globals->ff.fogStart;
    snap.fogEnd     = globals->ff.fogEnd;

    // GL_LIGHTING_BIT
    snap.colorMaterialFace   = globals->ff.colorMaterialFace;
    snap.colorMaterialMode   = globals->ff.colorMaterialMode;
    snap.lightModelTwoSide   = globals->ff.lightModelTwoSide;
    snap.lightModelLocalViewer = globals->ff.lightModelLocalViewer;
    snap.shadeModel          = globals->ff.shadeModel;
    memcpy(snap.lights, globals->ff.lights, sizeof(snap.lights));
    snap.ambient             = globals->render.ambient;

    // GL_TEXTURE_BIT
    snap.texEnvMode = globals->ff.texEnvMode;

    // GL_POINT_BIT / GL_LINE_BIT
    snap.pointSize = globals->ff.pointSize;
    snap.lineWidth = globals->ff.lineWidth;

    // GL_POLYGON_BIT
    snap.lastPolygonMode = globals->gl.lastPolygonMode;

    // GL_COLOR_BUFFER_BIT
    snap.alphaTestFunc = globals->ff.alphaTestFunc;
    snap.alphaTestRef  = globals->ff.alphaTestRef;

    snap.primitiveRestart = globals->gl.primitiveRestart;
    snap.fixed = globals->ff;
    for(GLenum cap : {GL_BLEND, GL_DEPTH_TEST, GL_STENCIL_TEST, GL_CULL_FACE, GL_SCISSOR_TEST, GL_DITHER, GL_POLYGON_OFFSET_FILL, GL_SAMPLE_ALPHA_TO_COVERAGE, GL_SAMPLE_COVERAGE}) snap.nativeEnable[cap] = glIsEnabled(cap);
    const GLenum blendNames[] = {GL_BLEND_SRC_RGB, GL_BLEND_DST_RGB, GL_BLEND_SRC_ALPHA, GL_BLEND_DST_ALPHA, GL_BLEND_EQUATION_RGB, GL_BLEND_EQUATION_ALPHA};
    for(int i = 0; i < 6; ++i) glGetIntegerv(blendNames[i], &snap.blend[i]);
    glGetIntegerv(GL_DEPTH_FUNC, &snap.depthFunc);
    glGetBooleanv(GL_DEPTH_WRITEMASK, &snap.depthMask);
    glGetBooleanv(GL_COLOR_WRITEMASK, snap.colorMask);
    glGetFloatv(GL_COLOR_CLEAR_VALUE, snap.clearColor);
    glGetFloatv(GL_BLEND_COLOR, snap.blendColor);
    glGetFloatv(GL_DEPTH_CLEAR_VALUE, &snap.clearDepth);
    glGetFloatv(GL_DEPTH_RANGE, snap.depthRange);
    glGetIntegerv(GL_CULL_FACE_MODE, &snap.cullFace);
    glGetIntegerv(GL_FRONT_FACE, &snap.frontFace);
    glGetIntegerv(GL_VIEWPORT, snap.viewport);
    glGetIntegerv(GL_SCISSOR_BOX, snap.scissor);
    globals->attribStack.push_back(snap);
}

void WRAP(glPopAttrib())
{
    DLREC(WRAP(glPopAttrib()));

    if(globals->attribStack.empty()) { SetError(GL_STACK_UNDERFLOW); return; }
    attrib_snapshot_t& snap = globals->attribStack.back();
    GLbitfield mask = snap.mask;

    if(mask & GL_CURRENT_BIT)
    {
        globals->render.color    = snap.color;
        globals->render.texcoord = snap.texcoord;
        memcpy(globals->render.multiTexcoord, snap.multiTexcoord, sizeof(snap.multiTexcoord));
        globals->render.normal   = snap.normal;
        globals->render.secondaryColor = snap.secondaryColor;
        globals->render.fogCoord = snap.fogCoord;
    }
    if(mask & GL_ENABLE_BIT)
    {
        globals->ff.pointSprite = snap.fixed.pointSprite;
        globals->ff.colorSum = snap.fixed.colorSum;
        for(int unit = 0; unit < 8; ++unit) for(int c = 0; c < 4; ++c) globals->ff.texGen[unit][c].enabled = snap.fixed.texGen[unit][c].enabled;
        globals->gl.primitiveRestart = snap.primitiveRestart;
        globals->ff.lightingEnabled  = snap.lightingEnabled;
        globals->ff.normalizeEnabled = snap.normalizeEnabled;
        globals->ff.rescaleNormalEnabled = snap.fixed.rescaleNormalEnabled;
        globals->ff.fogEnabled       = snap.fogEnabled;
        globals->ff.logicOpEnabled   = snap.logicOpEnabled;
        globals->ff.alphaTestEnabled = snap.alphaTestEnabled;
        globals->render.colorMaterial= snap.colorMaterial;
        globals->render.texture      = snap.texture;
        memcpy(globals->ff.lightEnabled, snap.lightEnabled, sizeof(snap.lightEnabled));
        memcpy(globals->ff.clipPlaneOn, snap.clipPlaneOn, sizeof(snap.clipPlaneOn));
    }
    if(mask & GL_FOG_BIT)
    {
        globals->ff.fogEnabled = snap.fogEnabled;
        globals->ff.fogMode    = snap.fogMode;
        globals->ff.fogColor   = snap.fogColor;
        globals->ff.fogDensity = snap.fogDensity;
        globals->ff.fogStart   = snap.fogStart;
        globals->ff.fogEnd     = snap.fogEnd;
        globals->ff.fogSource = snap.fixed.fogSource;
        globals->ff.colorSum = snap.fixed.colorSum;
    }
    if(mask & 0x00001000)
    {
        globals->ff.normalizeEnabled = snap.fixed.normalizeEnabled;
        globals->ff.rescaleNormalEnabled = snap.fixed.rescaleNormalEnabled;
    }
    if(mask & GL_LIGHTING_BIT)
    {
        globals->ff.clampVertexColor = snap.fixed.clampVertexColor;
        globals->ff.materials[0] = snap.fixed.materials[0];
        globals->ff.materials[1] = snap.fixed.materials[1];
        globals->ff.lightingEnabled = snap.lightingEnabled;
        globals->render.colorMaterial = snap.colorMaterial;
        memcpy(globals->ff.lightEnabled, snap.lightEnabled, sizeof(snap.lightEnabled));
        globals->ff.colorMaterialFace   = snap.colorMaterialFace;
        globals->ff.colorMaterialMode   = snap.colorMaterialMode;
        globals->ff.lightModelTwoSide   = snap.lightModelTwoSide;
        globals->ff.lightModelLocalViewer = snap.lightModelLocalViewer;
        globals->ff.lightModelColorControl = snap.fixed.lightModelColorControl;
        globals->ff.shadeModel          = snap.shadeModel;
        WRAP(glProvokingVertex(snap.fixed.provokingVertex));
        memcpy(globals->ff.lights, snap.lights, sizeof(snap.lights));
        globals->render.ambient         = snap.ambient;
    }
    if(mask & GL_TEXTURE_BIT)
    {
        globals->ff.texEnvMode = snap.texEnvMode;
        memcpy(globals->ff.texGen, snap.fixed.texGen, sizeof(globals->ff.texGen));
        memcpy(globals->ff.texEnv, snap.fixed.texEnv, sizeof(snap.fixed.texEnv));
        memcpy(globals->ff.textureEnabled, snap.fixed.textureEnabled, sizeof(snap.fixed.textureEnabled));
        globals->render.texture = globals->ff.textureEnabled[0];
    }
    if(mask & GL_POINT_BIT)
    {
        globals->ff.pointSize = snap.pointSize;
        globals->ff.pointMin = snap.fixed.pointMin;
        globals->ff.pointMax = snap.fixed.pointMax;
        globals->ff.pointFadeThreshold = snap.fixed.pointFadeThreshold;
        globals->ff.pointOrigin = snap.fixed.pointOrigin;
        globals->ff.pointSprite = snap.fixed.pointSprite;
        memcpy(globals->ff.pointCoordReplace, snap.fixed.pointCoordReplace, sizeof(globals->ff.pointCoordReplace));
        memcpy(globals->ff.pointAttenuation, snap.fixed.pointAttenuation, sizeof(globals->ff.pointAttenuation));
    }
    if(mask & GL_LINE_BIT) { globals->ff.lineWidth = snap.lineWidth; glLineWidth(snap.lineWidth); }
    if(mask & GL_POLYGON_BIT) globals->gl.lastPolygonMode = snap.lastPolygonMode;
    if(mask & GL_COLOR_BUFFER_BIT)
    {
        globals->ff.clampFragmentColor = snap.fixed.clampFragmentColor;
        globals->ff.clampReadColor = snap.fixed.clampReadColor;
        globals->ff.alphaTestEnabled = snap.alphaTestEnabled;
        globals->ff.alphaTestFunc = snap.alphaTestFunc;
        globals->ff.alphaTestRef  = snap.alphaTestRef;
    }

    auto restoreEnable = [&](GLenum cap) { if(snap.nativeEnable[cap]) glEnable(cap); else glDisable(cap); };
    if((mask & (GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT)) && snap.framebufferSRGB != globals->gl.framebufferSRGB)
    {
        if(snap.framebufferSRGB) WRAP(glEnable(0x8DB9));
        else WRAP(glDisable(0x8DB9));
    }
    if(mask & GL_ENABLE_BIT)
    {
        globals->ff.clampVertexColor = snap.fixed.clampVertexColor;
        globals->ff.clampFragmentColor = snap.fixed.clampFragmentColor;
        globals->ff.clampReadColor = snap.fixed.clampReadColor;
        for(auto& state : snap.nativeEnable) restoreEnable(state.first);
        memcpy(globals->ff.textureEnabled, snap.fixed.textureEnabled, sizeof(snap.fixed.textureEnabled));
    }
    if(mask & GL_COLOR_BUFFER_BIT)
    {
        restoreEnable(GL_BLEND); restoreEnable(GL_DITHER);
        glBlendFuncSeparate(snap.blend[0], snap.blend[1], snap.blend[2], snap.blend[3]);
        glBlendEquationSeparate(snap.blend[4], snap.blend[5]);
        glBlendColor(snap.blendColor[0], snap.blendColor[1], snap.blendColor[2], snap.blendColor[3]);
        glColorMask(snap.colorMask[0], snap.colorMask[1], snap.colorMask[2], snap.colorMask[3]);
        glClearColor(snap.clearColor[0], snap.clearColor[1], snap.clearColor[2], snap.clearColor[3]);
    }
    if(mask & GL_DEPTH_BUFFER_BIT)
    {
        restoreEnable(GL_DEPTH_TEST);
        glDepthFunc(snap.depthFunc); glDepthMask(snap.depthMask); glClearDepthf(snap.clearDepth);
    }
    if(mask & GL_POLYGON_BIT) { restoreEnable(GL_CULL_FACE); glCullFace(snap.cullFace); glFrontFace(snap.frontFace); }
    if(mask & 0x00000800) { glViewport(snap.viewport[0], snap.viewport[1], snap.viewport[2], snap.viewport[3]); glDepthRangef(snap.depthRange[0], snap.depthRange[1]); }
    if(mask & 0x00080000) { restoreEnable(GL_SCISSOR_TEST); glScissor(snap.scissor[0], snap.scissor[1], snap.scissor[2], snap.scissor[3]); }
    if(mask & (GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT)) UpdateColorMaterial();
    globals->attribStack.pop_back();
}

void WRAP(glPushClientAttrib(GLbitfield mask))
{
    if(globals->clientAttribStack.size() >= 16) { SetError(GL_STACK_OVERFLOW); return; }
    client_attrib_snapshot_t snap;
    snap.client = globals->client;
    const GLenum names[] = {GL_PACK_ALIGNMENT, GL_PACK_ROW_LENGTH, GL_PACK_SKIP_ROWS, GL_PACK_SKIP_PIXELS, GL_UNPACK_ALIGNMENT, GL_UNPACK_ROW_LENGTH, GL_UNPACK_SKIP_ROWS, GL_UNPACK_SKIP_PIXELS, GL_UNPACK_IMAGE_HEIGHT, GL_UNPACK_SKIP_IMAGES};
    for(int i = 0; i < 10; ++i) glGetIntegerv(names[i], &snap.pixelStore[i]);
    snap.mask = mask;

    if(mask & GL_CLIENT_VERTEX_ARRAY_BIT)
    {
        snap.vertexArrayEnabled     = globals->client.vertexArrayEnabled;
        snap.colorArrayEnabled      = globals->client.colorArrayEnabled;
        snap.normalArrayEnabled     = globals->client.normalArrayEnabled;
        snap.clientActiveTextureUnit= globals->client.clientActiveTextureUnit;
        memcpy(snap.texCoord, globals->client.texCoord, sizeof(snap.texCoord));
        snap.vertexSize   = globals->client.vertexSize;
        snap.vertexType   = globals->client.vertexType;
        snap.vertexStride = globals->client.vertexStride;
        snap.vertexBuffer = globals->client.vertexBuffer;
        snap.vertexPtr    = globals->client.vertexPtr;
        snap.colorSize    = globals->client.colorSize;
        snap.colorType    = globals->client.colorType;
        snap.colorStride  = globals->client.colorStride;
        snap.colorBuffer  = globals->client.colorBuffer;
        snap.colorPtr     = globals->client.colorPtr;
        snap.normalType   = globals->client.normalType;
        snap.normalStride = globals->client.normalStride;
        snap.normalBuffer = globals->client.normalBuffer;
        snap.normalPtr    = globals->client.normalPtr;
    }
    if(mask & GL_CLIENT_PIXEL_STORE_BIT)
    {
        snap.boundPixelUnpackBuffer = globals->client.boundPixelUnpackBuffer;
        snap.boundPixelPackBuffer   = globals->client.boundPixelPackBuffer;
    }

    globals->clientAttribStack.push_back(snap);
}

void WRAP(glPopClientAttrib())
{
    if(globals->clientAttribStack.empty()) { SetError(GL_STACK_UNDERFLOW); return; }
    client_attrib_snapshot_t& snap = globals->clientAttribStack.back();
    GLbitfield mask = snap.mask;

    if(mask & GL_CLIENT_VERTEX_ARRAY_BIT)
    {
        globals->client.vertexArrayEnabled      = snap.vertexArrayEnabled;
        globals->client.colorArrayEnabled       = snap.colorArrayEnabled;
        globals->client.normalArrayEnabled      = snap.normalArrayEnabled;
        globals->client.clientActiveTextureUnit = snap.clientActiveTextureUnit;
        memcpy(globals->client.texCoord, snap.texCoord, sizeof(snap.texCoord));
        globals->client.vertexSize   = snap.vertexSize;
        globals->client.vertexType   = snap.vertexType;
        globals->client.vertexStride = snap.vertexStride;
        globals->client.vertexBuffer = snap.vertexBuffer;
        globals->client.vertexPtr    = snap.vertexPtr;
        globals->client.colorSize    = snap.colorSize;
        globals->client.colorType    = snap.colorType;
        globals->client.colorStride  = snap.colorStride;
        globals->client.colorBuffer  = snap.colorBuffer;
        globals->client.colorPtr     = snap.colorPtr;
        globals->client.normalType   = snap.normalType;
        globals->client.normalStride = snap.normalStride;
        globals->client.normalBuffer = snap.normalBuffer;
        globals->client.normalPtr    = snap.normalPtr;
        globals->client.secondaryColorArrayEnabled = snap.client.secondaryColorArrayEnabled;
        globals->client.secondaryColorSize = snap.client.secondaryColorSize;
        globals->client.secondaryColorType = snap.client.secondaryColorType;
        globals->client.secondaryColorStride = snap.client.secondaryColorStride;
        globals->client.secondaryColorBuffer = snap.client.secondaryColorBuffer;
        globals->client.secondaryColorPtr = snap.client.secondaryColorPtr;
        globals->client.fogCoordArrayEnabled = snap.client.fogCoordArrayEnabled;
        globals->client.fogCoordType = snap.client.fogCoordType;
        globals->client.fogCoordStride = snap.client.fogCoordStride;
        globals->client.fogCoordBuffer = snap.client.fogCoordBuffer;
        globals->client.fogCoordPtr = snap.client.fogCoordPtr;
    }
    if(mask & GL_CLIENT_PIXEL_STORE_BIT)
    {
        const GLenum names[] = {GL_PACK_ALIGNMENT, GL_PACK_ROW_LENGTH, GL_PACK_SKIP_ROWS, GL_PACK_SKIP_PIXELS, GL_UNPACK_ALIGNMENT, GL_UNPACK_ROW_LENGTH, GL_UNPACK_SKIP_ROWS, GL_UNPACK_SKIP_PIXELS, GL_UNPACK_IMAGE_HEIGHT, GL_UNPACK_SKIP_IMAGES};
        for(int i = 0; i < 10; ++i) glPixelStorei(names[i], snap.pixelStore[i]);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, snap.boundPixelUnpackBuffer);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, snap.boundPixelPackBuffer);
        globals->client.boundPixelUnpackBuffer = snap.boundPixelUnpackBuffer;
        globals->client.boundPixelPackBuffer   = snap.boundPixelPackBuffer;
    }

    globals->clientAttribStack.pop_back();
}

void WRAP(glEnableIndexed(GLenum target, GLuint index))
{
    glEnablei(target, index);
}
void WRAP(glDisableIndexed(GLenum target, GLuint index))
{
    glDisablei(target, index);
}
void WRAP(glGetBooleanIndexedv(GLenum target, GLuint index, GLboolean *data))
{
    glGetBooleani_v(target, index, data);
}

// -----------------------------------------------------------------------
extern GLint GLIN_ExtensionCount();

static bool LegacyArrayState(GLenum pname, GLint64* value)
{
    const auto& client = globals->client;
    const auto& texture = client.texCoord[client.clientActiveTextureUnit];
    switch(pname)
    {
        case GL_VERTEX_ARRAY: *value = client.vertexArrayEnabled; break;
        case GL_NORMAL_ARRAY: *value = client.normalArrayEnabled; break;
        case GL_COLOR_ARRAY: *value = client.colorArrayEnabled; break;
        case GL_TEXTURE_COORD_ARRAY: *value = texture.enabled; break;
        case 0x807A: *value = client.vertexSize; break;
        case 0x807B: *value = client.vertexType; break;
        case 0x807C: *value = client.vertexStride; break;
        case 0x807E: *value = client.normalType; break;
        case 0x807F: *value = client.normalStride; break;
        case 0x8081: *value = client.colorSize; break;
        case 0x8082: *value = client.colorType; break;
        case 0x8083: *value = client.colorStride; break;
        case 0x8088: *value = texture.texCoordSize; break;
        case 0x8089: *value = texture.texCoordType; break;
        case 0x808A: *value = texture.texCoordStride; break;
        case 0x84E1: *value = GL_TEXTURE0 + client.clientActiveTextureUnit; break;
        case 0x8454: *value = client.fogCoordType; break;
        case 0x8455: *value = client.fogCoordStride; break;
        case 0x8457: *value = client.fogCoordArrayEnabled; break;
        case 0x845A: *value = client.secondaryColorSize; break;
        case 0x845B: *value = client.secondaryColorType; break;
        case 0x845C: *value = client.secondaryColorStride; break;
        case 0x845E: *value = client.secondaryColorArrayEnabled; break;
        case 0x8896: *value = client.vertexBuffer; break;
        case 0x8897: *value = client.normalBuffer; break;
        case 0x8898: *value = client.colorBuffer; break;
        case 0x889A: *value = texture.texCoordBuffer; break;
        case 0x889C: *value = client.secondaryColorBuffer; break;
        case 0x889D: *value = client.fogCoordBuffer; break;
        default: return false;
    }
    return true;
}

static int LegacyState(GLenum pname, GLdouble* values)
{
    if(pname == GL_MAJOR_VERSION || pname == GL_MINOR_VERSION) { *values = 3; return 1; }
    if(pname == GL_MAJOR_VERSION || pname == GL_MINOR_VERSION) { *values = 3; return 1; }
    if(pname == 0x9126) { *values = globals->contextProfile; return 1; }
    if(pname == 0x821E) { *values = globals->contextFlags; return 1; }
    GLint64 value;
    if(LegacyArrayState(pname, &value)) { *values = (GLdouble)value; return 1; }
    switch(pname)
    {
        case GL_COLOR_CLEAR_VALUE:
        {
            GLfloat color[4];
            glGetFloatv(pname, color);
            bool clamp = FragmentColorClamped();
            for(int i = 0; i < 4; ++i) values[i] = clamp ? std::min(std::max(color[i], 0.0f), 1.0f) : color[i];
            return 4;
        }
        case 0x0C60: case 0x0C61: case 0x0C62: case 0x0C63:
            if(globals->ff.activeTextureUnit >= 8) { SetError(GL_INVALID_OPERATION); return -1; }
            *values = globals->ff.texGen[globals->ff.activeTextureUnit][pname-0x0C60].enabled; return 1;
        case 0x8DB9: GLIN_InitExtensions(); *values = globals->gl.framebufferSRGB; return 1;
        case 0x0B11: *values = globals->ff.pointSize; return 1;
        case 0x8126: *values = globals->ff.pointMin; return 1;
        case 0x8127: GLIN_InitExtensions(); *values = globals->ff.pointMax; return 1;
        case 0x8128: *values = globals->ff.pointFadeThreshold; return 1;
        case 0x8129: for(int i = 0; i < 3; ++i) values[i] = globals->ff.pointAttenuation[i]; return 3;
        case 0x8CA0: *values = globals->ff.pointOrigin; return 1;
        case 0x8861: *values = globals->ff.pointSprite; return 1;
        case 0x803A: *values = globals->ff.rescaleNormalEnabled; return 1;
        case 0x81F8: *values = globals->ff.lightModelColorControl; return 1;
        case 0x0B51: *values = globals->ff.lightModelLocalViewer; return 1;
        case 0x0B52: *values = globals->ff.lightModelTwoSide; return 1;
        case 0x0B55: *values = globals->ff.colorMaterialFace; return 1;
        case 0x0B56: *values = globals->ff.colorMaterialMode; return 1;
        case 0x8E4F: *values = globals->ff.provokingVertex; return 1;
        case 0x8E4C: *values = GL_TRUE; return 1;
        case 0x891A: *values = globals->ff.clampVertexColor; return 1;
        case 0x891B: *values = globals->ff.clampFragmentColor; return 1;
        case 0x891C: *values = globals->ff.clampReadColor; return 1;
        case 0x8450: *values = globals->ff.fogSource; return 1;
        case 0x8453: *values = globals->render.fogCoord; return 1;
        case 0x8458: *values = globals->ff.colorSum; return 1;
        case 0x8459: values[0] = globals->render.secondaryColor.x; values[1] = globals->render.secondaryColor.y; values[2] = globals->render.secondaryColor.z; values[3] = 1; return 4;
        default: return 0;
    }
}

void WRAP(glGetPointerv(GLenum pname, void** params))
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    const auto& client = globals->client;
    switch(pname)
    {
        case 0x808E: *params = (void*)client.vertexPtr; return;
        case 0x808F: *params = (void*)client.normalPtr; return;
        case 0x8090: *params = (void*)client.colorPtr; return;
        case 0x8092: *params = (void*)client.texCoord[client.clientActiveTextureUnit].texCoordPtr; return;
        case 0x8456: *params = (void*)client.fogCoordPtr; return;
        case 0x845D: *params = (void*)client.secondaryColorPtr; return;
        default: glGetPointerv(pname, params); return;
    }
}

void WRAP(glGetIntegerv(GLenum pname, GLint* params))
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    GLdouble values[4];
    int count = LegacyState(pname, values);
    if(count)
    {
        for(int i = 0; i < count; ++i)
        {
            GLdouble value = pname == 0x8459 || pname == GL_COLOR_CLEAR_VALUE ? std::min(std::max(values[i], -1.0), 1.0) * (values[i] < 0 ? 2147483648.0 : 2147483647.0) : std::round(values[i]);
            params[i] = (GLint)std::min(std::max(value, -2147483648.0), 2147483647.0);
        }
        return;
    }
    switch(pname)
    {
        case 0x8F9E: *params = (GLint)globals->gl.restartIndex; return;
        case 0x8F9D: *params = globals->gl.primitiveRestart; return;
        case GL_NUM_EXTENSIONS: *params = GLIN_ExtensionCount(); return;
        case GL_MAJOR_VERSION: *params = 3; return;
        case GL_MINOR_VERSION: *params = 3; return;
        case 0x0C01: //GL_DRAW_BUFFER:
            *params = (globals->gl.activeDrawBuffer != 0) ? GL_COLOR_ATTACHMENT0 : GL_BACK;
            break;

        case 0x84EF: //GL_TEXTURE_COMPRESSION_HINT:
            *params = GL_DONT_CARE;
            break;

        case 0x862F: //GL_MAX_PROGRAM_MATRICES_ARB:
            *params = MAX_ARB_MATRIX;
            break;

        case 0x864B: //GL_PROGRAM_ERROR_POSITION_ARB:
            *params = globals->arb.errorPtr;
            break;

        case 0x8B8D: //GL_CURRENT_PROGRAM:
            *params = (GLint)globals->gl.activeProgram;
            break;

        case 0x0BA0: //GL_MATRIX_MODE:
            *params = (GLint)globals->matrix.mode;
            break;

        case 0x84E0: //GL_ACTIVE_TEXTURE:
            *params = (GLint)globals->gl.activeTexUnit;
            break;

        case 0x0D31: //GL_MAX_LIGHTS:
            *params = 8;
            break;

        case 0x84E2: //GL_MAX_TEXTURE_UNITS:
            *params = 8;
            break;

        // Alpha test
        case 0x0BC1: /*GL_ALPHA_TEST_FUNC*/ *params=(GLint)globals->ff.alphaTestFunc; return;

        // Attrib stack depths
        case 0x0BB0: /*GL_ATTRIB_STACK_DEPTH*/       *params=(GLint)globals->attribStack.size();       return;
        case 0x0BB1: /*GL_CLIENT_ATTRIB_STACK_DEPTH*/*params=(GLint)globals->clientAttribStack.size(); return;
        case 0x0D35: /*GL_MAX_ATTRIB_STACK_DEPTH*/
        case 0x0D3B: /*GL_MAX_CLIENT_ATTRIB_STACK_DEPTH*/ *params = 16;                                return;

        // List state
        case 0x0B32: /*GL_LIST_BASE*/   *params=(GLint)globals->listBase;       return;
        case 0x0B33: /*GL_LIST_INDEX*/  *params=(GLint)globals->currentList;    return;
        case 0x0B30: /*GL_LIST_MODE*/   *params=(GLint)globals->currentListMode;return;

        // Polygon / line
        case 0x0B25: /*GL_LINE_STIPPLE_PATTERN*/ *params=(GLint)globals->ff.lineStipplePattern; return;
        case 0x0B26: /*GL_LINE_STIPPLE_REPEAT*/  *params=(GLint)globals->ff.lineStippleFactor;  return;

        default:
            glGetIntegerv(pname, params);
            break;
    }
}

void WRAP(glGetInteger64v(GLenum pname, GLint64* params))
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(pname == GL_MAJOR_VERSION || pname == GL_MINOR_VERSION || pname == 0x9126 || pname == 0x821E)
    {
        GLdouble value;
        LegacyState(pname, &value);
        *params = (GLint64)value;
        return;
    }
    if(LegacyArrayState(pname, params)) return;
    if(pname == GL_COLOR_CLEAR_VALUE)
    {
        GLdouble values[4];
        LegacyState(pname, values);
        for(int i = 0; i < 4; ++i)
        {
            GLdouble value = values[i] * (values[i] < 0 ? 9223372036854775808.0 : 9223372036854775807.0);
            params[i] = value >= (GLdouble)INT64_MAX ? INT64_MAX : value <= (GLdouble)INT64_MIN ? INT64_MIN : std::isnan(value) ? 0 : (GLint64)std::round(value);
        }
        return;
    }
    if(pname == 0x0B11 || (pname >= 0x8126 && pname <= 0x8129) || (pname >= 0x0C60 && pname <= 0x0C63) || (pname >= 0x891A && pname <= 0x891C) || pname == 0x8CA0 || pname == 0x8861 || pname == 0x803A || pname == 0x81F8 || pname == 0x0B51 || pname == 0x0B52 || pname == 0x0B55 || pname == 0x0B56 || pname == 0x8E4F || pname == 0x8E4C)
    {
        GLdouble values[4];
        int count = LegacyState(pname, values);
        for(int i = 0; i < count; ++i)
        {
            GLdouble value = std::round(values[i]);
            params[i] = value >= (GLdouble)INT64_MAX ? INT64_MAX : value <= (GLdouble)INT64_MIN ? INT64_MIN : (GLint64)value;
        }
        return;
    }
    if(pname == 0x8DB9)
    {
        GLIN_InitExtensions();
        *params = globals->gl.framebufferSRGB;
        return;
    }
    glGetInteger64v(pname, params);
}

void WRAP(glEnable(GLenum cap))
{
    DLREC(WRAP(glEnable(cap)));
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(cap == 0x803A)
    {
        if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
        globals->ff.rescaleNormalEnabled = true;
        return;
    }
    if(cap >= 0x0C60 && cap <= 0x0C63 && (globals->render.begin || globals->ff.activeTextureUnit >= 8)) { SetError(GL_INVALID_OPERATION); return; }
    if(cap == 0x8DB9)
    {
        if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
        GLIN_InitExtensions();
        if(GLIN_HasExtension("GL_EXT_sRGB_write_control")) glEnable(cap);
        globals->gl.framebufferSRGB = true;
        return;
    }
    if(cap == 0x8458) { globals->ff.colorSum = true; return; }

    switch(cap)
    {
        case 0x8F9D: globals->gl.primitiveRestart = true; return;
        case 0x8620: //GL_VERTEX_PROGRAM_ARB:
            globals->gl.enabledVertProgARB = true;
            break;
            
        case 0x8804: //GL_FRAGMENT_PROGRAM_ARB:
            globals->gl.enabledFragProgARB = true;
            break;
            
        case GL_LIGHTING:
            globals->ff.lightingEnabled = true;
            break;
            
        case GL_NORMALIZE:
            globals->ff.normalizeEnabled = true;
            break;
            
        case GL_FOG:
            globals->ff.fogEnabled = true;
            break;
            
        case 0x0BF1: //GL_LOGIC_OP
            globals->ff.logicOpEnabled = true;
            break;

        case GL_ALPHA_TEST:
            globals->ff.alphaTestEnabled = true;
            break;
            
        case GL_CLIP_PLANE0: case GL_CLIP_PLANE1: case GL_CLIP_PLANE2:
        case GL_CLIP_PLANE3: case GL_CLIP_PLANE4: case GL_CLIP_PLANE5:
            globals->ff.clipPlaneOn[cap - GL_CLIP_PLANE0] = true;
            break;
            
        case GL_LIGHT0: case GL_LIGHT1: case GL_LIGHT2: case GL_LIGHT3:
        case GL_LIGHT4: case GL_LIGHT5: case GL_LIGHT6: case GL_LIGHT7:
            globals->ff.lightEnabled[cap - GL_LIGHT0] = true;
            break;
            
        case GL_TEXTURE_2D:
            if(globals->ff.activeTextureUnit >= 8) { SetError(GL_INVALID_OPERATION); return; }
            globals->ff.textureEnabled[globals->ff.activeTextureUnit] = true;
            globals->render.texture = globals->ff.textureEnabled[0];
            break;
            
        case GL_COLOR_MATERIAL:
            if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
            globals->render.colorMaterial = true;
            UpdateColorMaterial();
            break;

        case 0x0DE0: //GL_TEXTURE_1D — treat as 2D
            if(globals->ff.activeTextureUnit >= 8) { SetError(GL_INVALID_OPERATION); return; }
            globals->ff.textureEnabled[globals->ff.activeTextureUnit] = true;
            globals->render.texture = globals->ff.textureEnabled[0];
            break;

        case 0x0C60: //GL_TEXTURE_GEN_S
            globals->ff.texGen[globals->ff.activeTextureUnit][0].enabled = true; break;
        case 0x0C61: //GL_TEXTURE_GEN_T
            globals->ff.texGen[globals->ff.activeTextureUnit][1].enabled = true; break;
        case 0x0C62: //GL_TEXTURE_GEN_R
            globals->ff.texGen[globals->ff.activeTextureUnit][2].enabled = true; break;
        case 0x0C63: //GL_TEXTURE_GEN_Q
            globals->ff.texGen[globals->ff.activeTextureUnit][3].enabled = true; break;

        case 0x0B24: //GL_LINE_STIPPLE
            globals->ff.lineStippleEnabled = true; break;
        case 0x0B42: //GL_POLYGON_STIPPLE
            globals->ff.polygonStippleEnabled = true; break;

        case 0x8861: //GL_POINT_SPRITE
            if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
            globals->ff.pointSprite = true; break;

        default:
            glEnable(cap);
            break;
    }
}

void WRAP(glDisable(GLenum cap))
{
    DLREC(WRAP(glDisable(cap)));
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(cap == 0x803A)
    {
        if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
        globals->ff.rescaleNormalEnabled = false;
        return;
    }
    if(cap >= 0x0C60 && cap <= 0x0C63 && (globals->render.begin || globals->ff.activeTextureUnit >= 8)) { SetError(GL_INVALID_OPERATION); return; }
    if(cap == 0x8DB9)
    {
        if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
        GLIN_InitExtensions();
        if(!GLIN_HasExtension("GL_EXT_sRGB_write_control")) { SetError(GL_INVALID_OPERATION); return; }
        glDisable(cap);
        globals->gl.framebufferSRGB = false;
        return;
    }
    if(cap == 0x8458) { globals->ff.colorSum = false; return; }

    switch(cap)
    {
        case 0x8F9D: globals->gl.primitiveRestart = false; return;
        case 0x8620: //GL_VERTEX_PROGRAM_ARB:
            globals->gl.enabledVertProgARB = false;
            break;
            
        case 0x8804: //GL_FRAGMENT_PROGRAM_ARB:
            globals->gl.enabledFragProgARB = false;
            break;
            
        case GL_LIGHTING:
            globals->ff.lightingEnabled = false;
            break;
            
        case GL_NORMALIZE:
            globals->ff.normalizeEnabled = false;
            break;
            
        case GL_FOG:
            globals->ff.fogEnabled = false;
            break;
            
        case 0x0BF1: //GL_LOGIC_OP
            globals->ff.logicOpEnabled = false;
            break;

        case GL_ALPHA_TEST:
            globals->ff.alphaTestEnabled = false;
            break;
            
        case GL_CLIP_PLANE0: case GL_CLIP_PLANE1: case GL_CLIP_PLANE2:
        case GL_CLIP_PLANE3: case GL_CLIP_PLANE4: case GL_CLIP_PLANE5:
            globals->ff.clipPlaneOn[cap - GL_CLIP_PLANE0] = false;
            break;
            
        case GL_LIGHT0: case GL_LIGHT1: case GL_LIGHT2: case GL_LIGHT3:
        case GL_LIGHT4: case GL_LIGHT5: case GL_LIGHT6: case GL_LIGHT7:
            globals->ff.lightEnabled[cap - GL_LIGHT0] = false;
            break;
            
        case GL_TEXTURE_2D:
            if(globals->ff.activeTextureUnit >= 8) { SetError(GL_INVALID_OPERATION); return; }
            globals->ff.textureEnabled[globals->ff.activeTextureUnit] = false;
            globals->render.texture = globals->ff.textureEnabled[0];
            break;
            
        case GL_COLOR_MATERIAL:
            if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
            globals->render.colorMaterial = false;
            break;

        case 0x0DE0: //GL_TEXTURE_1D
            if(globals->ff.activeTextureUnit >= 8) { SetError(GL_INVALID_OPERATION); return; }
            globals->ff.textureEnabled[globals->ff.activeTextureUnit] = false;
            globals->render.texture = globals->ff.textureEnabled[0];
            break;

        case 0x0C60: //GL_TEXTURE_GEN_S
            globals->ff.texGen[globals->ff.activeTextureUnit][0].enabled = false; break;
        case 0x0C61: //GL_TEXTURE_GEN_T
            globals->ff.texGen[globals->ff.activeTextureUnit][1].enabled = false; break;
        case 0x0C62: //GL_TEXTURE_GEN_R
            globals->ff.texGen[globals->ff.activeTextureUnit][2].enabled = false; break;
        case 0x0C63: //GL_TEXTURE_GEN_Q
            globals->ff.texGen[globals->ff.activeTextureUnit][3].enabled = false; break;

        case 0x0B24: //GL_LINE_STIPPLE
            globals->ff.lineStippleEnabled = false; break;
        case 0x0B42: //GL_POLYGON_STIPPLE
            globals->ff.polygonStippleEnabled = false; break;

        case 0x8861: //GL_POINT_SPRITE
            if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
            globals->ff.pointSprite = false; break;

        default:
            glDisable(cap);
            break;
    }
}

GLboolean WRAP(glIsEnabled(GLenum cap))
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return GL_FALSE; }
    if(cap >= 0x0C60 && cap <= 0x0C63 && globals->ff.activeTextureUnit >= 8) { SetError(GL_INVALID_OPERATION); return GL_FALSE; }
    if(cap == GL_VERTEX_ARRAY || cap == GL_NORMAL_ARRAY || cap == GL_COLOR_ARRAY || cap == GL_TEXTURE_COORD_ARRAY || cap == 0x8457 || cap == 0x845E)
    {
        GLint64 value;
        LegacyArrayState(cap, &value);
        return value != 0;
    }
    if(cap == 0x8DB9)
    {
        GLIN_InitExtensions();
        return globals->gl.framebufferSRGB;
    }
    if(cap == 0x8458) return globals->ff.colorSum;
    if(cap == 0x8861) return globals->ff.pointSprite;
    if(cap == 0x803A) return globals->ff.rescaleNormalEnabled;
    switch(cap)
    {
        case 0x8F9D: return globals->gl.primitiveRestart;
        case 0x8620: //GL_VERTEX_PROGRAM_ARB:
            return globals->gl.enabledVertProgARB;
            
        case 0x8804: //GL_FRAGMENT_PROGRAM_ARB:
            return globals->gl.enabledFragProgARB;
            
        case GL_LIGHTING:
            return globals->ff.lightingEnabled;
            
        case GL_NORMALIZE:
            return globals->ff.normalizeEnabled;
            
        case GL_FOG:
            return globals->ff.fogEnabled;
            
        case 0x0BF1: //GL_LOGIC_OP
            return globals->ff.logicOpEnabled;

        case GL_ALPHA_TEST:
            return globals->ff.alphaTestEnabled;
            
        case GL_CLIP_PLANE0: case GL_CLIP_PLANE1: case GL_CLIP_PLANE2:
        case GL_CLIP_PLANE3: case GL_CLIP_PLANE4: case GL_CLIP_PLANE5:
            return globals->ff.clipPlaneOn[cap - GL_CLIP_PLANE0];
            
        case GL_LIGHT0: case GL_LIGHT1: case GL_LIGHT2: case GL_LIGHT3:
        case GL_LIGHT4: case GL_LIGHT5: case GL_LIGHT6: case GL_LIGHT7:
            return globals->ff.lightEnabled[cap - GL_LIGHT0];
            
        case GL_TEXTURE_2D:
            return globals->ff.activeTextureUnit < 8 ? globals->ff.textureEnabled[globals->ff.activeTextureUnit] : GL_FALSE;

        case GL_COLOR_MATERIAL:
            return globals->render.colorMaterial;

        case 0x0C60: //GL_TEXTURE_GEN_S
            return globals->ff.texGen[globals->ff.activeTextureUnit][0].enabled;
        case 0x0C61: //GL_TEXTURE_GEN_T
            return globals->ff.texGen[globals->ff.activeTextureUnit][1].enabled;
        case 0x0C62: //GL_TEXTURE_GEN_R
            return globals->ff.texGen[globals->ff.activeTextureUnit][2].enabled;
        case 0x0C63: //GL_TEXTURE_GEN_Q
            return globals->ff.texGen[globals->ff.activeTextureUnit][3].enabled;

        case 0x0B24: //GL_LINE_STIPPLE
            return globals->ff.lineStippleEnabled;
        case 0x0B42: //GL_POLYGON_STIPPLE
            return globals->ff.polygonStippleEnabled;

        default:
            return glIsEnabled(cap);
    }
}

void WRAP(glGetFloatv(GLenum pname, GLfloat* data))
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    GLdouble values[4];
    int count = LegacyState(pname, values);
    if(count) { for(int i = 0; i < count; ++i) data[i] = (GLfloat)values[i]; return; }
    if(pname == 0x8F9D || pname == 0x8F9E)
    {
        data[0] = (GLfloat)(pname == 0x8F9D ? globals->gl.primitiveRestart : globals->gl.restartIndex);
        return;
    }

    switch(pname)
    {
        default:
            glGetFloatv(pname, data);
            break;
        
        case GL_MODELVIEW_MATRIX:
            memcpy(data, globals->matrix.modelview.Current().data(), 16 * sizeof(GLfloat));
            break;
        
        case GL_PROJECTION_MATRIX:
            memcpy(data, globals->matrix.projection.Current().data(), 16 * sizeof(GLfloat));
            break;
        
        case GL_TEXTURE_MATRIX:
            memcpy(data, globals->matrix.Texture().Current().data(), 16 * sizeof(GLfloat));
            break;

        case 0x0B00: //GL_CURRENT_COLOR:
            data[0] = globals->render.color.x;
            data[1] = globals->render.color.y;
            data[2] = globals->render.color.z;
            data[3] = globals->render.color.w;
            break;

        case 0x0B01: //GL_CURRENT_INDEX:
            data[0] = 1.0f;
            break;

        case 0x0B02: //GL_CURRENT_NORMAL:
            data[0] = globals->render.normal.x;
            data[1] = globals->render.normal.y;
            data[2] = globals->render.normal.z;
            break;

        case 0x0B03: //GL_CURRENT_TEXTURE_COORDS:
            if(globals->ff.activeTextureUnit >= 8) { SetError(GL_INVALID_OPERATION); return; }
            memcpy(data, globals->ff.activeTextureUnit ? &globals->render.multiTexcoord[globals->ff.activeTextureUnit-1] : &globals->render.texcoord, 4 * sizeof(float));
            break;

        case GL_LIGHT_MODEL_AMBIENT:
            memcpy(data, &globals->render.ambient, 4 * sizeof(float));
            break;

        case 0x0B62: //GL_FOG_DENSITY:
            data[0] = globals->ff.fogDensity;
            break;

        case 0x0B63: //GL_FOG_START:
            data[0] = globals->ff.fogStart;
            break;

        case 0x0B64: //GL_FOG_END:
            data[0] = globals->ff.fogEnd;
            break;

        case 0x0B66: //GL_FOG_COLOR:
            memcpy(data, &globals->ff.fogColor, 4 * sizeof(float));
            if(FragmentColorClamped()) for(int i = 0; i < 4; ++i) data[i] = std::min(std::max(data[i], 0.0f), 1.0f);
            break;


        case 0x0B21: //GL_LINE_WIDTH:
            data[0] = globals->ff.lineWidth;
            break;
        case 0x0B07: /*GL_CURRENT_RASTER_POSITION*/
            data[0]=globals->render.rasterPos.x; data[1]=globals->render.rasterPos.y;
            data[2]=globals->render.rasterPos.z; data[3]=globals->render.rasterPos.w; return;
            
        case 0x0B52: /*GL_LIGHT_MODEL_TWO_SIDE*/
            data[0] = globals->ff.lightModelTwoSide ? 1.0f : 0.0f; return;
        case 0x0B51: /*GL_LIGHT_MODEL_LOCAL_VIEWER*/
            data[0] = globals->ff.lightModelLocalViewer ? 1.0f : 0.0f; return;

        // Point / line
        case 0x0B11: /*GL_POINT_SIZE*/  data[0]=globals->ff.pointSize;  return;

        // Alpha test
        case 0x0BC1: /*GL_ALPHA_TEST_FUNC*/ data[0]=(float)globals->ff.alphaTestFunc; return;
        case 0x0BC2: /*GL_ALPHA_TEST_REF*/  data[0]=globals->ff.alphaTestRef;         return;
            
        // TODO: more?
    }
}

void WRAP(glGetDoublev(GLenum pname, GLdouble* data))
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(LegacyState(pname, data)) return;
    switch(pname)
    {
        case GL_MODELVIEW_MATRIX:
        {
            const float* m = globals->matrix.modelview.Current().m;
            f4tod(m, data, 16); return;
        }
        case GL_PROJECTION_MATRIX:
        {
            const float* m = globals->matrix.projection.Current().m;
            f4tod(m, data, 16); return;
        }
        case GL_TEXTURE_MATRIX:
        {
            const float* m = globals->matrix.Texture().Current().m;
            f4tod(m, data, 16); return;
        }
        
        // TODO: more?
        
        default:
        {
            GLfloat tmp[16] = {};
            WRAP(glGetFloatv(pname, tmp));
            int n = 1;
            switch(pname)
            {
                case GL_VIEWPORT: case GL_SCISSOR_BOX: case GL_COLOR_CLEAR_VALUE: case GL_COLOR_WRITEMASK:
                case GL_BLEND_COLOR: case GL_CURRENT_COLOR: case GL_CURRENT_TEXTURE_COORDS:
                case GL_CURRENT_RASTER_POSITION: case GL_FOG_COLOR: case GL_LIGHT_MODEL_AMBIENT: n = 4; break;
                case GL_CURRENT_NORMAL: n = 3; break;
                case GL_DEPTH_RANGE: case GL_ALIASED_POINT_SIZE_RANGE: case GL_ALIASED_LINE_WIDTH_RANGE:
                case GL_MAX_VIEWPORT_DIMS: n = 2; break;
            }
            f4tod(tmp, data, n);
            return;
        }
    }
}

void WRAP(glPixelStoref(GLenum pname, GLfloat param))
{
    glPixelStorei(pname, param);
}

void WRAP(glAlphaFunc(GLenum func, GLfloat ref))
{
    // TODO: glAlphaFuncQCOM is everywhere?
    DLREC(WRAP(glAlphaFunc(func, ref)));
    if(func < GL_NEVER || func > GL_ALWAYS) { SetError(GL_INVALID_ENUM); return; }
    globals->ff.alphaTestFunc = func;
    globals->ff.alphaTestRef  = std::max(0.0f, std::min(1.0f, ref));
}

void WRAP(glPointSize(GLfloat size))
{
    DLREC(WRAP(glPointSize(size)));
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(size <= 0) { SetError(GL_INVALID_VALUE); return; }
    globals->ff.pointSize = size;
}

void WRAP(glLineWidth(GLfloat width))
{
    DLREC(WRAP(glLineWidth(width)));
    globals->ff.lineWidth = width;
    glLineWidth(width);
}

static void SetRasterPos(float x, float y, float z, float w)
{
    const float* mv  = globals->matrix.modelview.Current().m;
    const float* prj = globals->matrix.projection.Current().m;
    float pos[4] = { x, y, z, w };
    float eye[4], clip[4];
    
    // eye = MV * pos
    for(int r = 0; r < 4; ++r)
    {
        eye[r] = mv[0*4+r]*pos[0] + mv[1*4+r]*pos[1] + mv[2*4+r]*pos[2] + mv[3*4+r]*pos[3];
    }
    
    // clip = Proj * eye
    for(int r = 0; r < 4; ++r)
    {
        clip[r] = prj[0*4+r]*eye[0] + prj[1*4+r]*eye[1] + prj[2*4+r]*eye[2] + prj[3*4+r]*eye[3];
    }
    
    globals->render.rasterPos = { clip[0], clip[1], clip[2], clip[3] };
    globals->render.rasterPosValid = (clip[3] != 0.0f);
}
void WRAP(glRasterPos2f(GLfloat x, GLfloat y))  { DLREC(WRAP(glRasterPos2f(x,y)));         SetRasterPos(x,y,0,1); }
void WRAP(glRasterPos2d(GLdouble x, GLdouble y)) { WRAP(glRasterPos2f((float)x,(float)y)); }
void WRAP(glRasterPos2i(GLint x, GLint y))       { WRAP(glRasterPos2f((float)x,(float)y)); }
void WRAP(glRasterPos2s(GLshort x, GLshort y))   { WRAP(glRasterPos2f((float)x,(float)y)); }
void WRAP(glRasterPos3f(GLfloat x, GLfloat y, GLfloat z))  { DLREC(WRAP(glRasterPos3f(x,y,z))); SetRasterPos(x,y,z,1); }
void WRAP(glRasterPos3d(GLdouble x, GLdouble y, GLdouble z)){ WRAP(glRasterPos3f((float)x,(float)y,(float)z)); }
void WRAP(glRasterPos3i(GLint x, GLint y, GLint z))         { WRAP(glRasterPos3f((float)x,(float)y,(float)z)); }
void WRAP(glRasterPos3s(GLshort x, GLshort y, GLshort z))   { WRAP(glRasterPos3f((float)x,(float)y,(float)z)); }
void WRAP(glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)) { DLREC(WRAP(glRasterPos4f(x,y,z,w))); SetRasterPos(x,y,z,w); }
void WRAP(glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)){ WRAP(glRasterPos4f((float)x,(float)y,(float)z,(float)w)); }
void WRAP(glRasterPos4i(GLint x, GLint y, GLint z, GLint w))            { WRAP(glRasterPos4f((float)x,(float)y,(float)z,(float)w)); }
void WRAP(glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w))    { WRAP(glRasterPos4f((float)x,(float)y,(float)z,(float)w)); }
void WRAP(glRasterPos2fv(const GLfloat* v))  { WRAP(glRasterPos2f(v[0],v[1])); }
void WRAP(glRasterPos2dv(const GLdouble* v)) { WRAP(glRasterPos2f((float)v[0],(float)v[1])); }
void WRAP(glRasterPos2iv(const GLint* v))    { WRAP(glRasterPos2i(v[0],v[1])); }
void WRAP(glRasterPos3fv(const GLfloat* v))  { WRAP(glRasterPos3f(v[0],v[1],v[2])); }
void WRAP(glRasterPos3dv(const GLdouble* v)) { WRAP(glRasterPos3f((float)v[0],(float)v[1],(float)v[2])); }
void WRAP(glRasterPos4fv(const GLfloat* v))  { WRAP(glRasterPos4f(v[0],v[1],v[2],v[3])); }
void WRAP(glRasterPos4dv(const GLdouble* v)) { WRAP(glRasterPos4f((float)v[0],(float)v[1],(float)v[2],(float)v[3])); }

void WRAP(glWindowPos2f(GLfloat x, GLfloat y))
{
    DLREC(WRAP(glWindowPos2f(x,y)));
    globals->render.rasterPos = { x, y, 0.0f, 1.0f };
    globals->render.rasterPosValid = true;
}
void WRAP(glWindowPos2d(GLdouble x, GLdouble y)) { WRAP(glWindowPos2f((float)x,(float)y)); }
void WRAP(glWindowPos2i(GLint x, GLint y))       { WRAP(glWindowPos2f((float)x,(float)y)); }
void WRAP(glWindowPos3f(GLfloat x, GLfloat y, GLfloat z)) { DLREC(WRAP(glWindowPos3f(x,y,z))); globals->render.rasterPos={x,y,z,1}; globals->render.rasterPosValid=true; }
void WRAP(glWindowPos3d(GLdouble x, GLdouble y, GLdouble z)){ WRAP(glWindowPos3f((float)x,(float)y,(float)z)); }
void WRAP(glWindowPos2fv(const GLfloat* v))  { WRAP(glWindowPos2f(v[0],v[1])); }
void WRAP(glWindowPos2iv(const GLint* v))    { WRAP(glWindowPos2f((float)v[0],(float)v[1])); }
void WRAP(glWindowPos3fv(const GLfloat* v))  { WRAP(glWindowPos3f(v[0],v[1],v[2])); }

void WRAP(glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte* bitmap))
{
    if(globals->gl.conditionalDiscard) return;
    // TODO: Render bitmap at raster position via texture quad
    if(globals->render.rasterPosValid)
    {
        globals->render.rasterPos.x += xmove;
        globals->render.rasterPos.y += ymove;
    }
}

void WRAP(glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2))
{
    DLREC(WRAP(glRectf(x1,y1,x2,y2)));
    WRAP(glBegin(GL_TRIANGLE_FAN));
        WRAP(glVertex2f(x1, y1));
        WRAP(glVertex2f(x2, y1));
        WRAP(glVertex2f(x2, y2));
        WRAP(glVertex2f(x1, y2));
    WRAP(glEnd());
}
void WRAP(glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)) { WRAP(glRectf((float)x1,(float)y1,(float)x2,(float)y2)); }
void WRAP(glRecti(GLint x1, GLint y1, GLint x2, GLint y2))             { WRAP(glRectf((float)x1,(float)y1,(float)x2,(float)y2)); }
void WRAP(glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2))     { WRAP(glRectf((float)x1,(float)y1,(float)x2,(float)y2)); }
void WRAP(glRectfv(const GLfloat* v1, const GLfloat* v2))   { WRAP(glRectf(v1[0],v1[1],v2[0],v2[1])); }
void WRAP(glRectdv(const GLdouble* v1, const GLdouble* v2)) { WRAP(glRectf((float)v1[0],(float)v1[1],(float)v2[0],(float)v2[1])); }
void WRAP(glRectiv(const GLint* v1, const GLint* v2))       { WRAP(glRecti(v1[0],v1[1],v2[0],v2[1])); }
void WRAP(glRectsv(const GLshort* v1, const GLshort* v2))   { WRAP(glRects(v1[0],v1[1],v2[0],v2[1])); }

void WRAP(glSecondaryColor3f(GLfloat r, GLfloat g, GLfloat b))  { DLREC(WRAP(glSecondaryColor3f(r,g,b))); globals->render.secondaryColor = {r,g,b}; }
void WRAP(glSecondaryColor3d(GLdouble r, GLdouble g, GLdouble b)){ WRAP(glSecondaryColor3f((float)r,(float)g,(float)b)); }
void WRAP(glSecondaryColor3ub(GLubyte r, GLubyte g, GLubyte b))  { WRAP(glSecondaryColor3f(r/255.f,g/255.f,b/255.f)); }
void WRAP(glSecondaryColor3fv(const GLfloat* v))  { WRAP(glSecondaryColor3f(v[0],v[1],v[2])); }
void WRAP(glSecondaryColor3dv(const GLdouble* v)) { WRAP(glSecondaryColor3f((float)v[0],(float)v[1],(float)v[2])); }

void WRAP(glFogCoordf(GLfloat coord))  { DLREC(WRAP(glFogCoordf(coord))); globals->render.fogCoord = coord; }
void WRAP(glFogCoordd(GLdouble coord)) { WRAP(glFogCoordf((float)coord)); }
void WRAP(glFogCoordfv(const GLfloat* v))  { WRAP(glFogCoordf(v[0])); }
void WRAP(glFogCoorddv(const GLdouble* v)) { WRAP(glFogCoordf((float)v[0])); }

void WRAP(glHint(GLenum target, GLenum mode))
{
    switch(target)
    {
        case GL_PERSPECTIVE_CORRECTION_HINT:
            globals->ff.affineTexcoord = (mode == GL_FASTEST);
            break;
            
        default: return glHint(target, mode);
    }
}

void WRAP(glAccum(GLenum op, GLfloat value))           { if(!globals->gl.conditionalDiscard) SetError(GL_INVALID_OPERATION); }
void WRAP(glClearAccum(GLfloat r, GLfloat g, GLfloat b, GLfloat a)) { SetError(GL_INVALID_OPERATION); }
void WRAP(glPassThrough(GLfloat token))                { SetError(GL_INVALID_OPERATION); }
void WRAP(glSelectBuffer(GLsizei size, GLuint* buffer)) { SetError(GL_INVALID_OPERATION); }
void WRAP(glFeedbackBuffer(GLsizei size, GLenum type, GLfloat* buffer)) { SetError(GL_INVALID_OPERATION); }
GLint WRAP(glRenderMode(GLenum mode))                   { if(mode != 0x1C00) SetError(GL_INVALID_OPERATION); return 0; }
void WRAP(glInitNames())                                { SetError(GL_INVALID_OPERATION); }
void WRAP(glPushName(GLuint name))                      { SetError(GL_INVALID_OPERATION); }
void WRAP(glPopName())                                  { SetError(GL_INVALID_OPERATION); }
void WRAP(glLoadName(GLuint name))                      { SetError(GL_INVALID_OPERATION); }

void WRAP(glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat* points)) { SetError(GL_INVALID_OPERATION); }
void WRAP(glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble* points)) { SetError(GL_INVALID_OPERATION); }
void WRAP(glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat* points)) { SetError(GL_INVALID_OPERATION); }
void WRAP(glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble* points)) { SetError(GL_INVALID_OPERATION); }
void WRAP(glMapGrid1f(GLint un, GLfloat u1, GLfloat u2)) { SetError(GL_INVALID_OPERATION); }
void WRAP(glMapGrid1d(GLint un, GLdouble u1, GLdouble u2)) { SetError(GL_INVALID_OPERATION); }
void WRAP(glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)) { SetError(GL_INVALID_OPERATION); }
void WRAP(glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)) { SetError(GL_INVALID_OPERATION); }
void WRAP(glEvalMesh1(GLenum mode, GLint i1, GLint i2))  { if(!globals->gl.conditionalDiscard) SetError(GL_INVALID_OPERATION); }
void WRAP(glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)) { if(!globals->gl.conditionalDiscard) SetError(GL_INVALID_OPERATION); }
void WRAP(glEvalPoint1(GLint i))  { SetError(GL_INVALID_OPERATION); }
void WRAP(glEvalPoint2(GLint i, GLint j)) { SetError(GL_INVALID_OPERATION); }
void WRAP(glEvalCoord1f(GLfloat u)) { SetError(GL_INVALID_OPERATION); }
void WRAP(glEvalCoord1d(GLdouble u)) { SetError(GL_INVALID_OPERATION); }
void WRAP(glEvalCoord2f(GLfloat u, GLfloat v)) { SetError(GL_INVALID_OPERATION); }
void WRAP(glEvalCoord2d(GLdouble u, GLdouble v)) { SetError(GL_INVALID_OPERATION); }
void WRAP(glGetMapiv(GLenum target, GLenum query, GLint* v)) { SetError(GL_INVALID_OPERATION); }
void WRAP(glGetMapfv(GLenum target, GLenum query, GLfloat* v)) { SetError(GL_INVALID_OPERATION); }
void WRAP(glGetMapdv(GLenum target, GLenum query, GLdouble* v)) { SetError(GL_INVALID_OPERATION); }

void WRAP(glGetBooleanv(GLenum pname, GLboolean* params))
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    GLdouble values[4];
    int count = LegacyState(pname, values);
    if(count) { for(int i = 0; i < count; ++i) params[i] = values[i] != 0; return; }
    if(pname == 0x8F9D || pname == 0x8F9E)
    {
        params[0] = (GLboolean)((pname == 0x8F9D ? globals->gl.primitiveRestart : globals->gl.restartIndex) != 0);
        return;
    }

    switch(pname)
    {
        case GL_LIGHTING:           *params = globals->ff.lightingEnabled;   return;
        case GL_NORMALIZE:          *params = globals->ff.normalizeEnabled;  return;
        case GL_FOG:                *params = globals->ff.fogEnabled;        return;
        case GL_ALPHA_TEST:         *params = globals->ff.alphaTestEnabled;  return;
        case GL_COLOR_MATERIAL:     *params = globals->render.colorMaterial; return;
        case GL_TEXTURE_2D:         *params = globals->render.texture;       return;
        case 0x0BF1: /*GL_LOGIC_OP*/*params = globals->ff.logicOpEnabled;   return;
        case GL_LIGHT0: case GL_LIGHT1: case GL_LIGHT2: case GL_LIGHT3:
        case GL_LIGHT4: case GL_LIGHT5: case GL_LIGHT6: case GL_LIGHT7:
            *params = globals->ff.lightEnabled[pname - GL_LIGHT0];          return;
        case GL_CLIP_PLANE0: case GL_CLIP_PLANE1: case GL_CLIP_PLANE2:
        case GL_CLIP_PLANE3: case GL_CLIP_PLANE4: case GL_CLIP_PLANE5:
            *params = globals->ff.clipPlaneOn[pname - GL_CLIP_PLANE0];      return;
            
        default:
            glGetBooleanv(pname, params);
            return;
    }
}

static int GetLight(GLenum light, GLenum pname, GLfloat* params)
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return 0; }
    int idx = (int)(light - GL_LIGHT0);
    if(idx < 0 || idx > 7 || pname < GL_AMBIENT || pname > GL_QUADRATIC_ATTENUATION) { SetError(GL_INVALID_ENUM); return 0; }
    const fixed_light_t& L = globals->ff.lights[idx];
    switch(pname)
    {
        case 0x1200: memcpy(params, &L.ambient,  4*sizeof(float)); break; // GL_AMBIENT
        case 0x1201: memcpy(params, &L.diffuse,  4*sizeof(float)); break; // GL_DIFFUSE
        case 0x1202: memcpy(params, &L.spec,     4*sizeof(float)); break; // GL_SPECULAR
        case 0x1203: memcpy(params, &L.pos,      4*sizeof(float)); break; // GL_POSITION
        case 0x1204: memcpy(params, &L.dir,      3*sizeof(float)); break; // GL_SPOT_DIRECTION
        case 0x1205: params[0] = L.spotExp;      break;                   // GL_SPOT_EXPONENT
        case 0x1206: params[0] = L.spotCutoff;   break;                   // GL_SPOT_CUTOFF
        case 0x1207: params[0] = L.attenuationConst;  break;              // GL_CONSTANT_ATTENUATION
        case 0x1208: params[0] = L.attenuationLinear; break;              // GL_LINEAR_ATTENUATION
        case 0x1209: params[0] = L.attenuationQuad;   break;              // GL_QUADRATIC_ATTENUATION
        default: break;
    }
    return pname == GL_SPOT_DIRECTION ? 3 : pname <= GL_POSITION ? 4 : 1;
}

void WRAP(glGetLightfv(GLenum light, GLenum pname, GLfloat* params))
{
    GetLight(light, pname, params);
}
void WRAP(glGetLightiv(GLenum light, GLenum pname, GLint* params))
{
    GLfloat values[4];
    int count = GetLight(light, pname, values);
    for(int i = 0; i < count; ++i)
    {
        GLdouble value = values[i];
        if(pname >= GL_AMBIENT && pname <= GL_SPECULAR) value *= value < 0 ? 2147483648.0 : 2147483647.0;
        value = std::round(value);
        params[i] = value >= INT32_MAX ? INT32_MAX : value <= INT32_MIN ? INT32_MIN : std::isnan(value) ? 0 : (GLint)value;
    }
}

static int GetMaterial(GLenum face, GLenum pname, GLfloat* params)
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return 0; }
    if(face != GL_FRONT && face != GL_BACK) { SetError(GL_INVALID_ENUM); return 0; }
    const auto& material = globals->ff.materials[face == GL_BACK ? 1 : 0];
    switch(pname)
    {
        case GL_AMBIENT: memcpy(params, material.ambient, 4*sizeof(float)); return 4;
        case GL_DIFFUSE: memcpy(params, material.diffuse, 4*sizeof(float)); return 4;
        case GL_SPECULAR: memcpy(params, material.specular, 4*sizeof(float)); return 4;
        case GL_EMISSION: memcpy(params, material.emission, 4*sizeof(float)); return 4;
        case GL_SHININESS: params[0] = material.shininess; return 1;
        case 0x1603: memcpy(params, material.indexes, 3*sizeof(float)); return 3;
        default: SetError(GL_INVALID_ENUM); return 0;
    }
}

void WRAP(glGetMaterialfv(GLenum face, GLenum pname, GLfloat* params))
{
    GetMaterial(face, pname, params);
}

void WRAP(glGetMaterialiv(GLenum face, GLenum pname, GLint* params))
{
    GLfloat values[4];
    int count = GetMaterial(face, pname, values);
    for(int i = 0; i < count; ++i)
    {
        GLdouble value = values[i];
        if(count == 4) value *= value < 0 ? 2147483648.0 : 2147483647.0;
        value = std::round(value);
        params[i] = value >= INT32_MAX ? INT32_MAX : value <= INT32_MIN ? INT32_MIN : std::isnan(value) ? 0 : (GLint)value;
    }
}

void WRAP(glGetClipPlane(GLenum plane, GLdouble* equation))
{
    int idx = (int)(plane - GL_CLIP_PLANE0);
    if(idx < 0 || idx > 5) return;
    equation[0] = globals->ff.clipPlanes[idx][0];
    equation[1] = globals->ff.clipPlanes[idx][1];
    equation[2] = globals->ff.clipPlanes[idx][2];
    equation[3] = globals->ff.clipPlanes[idx][3];
}

static bool GetTextureEnv(GLenum target, GLenum pname, GLfloat* params)
{
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return false; }
    int unit = globals->ff.activeTextureUnit;
    if(unit >= 8) { SetError(GL_INVALID_OPERATION); return false; }
    if(target == 0x8861 && pname == 0x8862) { *params = globals->ff.pointCoordReplace[unit]; return true; }
    if(target != GL_TEXTURE_ENV) { SetError(GL_INVALID_ENUM); return false; }
    const auto& ts = globals->ff.texEnv[unit];
    if(pname == GL_TEXTURE_ENV_MODE)
    {
        params[0] = (float)ts.mode;
    }
    else if(pname == GL_TEXTURE_ENV_COLOR)
    {
        memcpy(params, &ts.color, 4 * sizeof(float));
        if(FragmentColorClamped()) for(int i = 0; i < 4; ++i) params[i] = std::min(std::max(params[i], 0.0f), 1.0f);
    }
    else if(pname == 0x8571) params[0] = (float)ts.combineRGB;
    else if(pname == 0x8572) params[0] = (float)ts.combineAlpha;
    else if(pname == 0x8573) params[0] = (float)ts.scaleRGB;
    else if(pname == 0x0D1C) params[0] = (float)ts.scaleAlpha;
    else if(pname >= 0x8580 && pname <= 0x8582) params[0] = (float)ts.sourceRGB[pname-0x8580];
    else if(pname >= 0x8588 && pname <= 0x858A) params[0] = (float)ts.sourceAlpha[pname-0x8588];
    else if(pname >= 0x8590 && pname <= 0x8592) params[0] = (float)ts.operandRGB[pname-0x8590];
    else if(pname >= 0x8598 && pname <= 0x859A) params[0] = (float)ts.operandAlpha[pname-0x8598];
    else { SetError(GL_INVALID_ENUM); return false; }
    return true;
}
void WRAP(glGetTexEnvfv(GLenum target, GLenum pname, GLfloat* params)) { GetTextureEnv(target, pname, params); }
void WRAP(glGetTexEnviv(GLenum target, GLenum pname, GLint* params))
{
    GLfloat tmp[4];
    if(!GetTextureEnv(target, pname, tmp)) return;
    for(int i = 0; i < (pname == GL_TEXTURE_ENV_COLOR ? 4 : 1); ++i)
    {
        double value = pname == GL_TEXTURE_ENV_COLOR ? (double)tmp[i] * (tmp[i] < 0 ? 2147483648.0 : 2147483647.0) : tmp[i];
        value = std::round(value);
        params[i] = std::isnan(value) ? 0 : value >= INT32_MAX ? INT32_MAX : value <= INT32_MIN ? INT32_MIN : (GLint)value;
    }
}

static bool GetTextureGen(GLenum coord, GLenum pname, GLfloat* params)
{
    if(globals->render.begin || globals->ff.activeTextureUnit >= 8) { SetError(GL_INVALID_OPERATION); return false; }
    int c = (int)(coord - GL_S);
    if(c < 0 || c > 3) { SetError(GL_INVALID_ENUM); return false; }
    const auto& tg = globals->ff.texGen[globals->ff.activeTextureUnit][c];
    if(pname == GL_TEXTURE_GEN_MODE)   { params[0] = (float)tg.mode; }
    else if(pname == GL_OBJECT_PLANE)  { memcpy(params, tg.objectPlane, 4*sizeof(float)); }
    else if(pname == GL_EYE_PLANE)     { memcpy(params, tg.eyePlane,    4*sizeof(float)); }
    else { SetError(GL_INVALID_ENUM); return false; }
    return true;
}
void WRAP(glGetTexGenfv(GLenum coord, GLenum pname, GLfloat* params)) { GetTextureGen(coord,pname,params); }
void WRAP(glGetTexGendv(GLenum coord, GLenum pname, GLdouble* params))
{
    GLfloat tmp[4];
    if(!GetTextureGen(coord,pname,tmp)) return;
    f4tod(tmp, params, pname == GL_TEXTURE_GEN_MODE ? 1 : 4);
}
void WRAP(glGetTexGeniv(GLenum coord, GLenum pname, GLint* params))
{
    GLfloat tmp[4];
    if(!GetTextureGen(coord,pname,tmp)) return;
    for(int i=0;i<(pname == GL_TEXTURE_GEN_MODE ? 1 : 4);++i) params[i]=(GLint)tmp[i];
}

// TEXTURE

void WRAP(glTexImage1D(GLenum target, GLint level, GLint internalformat,
                       GLsizei width, GLint border, GLenum format, GLenum type, const void* pixels))
{
    WRAP(glTexImage2D(GL_TEXTURE_2D, level, internalformat, width, 1, border, format, type, pixels));
}

void WRAP(glTexSubImage1D(GLenum target, GLint level, GLint xoffset,
                          GLsizei width, GLenum format, GLenum type, const void* pixels))
{
    WRAP(glTexSubImage2D(GL_TEXTURE_2D, level, xoffset, 0, width, 1, format, type, pixels));
}

void WRAP(glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat,
                           GLint x, GLint y, GLsizei width, GLint border))
{
    WRAP(glCopyTexImage2D(GL_TEXTURE_2D, level, internalformat, x, y, width, 1, border));
}

void WRAP(glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset,
                              GLint x, GLint y, GLsizei width))
{
    WRAP(glCopyTexSubImage2D(GL_TEXTURE_2D, level, xoffset, 0, x, y, width, 1));
}

void WRAP(glFramebufferTexture1D(GLenum target, GLenum attachment,
                                 GLenum textarget, GLuint texture, GLint level))
{
    if(textarget != 0x0DE0) { SetError(GL_INVALID_ENUM); return; }
    WRAP(glFramebufferTexture2D(target, attachment, GL_TEXTURE_2D, texture, level));
}

void WRAP(glClampColor(GLenum target, GLenum clamp))
{
    DLREC(WRAP(glClampColor(target, clamp)));
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(target < 0x891A || target > 0x891C || (clamp != GL_TRUE && clamp != GL_FALSE && clamp != 0x891D)) { SetError(GL_INVALID_ENUM); return; }
    if(target == 0x891A) globals->ff.clampVertexColor = clamp;
    else if(target == 0x891B) globals->ff.clampFragmentColor = clamp;
    else globals->ff.clampReadColor = clamp;
}

void WRAP(glPrioritizeTextures(GLsizei n, const GLuint* textures, const GLclampf* priorities))
{
    
}
GLboolean WRAP(glAreTexturesResident(GLsizei n, const GLuint* textures, GLboolean* residences))
{
    for(GLsizei i = 0; i < n; ++i) residences[i] = GL_TRUE;
    return GL_TRUE;
}

void WRAP(glColorTable(GLenum target, GLenum internalformat, GLsizei width,
                       GLenum format, GLenum type, const void* table))            { SetError(GL_INVALID_OPERATION); }
void WRAP(glColorTableParameterfv(GLenum target, GLenum pname, const GLfloat* p)) { SetError(GL_INVALID_OPERATION); }
void WRAP(glColorTableParameteriv(GLenum target, GLenum pname, const GLint* p))   { SetError(GL_INVALID_OPERATION); }
void WRAP(glCopyColorTable(GLenum target, GLenum internalformat,
                           GLint x, GLint y, GLsizei width))                      { SetError(GL_INVALID_OPERATION); }
void WRAP(glGetColorTable(GLenum target, GLenum format, GLenum type, void* table)){ SetError(GL_INVALID_OPERATION); }
void WRAP(glGetColorTableParameterfv(GLenum t, GLenum p, GLfloat* v))             { SetError(GL_INVALID_OPERATION); }
void WRAP(glGetColorTableParameteriv(GLenum t, GLenum p, GLint* v))               { SetError(GL_INVALID_OPERATION); }
void WRAP(glColorSubTable(GLenum target, GLsizei start, GLsizei count,
                          GLenum format, GLenum type, const void* data))           { SetError(GL_INVALID_OPERATION); }
void WRAP(glCopyColorSubTable(GLenum target, GLsizei start,
                              GLint x, GLint y, GLsizei width))                    { SetError(GL_INVALID_OPERATION); }

void WRAP(glConvolutionFilter1D(GLenum t,GLenum i,GLsizei w,GLenum f,GLenum ty,const void* d))    { SetError(GL_INVALID_OPERATION); }
void WRAP(glConvolutionFilter2D(GLenum t,GLenum i,GLsizei w,GLsizei h,GLenum f,GLenum ty,const void* d)){ SetError(GL_INVALID_OPERATION); }
void WRAP(glConvolutionParameterf(GLenum t,GLenum p,GLfloat v))                    { SetError(GL_INVALID_OPERATION); }
void WRAP(glConvolutionParameterfv(GLenum t,GLenum p,const GLfloat* v))            { SetError(GL_INVALID_OPERATION); }
void WRAP(glConvolutionParameteri(GLenum t,GLenum p,GLint v))                      { SetError(GL_INVALID_OPERATION); }
void WRAP(glConvolutionParameteriv(GLenum t,GLenum p,const GLint* v))              { SetError(GL_INVALID_OPERATION); }
void WRAP(glCopyConvolutionFilter1D(GLenum t,GLenum i,GLint x,GLint y,GLsizei w)) { SetError(GL_INVALID_OPERATION); }
void WRAP(glCopyConvolutionFilter2D(GLenum t,GLenum i,GLint x,GLint y,GLsizei w,GLsizei h)){ SetError(GL_INVALID_OPERATION); }
void WRAP(glGetConvolutionFilter(GLenum t,GLenum f,GLenum ty,void* img))           { SetError(GL_INVALID_OPERATION); }
void WRAP(glGetConvolutionParameterfv(GLenum t,GLenum p,GLfloat* v))              { SetError(GL_INVALID_OPERATION); }
void WRAP(glGetConvolutionParameteriv(GLenum t,GLenum p,GLint* v))                { SetError(GL_INVALID_OPERATION); }
void WRAP(glSeparableFilter2D(GLenum t,GLenum i,GLsizei w,GLsizei h,GLenum f,GLenum ty,const void* r,const void* c)){ SetError(GL_INVALID_OPERATION); }
void WRAP(glGetSeparableFilter(GLenum t,GLenum f,GLenum ty,void* r,void* c,void* sp)){ SetError(GL_INVALID_OPERATION); }

void WRAP(glHistogram(GLenum t,GLsizei w,GLenum i,GLboolean s))                   { SetError(GL_INVALID_OPERATION); }
void WRAP(glResetHistogram(GLenum t))                                              { SetError(GL_INVALID_OPERATION); }
void WRAP(glGetHistogram(GLenum t,GLboolean r,GLenum f,GLenum ty,void* v))        { SetError(GL_INVALID_OPERATION); }
void WRAP(glGetHistogramParameterfv(GLenum t,GLenum p,GLfloat* v))                { SetError(GL_INVALID_OPERATION); }
void WRAP(glGetHistogramParameteriv(GLenum t,GLenum p,GLint* v))                  { SetError(GL_INVALID_OPERATION); }
void WRAP(glMinmax(GLenum t,GLenum i,GLboolean s))                                { SetError(GL_INVALID_OPERATION); }
void WRAP(glResetMinmax(GLenum t))                                                 { SetError(GL_INVALID_OPERATION); }
void WRAP(glGetMinmax(GLenum t,GLboolean r,GLenum f,GLenum ty,void* v))           { SetError(GL_INVALID_OPERATION); }
void WRAP(glGetMinmaxParameterfv(GLenum t,GLenum p,GLfloat* v))                   { SetError(GL_INVALID_OPERATION); }
void WRAP(glGetMinmaxParameteriv(GLenum t,GLenum p,GLint* v))                     { SetError(GL_INVALID_OPERATION); }
