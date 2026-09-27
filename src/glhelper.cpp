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
    fixed_uniform_t uTexMatrix;
    fixed_uniform_t uTexMode0;
    fixed_uniform_t uTexColor0;
    fixed_uniform_t uShininess;
    fixed_uniform_t uMatAmbient;
    fixed_uniform_t uMatDiffuse;
    fixed_uniform_t uMatSpecular;
    fixed_uniform_t uMatEmission;
    fixed_lights_uniform_t uLights;
    fixed_uniform_t uAlphaRef;   // alpha test ref value
    fixed_uniform_t uAlphaFunc;  // alpha test func (int enum)
    fixed_uniform_t uClipPlanes; // 6 clip planes as vec4[6]
};

#define g_mapFixedPrograms globals->fixedPrograms

inline void BuildShaderFlag()
{
    g_nFixedPipelineShaderFlags = 0;
    
    if(globals->render.texture) EFL(SF_TEXTURED);
    if(globals->ff.lightingEnabled)
    {
        EFL(SF_LIGHTING);
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
    std::string s = "#version 320 es\nprecision highp float;\n";
    s += "layout(location = 0) in vec4 a_position;\n";
    s += "layout(location = 2) in vec3 a_normal;\n";
    s += "layout(location = 3) in vec4 a_color;\n";
    s += "layout(location = 8) in vec2 a_texCoord;\n";
    s += "uniform mat4 u_modelview;\n";
    s += "uniform mat4 u_proj;\nuniform mat4 u_texMatrix;\nuniform float u_pointSize;\n";
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
        s += "out vec2 v_texCoord;\n";
    }
    else
    {
        s += "out vec2 v_texCoord;\n";
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
        s += "layout(location = 9)  in vec2 a_texCoord1;\n";
        s += "layout(location = 10) in vec2 a_texCoord2;\n";
        s += "layout(location = 11) in vec2 a_texCoord3;\n";
        s += "layout(location = 12) in vec2 a_texCoord4;\n";
        s += "layout(location = 13) in vec2 a_texCoord5;\n";
        s += "layout(location = 14) in vec2 a_texCoord6;\n";
        s += "layout(location = 15) in vec2 a_texCoord7;\n";
        s += "out vec2 v_texCoords[7];\n";
    }
    if(FL(SF_LIGHTING))
    {
        s += "uniform vec4 u_ambientColor;\n";
        s += "uniform vec4 u_matAmbient;\n";
        s += "uniform vec4 u_matDiffuse;\n";
        s += "uniform vec4 u_matSpecular;\n";
        s += "uniform vec4 u_matEmission;\n";
        s += "uniform float u_shininess;\n";
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
        s += "vec4 calcLight(int i, vec3 n, vec3 vPos, vec3 vDir) {\n";
        s += "  LightData l = light[i];\n";
        s += "  float spotExp    = l.spotParams.x;\n";
        s += "  float spotCutoff = l.spotParams.y;\n";
        s += "  vec3 lDir;\n";
        s += "  float att = 1.0;\n";
        s += "  if(l.position.w == 0.0) {\n";
        s += "    lDir = normalize(l.position.xyz);\n";
        s += "  } else {\n";
        s += "    vec3 v = l.position.xyz - vPos;\n";
        s += "    float d = length(v);\n";
        s += "    lDir = v / d;\n";
        s += "    att = 1.0 / (l.attenuationParams.x\n";
        s += "               + l.attenuationParams.y * d\n";
        s += "               + l.attenuationParams.z * d * d);\n";
        s += "  }\n";
        s += "  float NdotL = max(dot(n, lDir), 0.0);\n";
        s += "  vec4 diff = u_matDiffuse * l.diffuse * NdotL;\n";
        s += "  vec4 amb  = u_matAmbient * l.ambient;\n";
        s += "  vec4 spec = vec4(0.0);\n";
        s += "  if(NdotL > 0.0) {\n";
        s += "    float spot = 1.0;\n";
        s += "    if(spotCutoff < 180.0) {\n";
        s += "      float sCos = dot(-lDir, normalize(l.spotDir.xyz));\n";
        s += "      if(sCos < cos(radians(spotCutoff))) spot = 0.0;\n";
        s += "      else spot = pow(max(sCos, 0.0), spotExp);\n";
        s += "    }\n";
        s += "    vec3 halfV = normalize(lDir + vDir);\n";
        s += "    float sh = max(u_shininess, 1.0);\n";
        s += "    spec = u_matSpecular * l.specular * pow(max(dot(n, halfV), 0.0), sh) * spot;\n";
        s += "    diff *= spot;\n";
        s += "  }\n";
        s += "  return (amb + diff + spec) * att;\n";
        s += "}\n";
        if(FL(SF_TWOSIDE))
        {
            // Back-face lighting with negated normal
            s += "vec4 calcLightBack(int i, vec3 n, vec3 vPos, vec3 vDir) {\n";
            s += "  return calcLight(i, -n, vPos, vDir);\n";
            s += "}\n";
            s += "flat out int v_facing;\n"; // 1=front,0=back; passed to FS
        }
    }
    
    // Body
    s += "void main() {\n";
    s += "  vec4 viewPos = u_modelview * a_position;\n";
    s += "  v_position   = u_proj * viewPos;\n";
    s += "  v_eyePos     = viewPos;\n";
    s += "  gl_Position  = v_position;\n  gl_PointSize = u_pointSize;\n";
    if(FL(SF_FOG_EXP2) || FL(SF_FOG_LINEAR) || FL(SF_FOG_EXP))
    {
        s += "  v_eyeDepth = -viewPos.z;\n";
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
        s += "  vec3 rawN = a_normal;\n";
        if(FL(SF_NORMALIZE))
        {
            s += "  rawN = normalize(rawN);\n";
        }
        s += "  vec3 n = normalize(normalMatrix * rawN);\n";
        s += "  vec3 vDir = normalize(-viewPos.xyz);\n";
        s += "  lowp vec4 finalLight = u_ambientColor * u_matAmbient + u_matEmission;\n";
        if(FL(SF_LIGHT0)) s += "  finalLight += calcLight(0, n, viewPos.xyz, vDir);\n";
        if(FL(SF_LIGHT1)) s += "  finalLight += calcLight(1, n, viewPos.xyz, vDir);\n";
        if(FL(SF_LIGHT2)) s += "  finalLight += calcLight(2, n, viewPos.xyz, vDir);\n";
        if(FL(SF_LIGHT3)) s += "  finalLight += calcLight(3, n, viewPos.xyz, vDir);\n";
        if(FL(SF_LIGHT4)) s += "  finalLight += calcLight(4, n, viewPos.xyz, vDir);\n";
        if(FL(SF_LIGHT5)) s += "  finalLight += calcLight(5, n, viewPos.xyz, vDir);\n";
        if(FL(SF_LIGHT6)) s += "  finalLight += calcLight(6, n, viewPos.xyz, vDir);\n";
        if(FL(SF_LIGHT7)) s += "  finalLight += calcLight(7, n, viewPos.xyz, vDir);\n";
        s += "  v_color = clamp(finalLight, 0.0, 1.0) * a_color;\n";
        if(FL(SF_TWOSIDE))
        {
            // TODO:
        }
    }
    else
    {
        s += "  v_color = a_color;\n";
    }
    if(FL(SF_TEXUNIT1)) s += "  v_texCoords[0] = a_texCoord1;\n";
    if(FL(SF_TEXUNIT2)) s += "  v_texCoords[1] = a_texCoord2;\n";
    if(FL(SF_TEXUNIT3)) s += "  v_texCoords[2] = a_texCoord3;\n";
    if(FL(SF_TEXUNIT4)) s += "  v_texCoords[3] = a_texCoord4;\n";
    if(FL(SF_TEXUNIT5)) s += "  v_texCoords[4] = a_texCoord5;\n";
    if(FL(SF_TEXUNIT6)) s += "  v_texCoords[5] = a_texCoord6;\n";
    if(FL(SF_TEXUNIT7)) s += "  v_texCoords[6] = a_texCoord7;\n";
    s += "  v_texCoord = (u_texMatrix * vec4(a_texCoord, 0.0, 1.0)).xy;\n";
    s += "}\n";
    return s;
}

std::string BuildFragmentShader()
{
    // Header
    std::string s = "#version 320 es\nprecision highp float;\n";
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
        s += "in vec2 v_texCoord;\n";
    }
    else
    {
        s += "in vec2 v_texCoord;\n";
    }
    s += "in vec4 v_position;\n";
    s += "in vec4 v_eyePos;\n";
    s += "uniform sampler2D u_texture;\nuniform int u_texMode0;\nuniform vec4 u_texColor0;\n";
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
        s += "in vec2 v_texCoords[7];\n";
        s += "uniform vec4 u_texColor[7];\n";
        s += "uniform int  u_texMode[7];\n";
        s += "uniform sampler2D u_texId[7];\n";
        s += "vec4 GLIN_sampleUnit(int unit) {\n";
        for(int i = 0; i < 7; ++i) s += "if(unit == " + std::to_string(i) + ") return texture(u_texId[" + std::to_string(i) + "], v_texCoords[" + std::to_string(i) + "]);\n";
        s += "return vec4(1.0); }\n";
        s += "void mixUnitColor(int unit) {\n";
        s += "  int mode = u_texMode[unit];\n";
        s += "  lowp vec4 texColor = GLIN_sampleUnit(unit);\n";
        s += "  if(u_alphaOnly[unit+1] != 0) { finalColor.a = mode == 0 ? texColor.a : finalColor.a * texColor.a; return; }\n";
        s += "  if(mode == 0) { finalColor = texColor; return; }\n";              // GL_REPLACE
        s += "  if(mode == 1) { finalColor *= texColor; return; }\n";             // GL_MODULATE
        s += "  if(mode == 2) { finalColor = vec4(finalColor.rgb + texColor.rgb, finalColor.a * texColor.a); return; }\n"; // GL_ADD
        s += "  if(mode == 3) { finalColor = vec4(mix(finalColor.rgb, u_texColor[unit].rgb, texColor.rgb), finalColor.a * texColor.a); return; }\n"; // GL_BLEND
        s += "  if(mode == 4) { finalColor = vec4(mix(finalColor.rgb, texColor.rgb, texColor.a), finalColor.a); return; }\n"; // GL_DECAL
        s += "}\n";
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
    if(FL(SF_TEXTURED))
    {
        s += "  vec4 tex = texture(u_texture, v_texCoord);\n";
        s += "  finalColor = v_color * tex;\n";
        s += "  if(u_texMode0 == 0) finalColor = tex;\n";
        s += "  if(u_texMode0 == 2) finalColor = vec4(v_color.rgb + tex.rgb, v_color.a * tex.a);\n";
        s += "  if(u_texMode0 == 3) finalColor = vec4(mix(v_color.rgb, u_texColor0.rgb, tex.rgb), v_color.a * tex.a);\n";
        s += "  if(u_texMode0 == 4) finalColor = vec4(mix(v_color.rgb, tex.rgb, tex.a), v_color.a);\n";
        s += "  if(u_alphaOnly[0] != 0) finalColor = vec4(v_color.rgb, u_texMode0 == 0 ? tex.a : v_color.a * tex.a);\n";
    }
    else
    {
        s += "  finalColor = v_color;\n";
    }
    if(FL(SF_TEXUNIT1)) s += "  mixUnitColor(0);\n";
    if(FL(SF_TEXUNIT2)) s += "  mixUnitColor(1);\n";
    if(FL(SF_TEXUNIT3)) s += "  mixUnitColor(2);\n";
    if(FL(SF_TEXUNIT4)) s += "  mixUnitColor(3);\n";
    if(FL(SF_TEXUNIT5)) s += "  mixUnitColor(4);\n";
    if(FL(SF_TEXUNIT6)) s += "  mixUnitColor(5);\n";
    if(FL(SF_TEXUNIT7)) s += "  mixUnitColor(6);\n";
    if(FL(SF_FOG_EXP2) || FL(SF_FOG_LINEAR) || FL(SF_FOG_EXP))
    {
        s += "  finalColor.rgb = mix(u_fogColor.rgb, finalColor.rgb, getFogValue());\n";
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

void UseFixedProgram()
{
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
    
    auto it = g_mapFixedPrograms.find(g_nFixedPipelineShaderFlags);
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
        g_mapFixedPrograms.emplace(g_nFixedPipelineShaderFlags, std::shared_ptr<fixed_program_t>(program));
        
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
        program->uTexMatrix.id = glGetUniformLocation(g_nUberShader, "u_texMatrix");
        program->uTexMode0.id = glGetUniformLocation(g_nUberShader, "u_texMode0");
        program->uTexColor0.id = glGetUniformLocation(g_nUberShader, "u_texColor0");
        program->uShininess.id    = glGetUniformLocation(g_nUberShader, "u_shininess");
        program->uMatAmbient.id   = glGetUniformLocation(g_nUberShader, "u_matAmbient");
        program->uMatDiffuse.id   = glGetUniformLocation(g_nUberShader, "u_matDiffuse");
        program->uMatSpecular.id  = glGetUniformLocation(g_nUberShader, "u_matSpecular");
        program->uMatEmission.id  = glGetUniformLocation(g_nUberShader, "u_matEmission");
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
    activeFixedProgram->uTexMatrix.Apply(globals->matrix.texture.Current());
    GLint previousUnit = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousUnit);
    int alphaOnly[8] = {};
    for(int i = 0; i < 8; ++i)
    {
        glActiveTexture(GL_TEXTURE0 + i);
        GLint binding = 0;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &binding);
        auto it = globals->textures.find((GLuint)binding);
        alphaOnly[i] = it != globals->textures.end() && it->second->baseFormat == GL_ALPHA;
    }
    glActiveTexture(previousUnit);
    activeFixedProgram->uAlphaOnly.Apply(alphaOnly, 8);
    activeFixedProgram->uTexMode0.Apply((int)globals->client.texCoord[0].texCoordBlendLogic);
    activeFixedProgram->uTexColor0.Apply(globals->client.texCoord[0].texCoordColor);
    activeFixedProgram->uFogColor.Apply(globals->ff.fogColor);
    activeFixedProgram->uFogValues.Apply(vector3_t{globals->ff.fogStart, globals->ff.fogEnd, globals->ff.fogDensity});
    activeFixedProgram->uAmbientColor.Apply(globals->render.ambient);
    activeFixedProgram->uLights.Apply(globals->ff.lights);
    activeFixedProgram->uShininess.Apply(globals->ff.matShininess);
    if(FL(SF_COLOR_MAT))
    {
        static const vector4_t white = {1.0f, 1.0f, 1.0f, 1.0f};
        activeFixedProgram->uMatAmbient.Apply(white);
        activeFixedProgram->uMatDiffuse.Apply(white);
    }
    else
    {
        activeFixedProgram->uMatAmbient.Apply(*(const vector4_t*)globals->ff.matAmbient);
        activeFixedProgram->uMatDiffuse.Apply(*(const vector4_t*)globals->ff.matDiffuse);
    }
    activeFixedProgram->uMatSpecular.Apply(*(const vector4_t*)globals->ff.matSpecular);
    activeFixedProgram->uMatEmission.Apply(*(const vector4_t*)globals->ff.matEmission);
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
            modes[i] = globals->client.texCoord[i + 1].texCoordBlendLogic;
        }
        vector4_t colors[7];
        for(int i = 0; i < 7; ++i) colors[i] = globals->client.texCoord[i+1].texCoordColor;
        if(activeFixedProgram->uTexColors.id != -1) glUniform4fv(activeFixedProgram->uTexColors.id, 7, &colors[0].x);
        activeFixedProgram->uTexModes.Apply(modes, 7);
        activeFixedProgram->uTexIDs.Apply(samplers, 7);
    }
}

void TransformFixedVerts()
{
    if(globals->render.vertices.empty()) return;
    client_state_t previous = globals->client;
    auto& client = globals->client;
    auto& render = globals->render;
    client.vertexArrayEnabled = true;
    client.vertexSize = 4; client.vertexType = GL_FLOAT; client.vertexStride = 0; client.vertexBuffer = 0;
    client.vertexPtr = render.vertices.data();
    client.colorArrayEnabled = true;
    client.colorSize = 4; client.colorType = GL_FLOAT; client.colorStride = 0; client.colorBuffer = 0;
    client.colorPtr = render.colors.data();
    client.normalArrayEnabled = true;
    client.normalType = GL_FLOAT; client.normalStride = 0; client.normalBuffer = 0;
    client.normalPtr = render.normals.data();
    for(auto& tex : client.texCoord) tex.enabled = false;
    auto& tex = client.texCoord[0];
    tex.enabled = true; tex.texCoordSize = 2; tex.texCoordType = GL_FLOAT;
    tex.texCoordStride = 0; tex.texCoordBuffer = 0; tex.texCoordPtr = render.texcoords.data();
    WRAP(glDrawArrays(render.lastPrimitiveMode, 0, (GLsizei)render.vertices.size()));
    globals->client = previous;
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
