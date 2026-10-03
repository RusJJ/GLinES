#include "GLES.h"
#include "gl_buffer.h"
#include "gl_matrix.h"
#include "gl_object.h"
#include "gl_queries.h"
#include "gl_render.h"
#include "gl_shader.h"
#include "gl_texture.h"

#include "glhelper.h"
#include "wrapped.h"

#include <unordered_map>
#include <string>

struct fixed_program_t;

thread_local unsigned long long g_nFixedPipelineShaderFlags = 0;
#define FL(x) ((g_nFixedPipelineShaderFlags & (x)) != 0)
#define EFL(x) (g_nFixedPipelineShaderFlags |= (x))

thread_local GLuint g_nUberShader = 0;
thread_local fixed_program_t* activeFixedProgram = NULL;

enum eShaderFlags : unsigned long long
{
    NONE         = 0,
    SF_TEXTURED  = (1ULL << 0),
    SF_LIGHT0    = (1ULL << 1),
    SF_LIGHT1    = (1ULL << 2),
    SF_LIGHT2    = (1ULL << 3),
    SF_LIGHT3    = (1ULL << 4),
    SF_LIGHT4    = (1ULL << 5),
    SF_LIGHT5    = (1ULL << 6),
    SF_LIGHT6    = (1ULL << 7),
    SF_LIGHT7    = (1ULL << 8),
    SF_FOG_LINEAR= (1ULL << 9),
    SF_FOG_EXP   = (1ULL << 10),
    SF_FOG_EXP2  = (1ULL << 11),
    SF_TEXUNIT1  = (1ULL << 12),
    SF_TEXUNIT2  = (1ULL << 13),
    SF_TEXUNIT3  = (1ULL << 14),
    SF_TEXUNIT4  = (1ULL << 15),
    SF_TEXUNIT5  = (1ULL << 16),
    SF_TEXUNIT6  = (1ULL << 17),
    SF_TEXUNIT7  = (1ULL << 18),
    SF_LIGHTING  = (1ULL << 19),
    SF_COLOR_MAT = (1ULL << 20),
    SF_FLATSHADING=(1ULL << 21),
    SF_AFFINE    = (1ULL << 22),
    SF_ALPHATEST = (1ULL << 23),
    SF_TWOSIDE   = (1ULL << 24),
    SF_CLIPPLANE1 = (1ULL << 25),
    SF_CLIPPLANE2 = (1ULL << 26),
    SF_CLIPPLANE3 = (1ULL << 27),
    SF_CLIPPLANE4 = (1ULL << 28),
    SF_CLIPPLANE5 = (1ULL << 29),
    SF_CLIPPLANE6 = (1ULL << 30),
    SF_NORMALIZE = (1ULL << 31),
    SF_MATERIAL_ARRAY = (1ULL << 32),
    SF_CLAMP_VERTEX = (1ULL << 33),
    SF_CLAMP_FRAGMENT = (1ULL << 34),
};

struct fixed_uniform_t
{
    fixed_uniform_t() { mat4 = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; }
    GLint id = -1;
    union
    {
        float f;
        int i;
        vector2_t vec2;
        vector3_t vec3;
        vector4_t vec4;
        matrix3_t mat3;
        matrix4_t mat4;
    };
    
    inline void Apply(float v)
    {
        if(id == -1 || f == v) return;
        glUniform1f(id, v);
        f = v;
    }
    inline void Apply(const float* v, int num)
    {
        if(id == -1) return;
        glUniform1fv(id, num, v);
    }
    inline void Apply(int v)
    {
        if(id == -1 || i == v) return;
        glUniform1i(id, v);
        i = v;
    }
    inline void Apply(const int* v, int num)
    {
        if(id == -1) return;
        glUniform1iv(id, num, v);
    }
    inline void Apply(const vector2_t& v)
    {
        if(id == -1 || vec2 == v) return;
        glUniform2f(id, v.x, v.y);
        vec2 = v;
    }
    inline void Apply(const vector2_t* v, int num)
    {
        if(id == -1) return;
        glUniform2fv(id, num, (GLfloat*)v);
    }
    inline void Apply(const vector3_t& v)
    {
        if(id == -1 || vec3 == v) return;
        glUniform3f(id, v.x, v.y, v.z);
        vec3 = v;
    }
    inline void Apply(const vector4_t& v)
    {
        if(id == -1 || vec4 == v) return;
        glUniform4f(id, v.x, v.y, v.z, v.w);
        vec4 = v;
    }
    inline void Apply(const matrix3_t& v, bool transpose = false)
    {
        if(id == -1 || mat3 == v) return;
        glUniformMatrix3fv(id, 1, transpose ? GL_TRUE : GL_FALSE, v.m);
        mat3 = v;
    }
    inline void Apply(const matrix4_t& v, bool transpose = false)
    {
        if(id == -1 || mat4 == v) return;
        glUniformMatrix4fv(id, 1, transpose ? GL_TRUE : GL_FALSE, v.m);
        mat4 = v;
    }
};

struct fixed_lights_uniform_t : fixed_uniform_t
{
    fixed_lights_uniform_t() { memset(&lights, 0, sizeof(lights)); }
    fixed_light_t lights[8];

    GLuint ubo = 0;
    
    inline void Init()
    {
        glGenBuffers(1, &ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(lights), NULL, GL_DYNAMIC_DRAW);
        
        id = glGetUniformBlockIndex(g_nUberShader, "u_lightBlock");
        if(id != (GLint)GL_INVALID_INDEX) glUniformBlockBinding(g_nUberShader, id, 0);
    }
    inline void Apply(const fixed_light_t* v)
    {
        if(ubo == 0) return;
        
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
        if(memcmp(&lights, v, sizeof(lights)) == 0) return;
 
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(lights), v);

        memcpy(&lights, v, sizeof(lights));
    }
};

struct fixed_program_t
{
    GLuint program;
    fixed_uniform_t uModelView;
    fixed_uniform_t uProj;
    fixed_uniform_t uNormal;
    fixed_uniform_t uDiffuse;
    fixed_uniform_t uFogColor;
    fixed_uniform_t uFogValues;
    fixed_uniform_t uAmbientColor;
    fixed_uniform_t uTexCoords;
    fixed_uniform_t uTexColors;
    fixed_uniform_t uTexModes;
    fixed_uniform_t uAlphaOnly;
    fixed_uniform_t uTexIDs;
    fixed_uniform_t uPointSize;
    fixed_uniform_t uPointLimits;
    fixed_uniform_t uPointAttenuation;
    fixed_uniform_t uPointSprite;
    fixed_uniform_t uObjectPlanes;
    fixed_uniform_t uEyePlanes;
    fixed_uniform_t uTexMatrix;
    fixed_uniform_t uTexMode0;
    fixed_uniform_t uTexColor0;
    fixed_uniform_t uShininess[2];
    fixed_uniform_t uMatAmbient[2];
    fixed_uniform_t uMatDiffuse[2];
    fixed_uniform_t uMatSpecular[2];
    fixed_uniform_t uMatEmission[2];
    fixed_uniform_t uMaterialWidth;
    fixed_uniform_t uMaterialValues;
    fixed_lights_uniform_t uLights;
    fixed_uniform_t uAlphaRef;   // alpha test ref value
    fixed_uniform_t uAlphaFunc;  // alpha test func (int enum)
    fixed_uniform_t uClipPlanes; // 6 clip planes as vec4[6]
};

#define g_mapFixedPrograms globals->fixedPrograms

static bool FixedColorBuffers()
{
    GLint count, framebuffer;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &framebuffer);
    glGetIntegerv(GL_MAX_DRAW_BUFFERS, &count);
    for(int i = 0; i < (framebuffer ? count : 1); ++i)
    {
        GLint buffer, type;
        glGetIntegerv(GL_DRAW_BUFFER0 + i, &buffer);
        if(buffer == GL_NONE) continue;
        glGetFramebufferAttachmentParameteriv(GL_DRAW_FRAMEBUFFER, buffer, GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE, &type);
        if(type != GL_UNSIGNED_NORMALIZED && type != GL_SIGNED_NORMALIZED) return false;
    }
    return true;
}

bool FragmentColorClamped()
{
    return globals->ff.clampFragmentColor == GL_TRUE || (globals->ff.clampFragmentColor == 0x891D && FixedColorBuffers());
}

inline void BuildShaderFlag()
{
    g_nFixedPipelineShaderFlags = 0;
    if(globals->ff.clampVertexColor == GL_TRUE || (globals->ff.clampVertexColor == 0x891D && FixedColorBuffers())) EFL(SF_CLAMP_VERTEX);
    if(FragmentColorClamped()) EFL(SF_CLAMP_FRAGMENT);
    
    if(globals->render.texture) EFL(SF_TEXTURED);
    if(globals->ff.lightingEnabled)
    {
        EFL(SF_LIGHTING);
        if(globals->render.materialWidth) EFL(SF_MATERIAL_ARRAY);
        if(globals->ff.lightEnabled[0]) EFL(SF_LIGHT0);
        if(globals->ff.lightEnabled[1]) EFL(SF_LIGHT1);
        if(globals->ff.lightEnabled[2]) EFL(SF_LIGHT2);
        if(globals->ff.lightEnabled[3]) EFL(SF_LIGHT3);
        if(globals->ff.lightEnabled[4]) EFL(SF_LIGHT4);
        if(globals->ff.lightEnabled[5]) EFL(SF_LIGHT5);
        if(globals->ff.lightEnabled[6]) EFL(SF_LIGHT6);
        if(globals->ff.lightEnabled[7]) EFL(SF_LIGHT7);
        if(globals->render.colorMaterial) EFL(SF_COLOR_MAT);
        if(globals->ff.lightModelTwoSide) EFL(SF_TWOSIDE);
    }
    if(globals->ff.fogEnabled)
    {
        if(globals->ff.fogMode == GL_EXP2)   EFL(SF_FOG_EXP2);
        else if(globals->ff.fogMode == GL_LINEAR) EFL(SF_FOG_LINEAR);
        else EFL(SF_FOG_EXP);
    }
    if(globals->ff.textureEnabled[1]) EFL(SF_TEXUNIT1);
    if(globals->ff.textureEnabled[2]) EFL(SF_TEXUNIT2);
    if(globals->ff.textureEnabled[3]) EFL(SF_TEXUNIT3);
    if(globals->ff.textureEnabled[4]) EFL(SF_TEXUNIT4);
    if(globals->ff.textureEnabled[5]) EFL(SF_TEXUNIT5);
    if(globals->ff.textureEnabled[6]) EFL(SF_TEXUNIT6);
    if(globals->ff.textureEnabled[7]) EFL(SF_TEXUNIT7);
    if(globals->ff.shadeModel == GL_FLAT) EFL(SF_FLATSHADING);
    if(globals->ff.affineTexcoord) EFL(SF_AFFINE);
    if(globals->ff.alphaTestEnabled) EFL(SF_ALPHATEST);
    if(globals->ff.normalizeEnabled) EFL(SF_NORMALIZE);
    if(globals->ff.clipPlaneOn[0]) EFL(SF_CLIPPLANE1);
    if(globals->ff.clipPlaneOn[1]) EFL(SF_CLIPPLANE2);
    if(globals->ff.clipPlaneOn[2]) EFL(SF_CLIPPLANE3);
    if(globals->ff.clipPlaneOn[3]) EFL(SF_CLIPPLANE4);
    if(globals->ff.clipPlaneOn[4]) EFL(SF_CLIPPLANE5);
    if(globals->ff.clipPlaneOn[5]) EFL(SF_CLIPPLANE6);
}

std::string BuildVertexShader()
{
    // Header
    std::string s = "#version 300 es\nprecision highp float;\n";
    s += "layout(location = 0) in vec4 a_position;\n";
    s += "layout(location = 2) in vec3 a_normal;\n";
    s += "layout(location = 3) in vec4 a_color;\n";
    s += "layout(location = 4) in vec3 a_secondaryColor;\nlayout(location = 5) in float a_fogCoord;\n";
    s += FL(SF_FLATSHADING) ? "flat out vec3 v_secondaryColor;\n" : "out vec3 v_secondaryColor;\n";
    if(FL(SF_TWOSIDE))
    {
        s += FL(SF_FLATSHADING) ? "flat out lowp vec4 v_backColor;\nflat out vec3 v_backSecondaryColor;\n" : "out lowp vec4 v_backColor;\nout vec3 v_backSecondaryColor;\n";
    }
    s += "layout(location = 8) in vec4 a_texCoord;\n";
    s += "uniform mat4 u_modelview;\n";
    s += "uniform mat4 u_proj;\nuniform mat4 u_texMatrix[8];\nuniform float u_pointSize;\n";
    s += "uniform vec2 u_pointLimits;\nuniform vec3 u_pointAttenuation;\n";
    s += "uniform vec4 u_objectPlanes[32];\nuniform vec4 u_eyePlanes[32];\n";
    s += "uniform mat3 u_normal;\n";
    if(FL(SF_FLATSHADING))
    {
        s += "flat out lowp vec4 v_color;\n";
    }
    else
    {
        s += "out lowp vec4 v_color;\n";
    }
    if(FL(SF_AFFINE))
    {
        s += "out vec4 v_texCoord;\n";
    }
    else
    {
        s += "out vec4 v_texCoord;\n";
    }
    s += "out vec4 v_position;\n";
    s += "out vec4 v_eyePos;\n"; // needed for clip planes & two-side
    if(FL(SF_FOG_EXP2) || FL(SF_FOG_LINEAR) || FL(SF_FOG_EXP))
    {
        s += "out float v_eyeDepth;\n";
    }
    if(FL(SF_TEXUNIT1) || FL(SF_TEXUNIT2) || FL(SF_TEXUNIT3) || FL(SF_TEXUNIT4) ||
        FL(SF_TEXUNIT5) || FL(SF_TEXUNIT6) || FL(SF_TEXUNIT7))
    {
        s += "layout(location = 9)  in vec4 a_texCoord1;\n";
        s += "layout(location = 10) in vec4 a_texCoord2;\n";
        s += "layout(location = 11) in vec4 a_texCoord3;\n";
        s += "layout(location = 12) in vec4 a_texCoord4;\n";
        s += "layout(location = 13) in vec4 a_texCoord5;\n";
        s += "layout(location = 14) in vec4 a_texCoord6;\n";
        s += "layout(location = 15) in vec4 a_texCoord7;\n";
        s += "out vec4 v_texCoords[7];\n";
    }
    if(FL(SF_LIGHTING))
    {
        s += "uniform vec4 u_ambientColor;\n";
        s += "uniform vec4 u_matAmbient[2];\n";
        s += "uniform vec4 u_matDiffuse[2];\n";
        s += "uniform vec4 u_matSpecular[2];\n";
        s += "uniform vec4 u_matEmission[2];\n";
        s += "uniform float u_shininess[2];\nfloat shininess;\n";
        s += "vec4 matAmbient, matDiffuse, matSpecular, matEmission;\n";
        if(FL(SF_MATERIAL_ARRAY))
        {
            s += "uniform highp sampler2D u_materialValues;\nuniform int u_materialWidth;\n";
            s += "vec4 materialValue(int offset) {\n";
            s += "  int address = gl_VertexID * 10 + offset;\n";
            s += "  return texelFetch(u_materialValues, ivec2(address % u_materialWidth, address / u_materialWidth), 0);\n}\n";
        }
    }
    if(FL(SF_LIGHT0) || FL(SF_LIGHT1) || FL(SF_LIGHT2) || FL(SF_LIGHT3) || 
            FL(SF_LIGHT4) || FL(SF_LIGHT5) || FL(SF_LIGHT6) || FL(SF_LIGHT7))
    {
        s += "struct LightData {\n";
        s += "  vec4 position;\n";
        s += "  vec4 ambient;\n";
        s += "  vec4 diffuse;\n";
        s += "  vec4 specular;\n";
        s += "  vec4 spotDir;\n";
        s += "  vec4 spotParams;\n";        // x=exp, y=cutoff
        s += "  vec4 attenuationParams;\n"; // x=const, y=linear, z=quad
        s += "};\n";
        s += "layout(std140) uniform u_lightBlock {\n";
        s += "  LightData light[8];\n";
        s += "};\n";
        // Per-vertex Phong lighting function
        s += "vec4 calcLight(int i, vec3 n, vec3 vPos, vec3 vDir, inout vec3 specular) {\n";
        s += "  LightData l = light[i];\n";
        s += "  float spotExp    = l.spotParams.x;\n";
        s += "  float spotCutoff = l.spotParams.y;\n";
        s += "  vec3 lDir;\n";
        s += "  float att = 1.0;\n";
        s += "  if(l.position.w == 0.0) {\n";
        s += "    lDir = normalize(l.position.xyz);\n";
        s += "  } else {\n";
        s += "    vec3 v = l.position.xyz / l.position.w - vPos;\n";
        s += "    float d = length(v);\n";
        s += "    lDir = v / d;\n";
        s += "    att = 1.0 / (l.attenuationParams.x\n";
        s += "               + l.attenuationParams.y * d\n";
        s += "               + l.attenuationParams.z * d * d);\n";
        s += "  }\n";
        s += "  float NdotL = max(dot(n, lDir), 0.0);\n";
        s += "  vec4 diff = matDiffuse * l.diffuse * NdotL;\n";
        s += "  vec4 amb  = matAmbient * l.ambient;\n";
        s += "  vec4 spec = vec4(0.0);\n";
        s += "  if(spotCutoff < 180.0) {\n";
        s += "    float sCos = dot(-lDir, normalize(l.spotDir.xyz));\n";
        s += "    if(sCos < cos(radians(spotCutoff))) att = 0.0;\n";
        s += "    else att *= spotExp == 0.0 ? 1.0 : pow(max(sCos, 0.0), spotExp);\n";
        s += "  }\n";
        s += "  if(NdotL > 0.0) {\n";
        s += "    vec3 halfV = normalize(lDir + vDir);\n";
        s += "    float shine = shininess == 0.0 ? 1.0 : pow(max(dot(n, halfV), 0.0), shininess);\n";
        s += "    spec = matSpecular * l.specular * shine;\n";
        s += "  }\n";
        s += "  specular += spec.rgb * att;\n";
        s += "  return (amb + diff) * att;\n";
        s += "}\n";
    }
    
    // Body
    s += "void main() {\n";
    s += "  vec4 viewPos = u_modelview * a_position;\n";
    s += "  v_position   = u_proj * viewPos;\n";
    s += "  v_eyePos     = viewPos;\n";
    s += "  gl_Position = v_position;\n";
    s += "  float pointDistance = length(viewPos.xyz / viewPos.w);\n";
    s += "  float pointDivisor = dot(u_pointAttenuation, vec3(1.0, pointDistance, pointDistance * pointDistance));\n";
    s += "  gl_PointSize = clamp(u_pointSize * inversesqrt(pointDivisor), u_pointLimits.x, u_pointLimits.y);\n";
    s += "  v_secondaryColor = a_secondaryColor;\n";
    if(FL(SF_FOG_EXP2) || FL(SF_FOG_LINEAR) || FL(SF_FOG_EXP))
    {
        s += globals->ff.fogSource == 0x8451 ? "  v_eyeDepth = a_fogCoord;\n" : "  v_eyeDepth = abs(viewPos.z);\n";
    }
    if(FL(SF_LIGHTING))
    {
        if(globals->settings.matrix_transpose_invector)
        {
            s += "  mat3 normalMatrix = transpose(inverse(mat3(u_modelview)));\n";
        }
        else
        {
            s += "  mat3 normalMatrix = u_normal;\n";
        }
        s += "  vec3 n = normalMatrix * a_normal;\n";
        if(globals->ff.rescaleNormalEnabled) s += "  n /= length(normalMatrix[2]);\n";
        if(FL(SF_NORMALIZE))
        {
            s += "  n = normalize(n);\n";
        }
        s += globals->ff.lightModelLocalViewer ? "  vec3 vDir = normalize(-viewPos.xyz / viewPos.w);\n" : "  vec3 vDir = vec3(0.0, 0.0, 1.0);\n";
        for(int side = 0; side < (FL(SF_TWOSIDE) ? 2 : 1); ++side)
        {
            std::string index = "[" + std::to_string(side) + "]";
            s += "  { matAmbient = u_matAmbient" + index + "; matDiffuse = u_matDiffuse" + index + ";\n";
            s += "  matSpecular = u_matSpecular" + index + "; matEmission = u_matEmission" + index + ";\n";
            s += "  shininess = u_shininess" + index + ";\n";
            if(FL(SF_MATERIAL_ARRAY))
            {
                s += "  matAmbient = materialValue(" + std::to_string(side * 5) + ");\n";
                s += "  matDiffuse = materialValue(" + std::to_string(side * 5 + 1) + ");\n";
                s += "  matSpecular = materialValue(" + std::to_string(side * 5 + 2) + ");\n";
                s += "  matEmission = materialValue(" + std::to_string(side * 5 + 3) + ");\n";
                s += "  shininess = materialValue(" + std::to_string(side * 5 + 4) + ").x;\n";
            }
            if(FL(SF_COLOR_MAT) && globals->ff.colorMaterialFace != (side ? GL_FRONT : GL_BACK))
            {
                GLenum mode = globals->ff.colorMaterialMode;
                if(mode == GL_AMBIENT || mode == GL_AMBIENT_AND_DIFFUSE) s += "  matAmbient = clamp(a_color, 0.0, 1.0);\n";
                if(mode == GL_DIFFUSE || mode == GL_AMBIENT_AND_DIFFUSE) s += "  matDiffuse = clamp(a_color, 0.0, 1.0);\n";
                if(mode == GL_SPECULAR) s += "  matSpecular = clamp(a_color, 0.0, 1.0);\n";
                if(mode == GL_EMISSION) s += "  matEmission = clamp(a_color, 0.0, 1.0);\n";
            }
            s += "  vec4 finalLight = u_ambientColor * matAmbient + matEmission;\n";
            s += "  vec3 specular = vec3(0.0);\n";
            for(int light = 0; light < 8; ++light)
                if(FL(SF_LIGHT0 << light)) s += "  finalLight += calcLight(" + std::to_string(light) + (side ? ", -n" : ", n") + ", viewPos.xyz / viewPos.w, vDir, specular);\n";
            std::string color = side ? "v_backColor" : "v_color", secondary = side ? "v_backSecondaryColor" : "v_secondaryColor";
            if(globals->ff.lightModelColorControl == 0x81FA) s += "  " + secondary + " = specular;\n";
            else s += "  finalLight.rgb += specular;\n  " + secondary + " = vec3(0.0);\n";
            s += "  " + color + " = vec4(finalLight.rgb, matDiffuse.a); }\n";
        }
    }
    else
    {
        s += "  v_color = a_color;\n";
    }
    if(FL(SF_CLAMP_VERTEX))
    {
        s += "  v_color = clamp(v_color, 0.0, 1.0); v_secondaryColor = clamp(v_secondaryColor, 0.0, 1.0);\n";
        if(FL(SF_TWOSIDE)) s += "  v_backColor = clamp(v_backColor, 0.0, 1.0); v_backSecondaryColor = clamp(v_backSecondaryColor, 0.0, 1.0);\n";
    }
    bool normalTexgen = false, reflectionTexgen = false, sphereTexgen = false;
    for(const auto& unit : globals->ff.texGen) for(const auto& gen : unit)
    {
        if(!gen.enabled) continue;
        normalTexgen |= gen.mode == 0x8511 || gen.mode == 0x8512 || gen.mode == 0x2402;
        reflectionTexgen |= gen.mode == 0x8512 || gen.mode == 0x2402;
        sphereTexgen |= gen.mode == 0x2402;
    }
    if(normalTexgen)
    {
        s += "  mat3 texgenNormalMatrix = transpose(inverse(mat3(u_modelview)));\n";
        s += "  vec3 texgenNormal = texgenNormalMatrix * a_normal;\n";
        if(globals->ff.rescaleNormalEnabled) s += "  texgenNormal /= length(texgenNormalMatrix[2]);\n";
        if(FL(SF_NORMALIZE)) s += "  texgenNormal = normalize(texgenNormal);\n";
    }
    if(reflectionTexgen) s += "  vec3 texgenReflection = reflect(normalize(viewPos.xyz / viewPos.w), texgenNormal);\n";
    if(sphereTexgen) s += "  vec2 texgenSphere = texgenReflection.xy / (2.0 * length(texgenReflection + vec3(0.0,0.0,1.0))) + vec2(0.5);\n";
    for(int unit = 0; unit < 8; ++unit)
    {
        if(unit && !FL(SF_TEXUNIT1 << (unit-1))) continue;
        s += "  { vec4 coords = a_texCoord" + (unit ? std::to_string(unit) : "") + ";\n";
        for(int c = 0; c < 4; ++c)
        {
            const auto& gen = globals->ff.texGen[unit][c];
            if(!gen.enabled) continue;
            std::string component(1, "xyzw"[c]), index = std::to_string(unit*4+c);
            if(gen.mode == 0x2401) s += "    coords." + component + " = dot(a_position, u_objectPlanes[" + index + "]);\n";
            else if(gen.mode == 0x2400) s += "    coords." + component + " = dot(viewPos, u_eyePlanes[" + index + "]);\n";
            else if(gen.mode == 0x8511) s += "    coords." + component + " = texgenNormal." + component + ";\n";
            else if(gen.mode == 0x8512) s += "    coords." + component + " = texgenReflection." + component + ";\n";
            else if(gen.mode == 0x2402) s += "    coords." + component + " = texgenSphere." + component + ";\n";
        }
        s += "    " + (unit ? "v_texCoords[" + std::to_string(unit-1) + "]" : "v_texCoord") + " = u_texMatrix[" + std::to_string(unit) + "] * coords; }\n";
    }
    s += "}\n";
    return s;
}

static std::string ClampFragment(const std::string& value)
{
    return FL(SF_CLAMP_FRAGMENT) ? "clamp(" + value + ", 0.0, 1.0)" : value;
}

static std::string TextureSource(GLenum source, int unit)
{
    if(source == GL_TEXTURE) return ClampFragment("tex" + std::to_string(unit));
    if(source >= GL_TEXTURE0 && source < GL_TEXTURE0 + 8) return ClampFragment("tex" + std::to_string(source - GL_TEXTURE0));
    if(source == 0x8576) return ClampFragment(unit ? "u_texColor[" + std::to_string(unit-1) + "]" : "u_texColor0");
    if(source == 0x8577) return ClampFragment("primaryColor");
    return ClampFragment("finalColor");
}

static std::string TextureArgument(GLenum source, GLenum operand, int unit, bool alpha)
{
    std::string value = TextureSource(source, unit);
    value += alpha ? ".a" : (operand == GL_SRC_COLOR || operand == GL_ONE_MINUS_SRC_COLOR) ? ".rgb" : ".aaa";
    if(operand == GL_ONE_MINUS_SRC_COLOR || operand == GL_ONE_MINUS_SRC_ALPHA) value = "(1.0 - " + value + ")";
    return value;
}

static std::string TextureCombine(GLenum mode, const std::string* args)
{
    if(mode == GL_REPLACE) return args[0];
    if(mode == GL_MODULATE) return args[0] + " * " + args[1];
    if(mode == GL_ADD) return args[0] + " + " + args[1];
    if(mode == GL_SUBTRACT) return args[0] + " - " + args[1];
    if(mode == 0x8574) return args[0] + " + " + args[1] + " - 0.5";
    if(mode == 0x8575) return "mix(" + args[1] + ", " + args[0] + ", " + args[2] + ")";
    return "vec3(4.0 * dot(" + args[0] + " - 0.5, " + args[1] + " - 0.5))";
}

static std::string BuildTextureEnv(int unit)
{
    const auto& env = globals->ff.texEnv[unit];
    std::string index = std::to_string(unit), tex = ClampFragment("tex" + index);
    std::string color = TextureSource(0x8576, unit);
    if(env.mode == 0x8570)
    {
        std::string rgb[3], alpha[3];
        for(int i = 0; i < 3; ++i)
        {
            rgb[i] = TextureArgument(env.sourceRGB[i], env.operandRGB[i], unit, false);
            alpha[i] = TextureArgument(env.sourceAlpha[i], env.operandAlpha[i], unit, true);
        }
        std::string result = "  { vec3 rgb = " + ClampFragment("(" + TextureCombine(env.combineRGB, rgb) + ") * " + std::to_string(env.scaleRGB) + ".0") + ";\n";
        std::string a = env.combineRGB == 0x86AF ? "rgb.r" : ClampFragment("(" + TextureCombine(env.combineAlpha, alpha) + ") * " + std::to_string(env.scaleAlpha) + ".0");
        return result + "    finalColor = vec4(rgb, " + a + "); }\n";
    }
    std::string result = "  { finalColor = " + ClampFragment("finalColor") + "; vec4 previous = finalColor;\n";
    if(env.mode == GL_REPLACE) result += "    finalColor = " + tex + ";\n";
    else if(env.mode == GL_MODULATE) result += "    finalColor *= " + tex + ";\n";
    else if(env.mode == GL_ADD) result += "    finalColor = vec4(finalColor.rgb + " + tex + ".rgb, finalColor.a * " + tex + ".a);\n";
    else if(env.mode == GL_BLEND) result += "    finalColor = vec4(mix(finalColor.rgb, " + color + ".rgb, " + tex + ".rgb), finalColor.a * " + tex + ".a);\n";
    else if(env.mode == GL_DECAL) result += "    finalColor.rgb = mix(finalColor.rgb, " + tex + ".rgb, " + tex + ".a);\n";
    result += "    if(u_alphaOnly[" + index + "] == 1) finalColor.rgb = previous.rgb;\n";
    result += "    if(u_alphaOnly[" + index + "] == 2) finalColor.a = previous.a;\n";
    if(env.mode == GL_ADD) result += "    if(u_alphaOnly[" + index + "] == 3) finalColor.a = previous.a + " + tex + ".a;\n";
    if(env.mode == GL_BLEND) result += "    if(u_alphaOnly[" + index + "] == 3) finalColor.a = mix(previous.a, " + color + ".a, " + tex + ".a);\n";
    return result + "    finalColor = " + ClampFragment("finalColor") + "; }\n";
}

std::string BuildFragmentShader()
{
    std::string pointCoords = globals->ff.pointOrigin == 0x8CA2 ? "gl_PointCoord" : "vec2(gl_PointCoord.x, 1.0 - gl_PointCoord.y)";
    // Header
    std::string s = "#version 300 es\nprecision highp float;\n";
    s += FL(SF_FLATSHADING) ? "flat in vec3 v_secondaryColor;\n" : "in vec3 v_secondaryColor;\n";
    if(FL(SF_TWOSIDE))
    {
        s += FL(SF_FLATSHADING) ? "flat in lowp vec4 v_backColor;\nflat in vec3 v_backSecondaryColor;\n" : "in lowp vec4 v_backColor;\nin vec3 v_backSecondaryColor;\n";
    }
    if(FL(SF_FLATSHADING))
    {
        s += "flat in lowp vec4 v_color;\n";
    }
    else
    {
        s += "in lowp vec4 v_color;\n";
    }
    if(FL(SF_AFFINE))
    {
        s += "in vec4 v_texCoord;\n";
    }
    else
    {
        s += "in vec4 v_texCoord;\n";
    }
    s += "in vec4 v_position;\n";
    s += "in vec4 v_eyePos;\n";
    s += "uniform sampler2D u_texture;\nuniform int u_texMode0;\nuniform vec4 u_texColor0;\n";
    s += "uniform bool u_pointSprite;\n";
    s += "out lowp vec4 out_FragColor;\n";
    s += "lowp vec4 finalColor;\n";
    if(FL(SF_FOG_EXP2) || FL(SF_FOG_LINEAR) || FL(SF_FOG_EXP))
    {
        s += "in float v_eyeDepth;\n";
        s += "uniform vec4 u_fogColor;\n";
        s += "uniform vec3 u_fogValues;\n"; // x=start, y=end, z=density
        s += "float getFogValue() {\n";
        if(FL(SF_FOG_LINEAR))
        {
            s += "  float factor = (u_fogValues.y - v_eyeDepth) / (u_fogValues.y - u_fogValues.x);\n";
        }
        else if(FL(SF_FOG_EXP))
        {
            s += "  float factor = exp(-u_fogValues.z * v_eyeDepth);\n";
        }
        else // SF_FOG_EXP2
        {
            s += "  float factor = exp(-pow(u_fogValues.z * v_eyeDepth, 2.0));\n";
        }
        s += "  return clamp(factor, 0.0, 1.0);\n";
        s += "}\n";
    }
    s += "uniform int u_alphaOnly[8];\n";
    if(FL(SF_TEXUNIT1) || FL(SF_TEXUNIT2) || FL(SF_TEXUNIT3) || FL(SF_TEXUNIT4) ||
        FL(SF_TEXUNIT5) || FL(SF_TEXUNIT6) || FL(SF_TEXUNIT7))
    {
        s += "in vec4 v_texCoords[7];\n";
        s += "uniform vec4 u_texColor[7];\n";
        s += "uniform int  u_texMode[7];\n";
        s += "uniform sampler2D u_texId[7];\n";
        s += "vec4 GLIN_sampleUnit(int unit) {\n";
        for(int i = 0; i < 7; ++i)
        {
            std::string index = std::to_string(i);
            std::string sample = "textureProj(u_texId[" + index + "], v_texCoords[" + index + "])";
            if(globals->ff.pointCoordReplace[i+1]) sample = "(u_pointSprite ? texture(u_texId[" + index + "], " + pointCoords + ") : " + sample + ")";
            s += "if(unit == " + index + ") return " + sample + ";\n";
        }
        s += "return vec4(1.0); }\n";
    }
    // Alpha test helper
    if(FL(SF_ALPHATEST))
    {
        s += "uniform float u_alphaRef;\n";
        s += "uniform int   u_alphaFunc;\n"; // GL enum values mapped to 0-7
        // 0=NEVER,1=LESS,2=EQUAL,3=LEQUAL,4=GREATER,5=NOTEQUAL,6=GEQUAL,7=ALWAYS
        s += "bool alphaTest(float a) {\n";
        s += "  if(u_alphaFunc == 0) return false;\n"; // GL_NEVER
        s += "  if(u_alphaFunc == 7) return true;\n";  // GL_ALWAYS
        s += "  if(u_alphaFunc == 1) return a <  u_alphaRef;\n"; // GL_LESS
        s += "  if(u_alphaFunc == 2) return abs(a - u_alphaRef) < 0.001;\n"; // GL_EQUAL
        s += "  if(u_alphaFunc == 3) return a <= u_alphaRef;\n"; // GL_LEQUAL
        s += "  if(u_alphaFunc == 4) return a >  u_alphaRef;\n"; // GL_GREATER
        s += "  if(u_alphaFunc == 5) return abs(a - u_alphaRef) >= 0.001;\n"; // GL_NOTEQUAL
        s += "  if(u_alphaFunc == 6) return a >= u_alphaRef;\n"; // GL_GEQUAL
        s += "  return true;\n";
        s += "}\n";
    }
    if(FL(SF_CLIPPLANE1) || FL(SF_CLIPPLANE2) || FL(SF_CLIPPLANE3) ||
        FL(SF_CLIPPLANE4) || FL(SF_CLIPPLANE5) || FL(SF_CLIPPLANE6))
    {
        s += "uniform vec4 u_clipPlane[6];\n";
    }
    
    // Body
    s += "void main() {\n";
    if(FL(SF_CLIPPLANE1)) s += "  if(dot(v_eyePos, u_clipPlane[0]) < 0.0) discard;\n";
    if(FL(SF_CLIPPLANE2)) s += "  if(dot(v_eyePos, u_clipPlane[1]) < 0.0) discard;\n";
    if(FL(SF_CLIPPLANE3)) s += "  if(dot(v_eyePos, u_clipPlane[2]) < 0.0) discard;\n";
    if(FL(SF_CLIPPLANE4)) s += "  if(dot(v_eyePos, u_clipPlane[3]) < 0.0) discard;\n";
    if(FL(SF_CLIPPLANE5)) s += "  if(dot(v_eyePos, u_clipPlane[4]) < 0.0) discard;\n";
    if(FL(SF_CLIPPLANE6)) s += "  if(dot(v_eyePos, u_clipPlane[5]) < 0.0) discard;\n";
    for(int i = 0; i < 8; ++i)
    {
        bool enabled = i == 0 ? FL(SF_TEXTURED) : FL(SF_TEXUNIT1 << (i-1));
        std::string sample = i ? "GLIN_sampleUnit(" + std::to_string(i-1) + ")" : "textureProj(u_texture, v_texCoord)";
        if(!i && globals->ff.pointCoordReplace[0]) sample = "(u_pointSprite ? texture(u_texture, " + pointCoords + ") : " + sample + ")";
        s += "  vec4 tex" + std::to_string(i) + " = " + (enabled ? sample : "vec4(1.0)") + ";\n";
    }
    s += FL(SF_TWOSIDE) ? "  vec4 primaryColor = gl_FrontFacing ? v_color : v_backColor;\n  vec3 secondaryColor = gl_FrontFacing ? v_secondaryColor : v_backSecondaryColor;\n" : "  vec4 primaryColor = v_color;\n  vec3 secondaryColor = v_secondaryColor;\n";
    s += "  finalColor = primaryColor;\n";
    for(int i = 0; i < 8; ++i)
        if(i == 0 ? FL(SF_TEXTURED) : FL(SF_TEXUNIT1 << (i-1))) s += BuildTextureEnv(i);
    if(FL(SF_LIGHTING) ? globals->ff.lightModelColorControl == 0x81FA : globals->ff.colorSum) s += "  finalColor.rgb += secondaryColor;\n";
    s += "  finalColor = " + ClampFragment("finalColor") + ";\n";
    if(FL(SF_FOG_EXP2) || FL(SF_FOG_LINEAR) || FL(SF_FOG_EXP))
    {
        s += "  finalColor.rgb = " + ClampFragment("mix(" + ClampFragment("u_fogColor.rgb") + ", finalColor.rgb, getFogValue())") + ";\n";
    }
    // Alpha test — discard if test fails
    if(FL(SF_ALPHATEST))
    {
        s += "  if(!alphaTest(finalColor.a)) discard;\n";
    }
    s += "  out_FragColor = finalColor;\n";
    s += "}\n";
    return s;
}

unsigned int BuildFixedProgram()
{
    std::string vertexS = BuildVertexShader();
    std::string fragS   = BuildFragmentShader();
    
    const char* vs = vertexS.c_str();
    const char* fs = fragS.c_str();

    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vs, 0);
    glCompileShader(vertex);

    // Log vertex shader errors
    {
        GLint status = 0;
        glGetShaderiv(vertex, GL_COMPILE_STATUS, &status);
        if(status == GL_FALSE)
        {
            char log[2048]; GLsizei len;
            glGetShaderInfoLog(vertex, sizeof(log), &len, log);
            ERR("[Fixed VS] Compile error: %s", log);
        }
    }

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fs, 0);
    glCompileShader(fragment);

    // Log fragment shader errors
    {
        GLint status = 0;
        glGetShaderiv(fragment, GL_COMPILE_STATUS, &status);
        if(status == GL_FALSE)
        {
            char log[2048]; GLsizei len;
            glGetShaderInfoLog(fragment, sizeof(log), &len, log);
            ERR("[Fixed FS] Compile error: %s", log);
        }
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    {
        GLint status = 0;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if(status == GL_FALSE)
        {
            char log[2048]; GLsizei len;
            glGetProgramInfoLog(program, sizeof(log), &len, log);
            ERR("[Fixed Prog] Link error: %s", log);
        }
    }

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}

// Encode GL alpha-func enum to 0-7 int for the shader
static inline int AlphaFuncToInt(GLenum f)
{
    switch(f)
    {
        case GL_NEVER:    return 0;
        case GL_LESS:     return 1;
        case GL_EQUAL:    return 2;
        case GL_LEQUAL:   return 3;
        case GL_GREATER:  return 4;
        case GL_NOTEQUAL: return 5;
        case GL_GEQUAL:   return 6;
        case GL_ALWAYS:   return 7;
        default:          return 7;
    }
}

void UseFixedProgram(GLenum mode)
{
    GLIN_InitExtensions();
    // If the app has its own shader active, don't override it.
    if(globals->gl.activeProgram != 0)
    {
        GLuint program = globals->gl.activeProgram;
        glUseProgram(program);
        matrix4_t mvp = globals->matrix.projection.Current() * globals->matrix.modelview.Current();
        glUniformMatrix4fv(glGetUniformLocation(program, "GLIN_ModelViewProjectionMatrix"), 1, GL_FALSE, mvp.m);
        glUniformMatrix4fv(glGetUniformLocation(program, "GLIN_ModelViewMatrix"), 1, GL_FALSE, globals->matrix.modelview.Current().m);
        glUniformMatrix4fv(glGetUniformLocation(program, "GLIN_ProjectionMatrix"), 1, GL_FALSE, globals->matrix.projection.Current().m);
        matrix3_t normal = GetNormalMatrix(globals->matrix.modelview.Current().m);
        glUniformMatrix3fv(glGetUniformLocation(program, "GLIN_NormalMatrix"), 1, GL_FALSE, normal.m);
        return;
    }
    
    BuildShaderFlag();
    
    std::string key = std::to_string(g_nFixedPipelineShaderFlags);
    key += ":" + std::to_string(globals->ff.fogSource) + ":" + std::to_string(globals->ff.colorSum);
    key += ":" + std::to_string(globals->ff.pointOrigin);
    key += ":" + std::to_string(globals->ff.rescaleNormalEnabled);
    key += ":" + std::to_string(globals->ff.lightModelColorControl) + ":" + std::to_string(globals->ff.lightModelLocalViewer);
    key += ":" + std::to_string(globals->ff.colorMaterialFace) + ":" + std::to_string(globals->ff.colorMaterialMode);
    for(bool replace : globals->ff.pointCoordReplace) key += replace ? ":1" : ":0";
    for(const auto& unit : globals->ff.texGen) for(const auto& gen : unit) key += ":" + std::to_string(gen.enabled) + ":" + std::to_string(gen.mode);
    for(const auto& env : globals->ff.texEnv)
    {
        key += ":" + std::to_string(env.mode) + ":" + std::to_string(env.combineRGB) + ":" + std::to_string(env.combineAlpha);
        key += ":" + std::to_string(env.scaleRGB) + ":" + std::to_string(env.scaleAlpha);
        for(int i = 0; i < 3; ++i)
            key += ":" + std::to_string(env.sourceRGB[i]) + ":" + std::to_string(env.sourceAlpha[i]) + ":" + std::to_string(env.operandRGB[i]) + ":" + std::to_string(env.operandAlpha[i]);
    }
    auto it = g_mapFixedPrograms.find(key);
    if(it != g_mapFixedPrograms.end())
    {
        activeFixedProgram = it->second.get();
        g_nUberShader = activeFixedProgram->program;
    }
    else
    {
        g_nUberShader = BuildFixedProgram();
        
        fixed_program_t* program = new fixed_program_t;
        program->program = g_nUberShader;
        
        activeFixedProgram = program;
        g_mapFixedPrograms.emplace(key, std::shared_ptr<fixed_program_t>(program));
        
        program->uModelView.id    = glGetUniformLocation(g_nUberShader, "u_modelview");
        program->uProj.id         = glGetUniformLocation(g_nUberShader, "u_proj");
        program->uNormal.id       = glGetUniformLocation(g_nUberShader, "u_normal");
        program->uDiffuse.id      = glGetUniformLocation(g_nUberShader, "u_texture");
        program->uFogColor.id     = glGetUniformLocation(g_nUberShader, "u_fogColor");
        program->uFogValues.id    = glGetUniformLocation(g_nUberShader, "u_fogValues");
        program->uAmbientColor.id = glGetUniformLocation(g_nUberShader, "u_ambientColor");
        program->uTexCoords.id    = glGetUniformLocation(g_nUberShader, "u_texCoords");
        program->uTexColors.id    = glGetUniformLocation(g_nUberShader, "u_texColor");
        program->uAlphaOnly.id = glGetUniformLocation(g_nUberShader, "u_alphaOnly");
        program->uTexModes.id     = glGetUniformLocation(g_nUberShader, "u_texMode");
        program->uTexIDs.id       = glGetUniformLocation(g_nUberShader, "u_texId");
        program->uPointSize.id = glGetUniformLocation(g_nUberShader, "u_pointSize");
        program->uPointLimits.id = glGetUniformLocation(g_nUberShader, "u_pointLimits");
        program->uPointAttenuation.id = glGetUniformLocation(g_nUberShader, "u_pointAttenuation");
        program->uPointSprite.id = glGetUniformLocation(g_nUberShader, "u_pointSprite");
        program->uObjectPlanes.id = glGetUniformLocation(g_nUberShader, "u_objectPlanes");
        program->uEyePlanes.id = glGetUniformLocation(g_nUberShader, "u_eyePlanes");
        program->uTexMatrix.id = glGetUniformLocation(g_nUberShader, "u_texMatrix");
        program->uTexMode0.id = glGetUniformLocation(g_nUberShader, "u_texMode0");
        program->uTexColor0.id = glGetUniformLocation(g_nUberShader, "u_texColor0");
        program->uMaterialValues.id = glGetUniformLocation(g_nUberShader, "u_materialValues");
        program->uMaterialWidth.id = glGetUniformLocation(g_nUberShader, "u_materialWidth");
        for(int side = 0; side < 2; ++side)
        {
            std::string index = "[" + std::to_string(side) + "]";
            program->uShininess[side].id = glGetUniformLocation(g_nUberShader, ("u_shininess" + index).c_str());
            program->uMatAmbient[side].id = glGetUniformLocation(g_nUberShader, ("u_matAmbient" + index).c_str());
            program->uMatDiffuse[side].id = glGetUniformLocation(g_nUberShader, ("u_matDiffuse" + index).c_str());
            program->uMatSpecular[side].id = glGetUniformLocation(g_nUberShader, ("u_matSpecular" + index).c_str());
            program->uMatEmission[side].id = glGetUniformLocation(g_nUberShader, ("u_matEmission" + index).c_str());
        }
        program->uAlphaRef.id     = glGetUniformLocation(g_nUberShader, "u_alphaRef");
        program->uAlphaFunc.id    = glGetUniformLocation(g_nUberShader, "u_alphaFunc");
        program->uClipPlanes.id   = glGetUniformLocation(g_nUberShader, "u_clipPlane");
        
        if(FL(SF_LIGHT0) || FL(SF_LIGHT1) || FL(SF_LIGHT2) || FL(SF_LIGHT3) || 
            FL(SF_LIGHT4) || FL(SF_LIGHT5) || FL(SF_LIGHT6) || FL(SF_LIGHT7))
        {
            program->uLights.Init();
        }
    }
    
    glUseProgram(g_nUberShader);
    
    activeFixedProgram->uModelView.Apply(globals->matrix.modelview.Current());
    activeFixedProgram->uProj.Apply(globals->matrix.projection.Current());
    activeFixedProgram->uNormal.Apply(GetNormalMatrix(globals->matrix.modelview.Current().m), false);
    activeFixedProgram->uDiffuse.Apply(0);
    activeFixedProgram->uPointSize.Apply(globals->ff.pointSize);
    activeFixedProgram->uPointSprite.Apply((int)(mode == GL_POINTS && globals->ff.pointSprite));
    GLfloat objectPlanes[32][4], eyePlanes[32][4];
    for(int unit = 0; unit < 8; ++unit) for(int c = 0; c < 4; ++c)
    {
        memcpy(objectPlanes[unit*4+c], globals->ff.texGen[unit][c].objectPlane, sizeof(objectPlanes[0]));
        memcpy(eyePlanes[unit*4+c], globals->ff.texGen[unit][c].eyePlane, sizeof(eyePlanes[0]));
    }
    glUniform4fv(activeFixedProgram->uObjectPlanes.id, 32, objectPlanes[0]);
    glUniform4fv(activeFixedProgram->uEyePlanes.id, 32, eyePlanes[0]);
    activeFixedProgram->uPointLimits.Apply(vector2_t{globals->ff.pointMin, globals->ff.pointMax});
    activeFixedProgram->uPointAttenuation.Apply(vector3_t{globals->ff.pointAttenuation[0], globals->ff.pointAttenuation[1], globals->ff.pointAttenuation[2]});
    matrix4_t textureMatrices[8];
    textureMatrices[0] = globals->matrix.texture.Current();
    for(int i = 1; i < 8; ++i) textureMatrices[i] = globals->matrix.textureUnits[i-1].Current();
    glUniformMatrix4fv(activeFixedProgram->uTexMatrix.id, 8, GL_FALSE, textureMatrices[0].m);
    GLint previousUnit = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousUnit);
    int alphaOnly[8] = {};
    for(int i = 0; i < 8; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        GLint binding = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding);
        auto it = globals->textures.find((GLuint)binding);
        GLenum format = it != globals->textures.end() ? it->second->baseFormat : GL_RGBA;
        alphaOnly[i] = format == GL_ALPHA ? 1 : (format == GL_RGB || format == GL_LUMINANCE || format == 1 || format == 3 || format == GL_RGB8 || format == GL_SRGB8) ? 2 : format == 0x8049 ? 3 : 0;
    }
    glActiveTexture(previousUnit);
    activeFixedProgram->uAlphaOnly.Apply(alphaOnly, 8);
    activeFixedProgram->uTexMode0.Apply((int)globals->ff.texEnv[0].mode);
    activeFixedProgram->uTexColor0.Apply(globals->ff.texEnv[0].color);
    activeFixedProgram->uFogColor.Apply(globals->ff.fogColor);
    activeFixedProgram->uFogValues.Apply(vector3_t{globals->ff.fogStart, globals->ff.fogEnd, globals->ff.fogDensity});
    activeFixedProgram->uAmbientColor.Apply(globals->render.ambient);
    activeFixedProgram->uMaterialValues.Apply(8);
    activeFixedProgram->uMaterialWidth.Apply(globals->render.materialWidth);
    activeFixedProgram->uLights.Apply(globals->ff.lights);
    for(int side = 0; side < 2; ++side)
    {
        const auto& material = globals->ff.materials[side];
        activeFixedProgram->uShininess[side].Apply(material.shininess);
        activeFixedProgram->uMatAmbient[side].Apply(*(const vector4_t*)material.ambient);
        activeFixedProgram->uMatDiffuse[side].Apply(*(const vector4_t*)material.diffuse);
        activeFixedProgram->uMatSpecular[side].Apply(*(const vector4_t*)material.specular);
        activeFixedProgram->uMatEmission[side].Apply(*(const vector4_t*)material.emission);
    }
    // Alpha test uniforms
    if(FL(SF_ALPHATEST))
    {
        activeFixedProgram->uAlphaRef.Apply(globals->ff.alphaTestRef);
        activeFixedProgram->uAlphaFunc.Apply(AlphaFuncToInt(globals->ff.alphaTestFunc));
    }
    if(FL(SF_CLIPPLANE1) || FL(SF_CLIPPLANE2) || FL(SF_CLIPPLANE3) ||
        FL(SF_CLIPPLANE4) || FL(SF_CLIPPLANE5) || FL(SF_CLIPPLANE6))
    {
        if(activeFixedProgram->uClipPlanes.id != -1)
        {
            glUniform4fv(activeFixedProgram->uClipPlanes.id, 6, (const GLfloat*)globals->ff.clipPlanes);
        }
    }
    // TODO: per-unit texture uniforms (u_texColor, u_texMode, u_texId)
    if(FL(SF_TEXUNIT1) || FL(SF_TEXUNIT2) || FL(SF_TEXUNIT3) || FL(SF_TEXUNIT4) ||
       FL(SF_TEXUNIT5) || FL(SF_TEXUNIT6) || FL(SF_TEXUNIT7))
    {
        int modes[7]    = {0};
        const int samplers[7] = {1, 2, 3, 4, 5, 6, 7};
        for(int i = 0; i < 7; ++i)
        {
            modes[i] = globals->ff.texEnv[i+1].mode;
        }
        vector4_t colors[7];
        for(int i = 0; i < 7; ++i) colors[i] = globals->ff.texEnv[i+1].color;
        if(activeFixedProgram->uTexColors.id != -1) glUniform4fv(activeFixedProgram->uTexColors.id, 7, &colors[0].x);
        activeFixedProgram->uTexModes.Apply(modes, 7);
        activeFixedProgram->uTexIDs.Apply(samplers, 7);
    }
}

void TransformFixedVerts()
{
    if(globals->render.vertices.empty()) return;
    auto& render = globals->render;
    GLuint materialTexture = 0;
    GLint activeTexture = 0, textureBinding = 0, samplerBinding = 0, unpackBuffer = 0, unpack[3] = {};
    const GLenum unpackNames[] = {GL_UNPACK_ROW_LENGTH, GL_UNPACK_SKIP_ROWS, GL_UNPACK_SKIP_PIXELS};
    if(!render.materialValues.empty())
    {
        GLint maxSize;
        glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
        size_t count = render.materialValues.size();
        if(maxSize <= 0 || count > (size_t)maxSize * maxSize || count > INT32_MAX) { SetError(GL_OUT_OF_MEMORY); return; }
        GLint width = count < (size_t)maxSize ? (GLint)count : maxSize;
        GLint height = (GLint)((count + width - 1) / width);
        render.materialValues.resize((size_t)width * height);
        glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
        glActiveTexture(GL_TEXTURE0 + 8);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &textureBinding);
        glGetIntegerv(GL_SAMPLER_BINDING, &samplerBinding);
        glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &unpackBuffer);
        for(int i = 0; i < 3; ++i)
        {
            glGetIntegerv(unpackNames[i], &unpack[i]);
            glPixelStorei(unpackNames[i], 0);
        }
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        glBindSampler(8, 0);
        glGenTextures(1, &materialTexture);
        glBindTexture(GL_TEXTURE_2D, materialTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, render.materialValues.data());
        render.materialWidth = width;
        glActiveTexture(activeTexture);
    }
    client_state_t previous = globals->client;
    auto& client = globals->client;
    client.vertexArrayEnabled = true;
    client.vertexSize = 4; client.vertexType = GL_FLOAT; client.vertexStride = 0; client.vertexBuffer = 0;
    client.vertexPtr = render.vertices.data();
    client.colorArrayEnabled = true;
    client.colorSize = 4; client.colorType = GL_FLOAT; client.colorStride = 0; client.colorBuffer = 0;
    client.colorPtr = render.colors.data();
    client.normalArrayEnabled = true;
    client.normalType = GL_FLOAT; client.normalStride = 0; client.normalBuffer = 0;
    client.normalPtr = render.normals.data();
    client.secondaryColorArrayEnabled = true;
    client.secondaryColorSize = 3; client.secondaryColorType = GL_FLOAT; client.secondaryColorStride = 0; client.secondaryColorBuffer = 0;
    client.secondaryColorPtr = render.secondaryColors.data();
    client.fogCoordArrayEnabled = true;
    client.fogCoordType = GL_FLOAT; client.fogCoordStride = 0; client.fogCoordBuffer = 0;
    client.fogCoordPtr = render.fogCoords.data();
    for(int i = 0; i < 8; ++i)
    {
        auto& tex = client.texCoord[i];
        tex.enabled = true; tex.texCoordSize = 4; tex.texCoordType = GL_FLOAT;
        tex.texCoordStride = 0; tex.texCoordBuffer = 0; tex.texCoordPtr = render.texcoords[i].data();
    }
    WRAP(glDrawArrays(render.lastPrimitiveMode, 0, (GLsizei)render.vertices.size()));
    globals->client = previous;
    if(render.materialWidth)
    {
        render.materialWidth = 0;
        glActiveTexture(GL_TEXTURE0 + 8);
        glBindTexture(GL_TEXTURE_2D, textureBinding);
        glBindSampler(8, samplerBinding);
        glDeleteTextures(1, &materialTexture);
        glActiveTexture(activeTexture);
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, unpackBuffer);
        for(int i = 0; i < 3; ++i) glPixelStorei(unpackNames[i], unpack[i]);
    }
}

void TransposeMatrix(const float* src, float* dst)
{
    for(int i = 0; i < 4; i++)
    {
        for(int j = 0; j < 4; j++)
        {
            dst[i * 4 + j] = src[j * 4 + i];
        }
    }
}

void MultiplyMatrix2(const float* m, const float* v, float* out)
{
    for(int r = 0; r < 4; ++r)
    {
        out[r] = m[0*4+r]*v[0] + m[1*4+r]*v[1] + m[2*4+r]*v[2] + m[3*4+r]*v[3];
    }
}

void GetNormalMatrix(const float* mview, float* normalMat)
{
    float m00 = mview[0], m01 = mview[4], m02 = mview[8];
    float m10 = mview[1], m11 = mview[5], m12 = mview[9];
    float m20 = mview[2], m21 = mview[6], m22 = mview[10];

    float det = m00*(m11*m22 - m12*m21) - m01*(m10*m22 - m12*m20) + m02*(m10*m21 - m11*m20);
    if (det == 0.0f)
    {
        normalMat[0]=1; normalMat[1]=0; normalMat[2]=0;
        normalMat[3]=0; normalMat[4]=1; normalMat[5]=0;
        normalMat[6]=0; normalMat[7]=0; normalMat[8]=1;
        return;
    }
    const float inv = 1.0f / det;

    normalMat[0] =  (m11*m22 - m12*m21) * inv;
    normalMat[1] = -(m10*m22 - m12*m20) * inv;
    normalMat[2] =  (m10*m21 - m11*m20) * inv;

    normalMat[3] = -(m01*m22 - m02*m21) * inv;
    normalMat[4] =  (m00*m22 - m02*m20) * inv;
    normalMat[5] = -(m00*m21 - m01*m20) * inv;

    normalMat[6] =  (m01*m12 - m02*m11) * inv;
    normalMat[7] = -(m00*m12 - m02*m10) * inv;
    normalMat[8] =  (m00*m11 - m01*m10) * inv;
}

matrix3_t GetNormalMatrix(const float* mview)
{
    matrix3_t ret;
    GetNormalMatrix(mview, ret.m);
    return ret;
}
