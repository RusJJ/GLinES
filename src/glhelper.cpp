#include "GLES.h"
#include "gl_buffer.h"
#include "gl_matrix.h"
#include "gl_object.h"
#include "gl_queries.h"
#include "gl_render.h"
#include "gl_shader.h"
#include "gl_texture.h"

#include "glhelper.h"

#include <unordered_map>
#include <string>

struct fixed_program_t;

unsigned int g_nFixedPipelineShaderFlags = 0;
#define FL(x) ((g_nFixedPipelineShaderFlags & (x)) != 0)
#define EFL(x) (g_nFixedPipelineShaderFlags |= (x))

GLuint g_nUberShader = 0;
fixed_program_t* activeFixedProgram = NULL;

enum eShaderFlags
{
    NONE = 0,
    SF_TEXTURED = (1 << 0),
    SF_LIGHT0 = (1 << 1),
    SF_LIGHT1 = (1 << 2),
    SF_LIGHT2 = (1 << 3),
    SF_LIGHT3 = (1 << 4),
    SF_LIGHT4 = (1 << 5),
    SF_LIGHT5 = (1 << 6),
    SF_LIGHT6 = (1 << 7),
    SF_LIGHT7 = (1 << 8),
    SF_FOG_LINEAR = (1 << 9),
    SF_FOG_EXP = (1 << 10),
    SF_FOG_EXP2 = (1 << 11),
    SF_TEXUNIT1 = (1 << 12),
    SF_TEXUNIT2 = (1 << 13),
    SF_TEXUNIT3 = (1 << 14),
    SF_TEXUNIT4 = (1 << 15),
    SF_TEXUNIT5 = (1 << 16),
    SF_TEXUNIT6 = (1 << 17),
    SF_TEXUNIT7 = (1 << 18),
    SF_LIGHTING = (1 << 19),
    SF_COLOR_MAT = (1 << 20)
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
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);
        
        id = glGetUniformBlockIndex(g_nUberShader, "u_lightBlock");
        if(id != GL_INVALID_INDEX) glUniformBlockBinding(g_nUberShader, id, 0);
    }
    inline void Apply(const fixed_light_t* v)
    {
        if(ubo == 0 || !memcmp(&lights, v, sizeof(lights))) return;
 
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
    fixed_uniform_t uTexIDs;
    fixed_uniform_t uShininess;
    fixed_uniform_t uMatAmbient;
    fixed_uniform_t uMatDiffuse;
    fixed_uniform_t uMatSpecular;
    fixed_uniform_t uMatEmission;
    fixed_lights_uniform_t uLights;
};

std::unordered_map<unsigned int, fixed_program_t*> g_mapFixedPrograms;

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
    }
    if(globals->ff.fogEnabled)
    {
        if(globals->ff.fogMode == GL_EXP2) EFL(SF_FOG_EXP2);
        else if(globals->ff.fogMode == GL_LINEAR) EFL(SF_FOG_LINEAR);
        else EFL(SF_FOG_EXP);
    }
    if(globals->client.texCoord[1].enabled) EFL(SF_TEXUNIT1);
    if(globals->client.texCoord[2].enabled) EFL(SF_TEXUNIT2);
    if(globals->client.texCoord[3].enabled) EFL(SF_TEXUNIT3);
    if(globals->client.texCoord[4].enabled) EFL(SF_TEXUNIT4);
    if(globals->client.texCoord[5].enabled) EFL(SF_TEXUNIT5);
    if(globals->client.texCoord[6].enabled) EFL(SF_TEXUNIT6);
    if(globals->client.texCoord[7].enabled) EFL(SF_TEXUNIT7);
}

std::string BuildVertexShader()
{
    // Header
    std::string s = "#version 320 es\nprecision highp float;\n";
    s += "layout(location = 0) in vec3 a_position;\n";
    s += "layout(location = 2) in vec3 a_normal;\n";
    s += "layout(location = 3) in vec4 a_color;\n";
    s += "layout(location = 8) in vec2 a_texCoord;\n";
    s += "uniform mat4 u_modelview;\n";
    s += "uniform mat4 u_proj;\n";
    s += "uniform mat3 u_normal;\n";
    s += "out lowp vec4 v_color;\n";
    s += "out vec2 v_texCoord;\n";
    s += "out vec4 v_position;\n";
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
        s += "  vec4 spotParams;\n";       // x=exp, y=cutoff
        s += "  vec4 attenuationParams;\n"; // x=const, y=linear, z=quad
        s += "};\n";
        s += "layout(std140) uniform u_lightBlock {\n";
        s += "  LightData light[8];\n";
        s += "};\n";
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
    }
    
    // Body
    s += "void main() {\n";
    s += "  vec4 viewPos = u_modelview * vec4(a_position, 1.0);\n";
    s += "  v_position   = u_proj * viewPos;\n";
    s += "  gl_Position  = v_position;\n";
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
        s += "  lowp vec4 finalLight = u_ambientColor * u_matAmbient + u_matEmission;\n";
        s += "  vec3 n = normalize(normalMatrix * a_normal);\n";
        s += "  vec3 vDir = normalize(-viewPos.xyz);\n";
        if(FL(SF_LIGHT0)) s += "  finalLight += calcLight(0, n, viewPos.xyz, vDir);\n";
        if(FL(SF_LIGHT1)) s += "  finalLight += calcLight(1, n, viewPos.xyz, vDir);\n";
        if(FL(SF_LIGHT2)) s += "  finalLight += calcLight(2, n, viewPos.xyz, vDir);\n";
        if(FL(SF_LIGHT3)) s += "  finalLight += calcLight(3, n, viewPos.xyz, vDir);\n";
        if(FL(SF_LIGHT4)) s += "  finalLight += calcLight(4, n, viewPos.xyz, vDir);\n";
        if(FL(SF_LIGHT5)) s += "  finalLight += calcLight(5, n, viewPos.xyz, vDir);\n";
        if(FL(SF_LIGHT6)) s += "  finalLight += calcLight(6, n, viewPos.xyz, vDir);\n";
        if(FL(SF_LIGHT7)) s += "  finalLight += calcLight(7, n, viewPos.xyz, vDir);\n";
        s += "  v_color = clamp(finalLight, 0.0, 1.0) * a_color;\n";
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
    s += "  v_texCoord = a_texCoord;\n";
    s += "}\n";
    return s;
}

std::string BuildFragmentShader()
{
    // Header
    std::string s = "#version 320 es\nprecision mediump float;\n";
    s += "in lowp vec4 v_color;\n";
    s += "in vec2 v_texCoord;\n";
    s += "in vec4 v_position;\n";
    s += "uniform sampler2D u_texture;\n";
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
    if(FL(SF_TEXUNIT1) || FL(SF_TEXUNIT2) || FL(SF_TEXUNIT3) || FL(SF_TEXUNIT4) ||
        FL(SF_TEXUNIT5) || FL(SF_TEXUNIT6) || FL(SF_TEXUNIT7))
    {
        s += "in vec2 v_texCoords[7];\n";
        s += "uniform vec4 u_texColor[7];\n";
        s += "uniform int  u_texMode[7];\n";
        s += "uniform sampler2D u_texId[7];\n";
        s += "void mixUnitColor(int unit) {\n";
        s += "  int mode = u_texMode[unit];\n";
        s += "  lowp vec4 texColor = texture(u_texId[unit], v_texCoords[unit]);\n";
        s += "  if(mode == 0) { finalColor = texColor; return; }\n";             // GL_REPLACE
        s += "  if(mode == 1) { finalColor *= texColor; return; }\n";            // GL_MODULATE
        s += "  if(mode == 2) { finalColor = vec4(finalColor.rgb + texColor.rgb, finalColor.a * texColor.a); return; }\n"; // GL_ADD
        s += "  if(mode == 3) { finalColor = vec4(mix(finalColor.rgb, u_texColor[unit].rgb, texColor.rgb), finalColor.a * texColor.a); return; }\n"; // GL_BLEND
        s += "  if(mode == 4) { finalColor = vec4(mix(finalColor.rgb, texColor.rgb, texColor.a), finalColor.a); return; }\n"; // GL_DECAL
        s += "}\n";
    }
    
    // Body
    s += "void main() {\n";
    if(FL(SF_TEXTURED))
    {
        s += "  finalColor = v_color * texture(u_texture, v_texCoord);\n";
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

void UseFixedProgram()
{
    // If the app has its own shader active, don't override it.
    if(globals->gl.activeProgram != 0) return;
    
    BuildShaderFlag();
    
    auto it = g_mapFixedPrograms.find(g_nFixedPipelineShaderFlags);
    if(it != g_mapFixedPrograms.end())
    {
        activeFixedProgram = it->second;
        g_nUberShader = activeFixedProgram->program;
    }
    else
    {
        g_nUberShader = BuildFixedProgram();
        
        fixed_program_t* program = new fixed_program_t;
        program->program = g_nUberShader;
        
        activeFixedProgram = program;
        g_mapFixedPrograms.insert({ g_nFixedPipelineShaderFlags, program });
        
        program->uModelView.id    = glGetUniformLocation(g_nUberShader, "u_modelview");
        program->uProj.id         = glGetUniformLocation(g_nUberShader, "u_proj");
        program->uNormal.id       = glGetUniformLocation(g_nUberShader, "u_normal");
        program->uDiffuse.id      = glGetUniformLocation(g_nUberShader, "u_texture");
        program->uFogColor.id     = glGetUniformLocation(g_nUberShader, "u_fogColor");
        program->uFogValues.id    = glGetUniformLocation(g_nUberShader, "u_fogValues");
        program->uAmbientColor.id = glGetUniformLocation(g_nUberShader, "u_ambientColor");
        program->uTexCoords.id    = glGetUniformLocation(g_nUberShader, "u_texCoords");
        program->uTexColors.id    = glGetUniformLocation(g_nUberShader, "u_texColor");
        program->uTexModes.id     = glGetUniformLocation(g_nUberShader, "u_texMode");
        program->uTexIDs.id       = glGetUniformLocation(g_nUberShader, "u_texId");
        program->uShininess.id    = glGetUniformLocation(g_nUberShader, "u_shininess");
        program->uMatAmbient.id   = glGetUniformLocation(g_nUberShader, "u_matAmbient");
        program->uMatDiffuse.id   = glGetUniformLocation(g_nUberShader, "u_matDiffuse");
        program->uMatSpecular.id  = glGetUniformLocation(g_nUberShader, "u_matSpecular");
        program->uMatEmission.id  = glGetUniformLocation(g_nUberShader, "u_matEmission");
        
        if(FL(SF_LIGHT0) || FL(SF_LIGHT1) || FL(SF_LIGHT2) || FL(SF_LIGHT3) || 
            FL(SF_LIGHT4) || FL(SF_LIGHT5) || FL(SF_LIGHT6) || FL(SF_LIGHT7))
        {
            program->uLights.Init();
        }
    }
    
    glUseProgram(g_nUberShader);
    
    activeFixedProgram->uModelView.Apply(globals->matrix.modelview.Current());
    activeFixedProgram->uProj.Apply(globals->matrix.projection.Current());
    if(!globals->settings.matrix_transpose_invector)
    {
        activeFixedProgram->uNormal.Apply(GetNormalMatrix(globals->matrix.modelview.Current().m), true);
    }
    activeFixedProgram->uDiffuse.Apply(0);
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
    // TODO: per-unit texture uniforms (u_texColor, u_texMode, u_texId)
    if(FL(SF_TEXUNIT1) || FL(SF_TEXUNIT2) || FL(SF_TEXUNIT3) || FL(SF_TEXUNIT4) ||
       FL(SF_TEXUNIT5) || FL(SF_TEXUNIT6) || FL(SF_TEXUNIT7))
    {
        int modes[7]    = {0};
        int samplers[7] = {1, 2, 3, 4, 5, 6, 7}; // texture units 1-7 → sampler slots 1-7
        for(int i = 0; i < 7; ++i)
        {
            modes[i] = globals->client.texCoord[i + 1].texCoordBlendLogic;
        }
        activeFixedProgram->uTexModes.Apply(modes, 7);
        activeFixedProgram->uTexIDs.Apply(samplers, 7);
    }
}

void TransformFixedVerts()
{
    GLenum drawMode = globals->lastPrimitiveMode;
    std::vector<vector3_t> finalVerts;
    std::vector<vector4_t> finalColors;
    std::vector<vector2_t> finalTexCoords;
    std::vector<vector3_t> finalNormals;

    if(drawMode == GL_QUADS) // GLES doesn't have GL_QUADS
    {
        drawMode = GL_TRIANGLES;
        size_t n = globals->render.vertices.size();
        
        for(size_t i = 0; i + 4 < n; i += 4)
        {
            finalVerts.push_back(globals->render.vertices[i+0]);
            finalVerts.push_back(globals->render.vertices[i+1]);
            finalVerts.push_back(globals->render.vertices[i+2]);
            finalVerts.push_back(globals->render.vertices[i+0]);
            finalVerts.push_back(globals->render.vertices[i+2]);
            finalVerts.push_back(globals->render.vertices[i+3]);

            if(!globals->render.colors.empty())
            {
                finalColors.push_back(globals->render.colors[i+0]);
                finalColors.push_back(globals->render.colors[i+1]);
                finalColors.push_back(globals->render.colors[i+2]);
                finalColors.push_back(globals->render.colors[i+0]);
                finalColors.push_back(globals->render.colors[i+2]);
                finalColors.push_back(globals->render.colors[i+3]);
            }
            if(!globals->render.texcoords.empty())
            {
                finalTexCoords.push_back(globals->render.texcoords[i+0]);
                finalTexCoords.push_back(globals->render.texcoords[i+1]);
                finalTexCoords.push_back(globals->render.texcoords[i+2]);
                finalTexCoords.push_back(globals->render.texcoords[i+0]);
                finalTexCoords.push_back(globals->render.texcoords[i+2]);
                finalTexCoords.push_back(globals->render.texcoords[i+3]);
            }
            if(!globals->render.normals.empty())
            {
                finalNormals.push_back(globals->render.normals[i+0]);
                finalNormals.push_back(globals->render.normals[i+1]);
                finalNormals.push_back(globals->render.normals[i+2]);
                finalNormals.push_back(globals->render.normals[i+0]);
                finalNormals.push_back(globals->render.normals[i+2]);
                finalNormals.push_back(globals->render.normals[i+3]);
            }
        }
    }
    else if(drawMode == 0x0009) // GL_POLYGON -> triangle fan
    {
        drawMode = GL_TRIANGLE_FAN; 
        finalVerts      = globals->render.vertices;
        finalColors     = globals->render.colors;
        finalTexCoords  = globals->render.texcoords;
        finalNormals    = globals->render.normals;
    }
    else
    {
        finalVerts      = globals->render.vertices;
        finalColors     = globals->render.colors;
        finalTexCoords  = globals->render.texcoords;
        finalNormals    = globals->render.normals;
    }
    
    if(finalVerts.empty()) return;

    GLuint* vbos = &globals->render.fixedVBO[0];
    if(globals->render.fixedVAO == 0)
    {
        glGenVertexArrays(1, &globals->render.fixedVAO);
        glGenBuffers(11, globals->render.fixedVBO);
    }
    glBindVertexArray(globals->render.fixedVAO);

    glBindBuffer(GL_ARRAY_BUFFER, vbos[0]);
    glBufferData(GL_ARRAY_BUFFER, finalVerts.size() * sizeof(vector3_t), finalVerts.data(), GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);

    if(!finalColors.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, vbos[1]);
        glBufferData(GL_ARRAY_BUFFER, finalColors.size() * sizeof(vector4_t), finalColors.data(), GL_STREAM_DRAW);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 0, NULL);
    }
    else
    {
        glDisableVertexAttribArray(3);
        // Supply the current colour as a constant attribute.
        glVertexAttrib4fv(3, &globals->render.color.x);
    }

    if(!finalNormals.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
        glBufferData(GL_ARRAY_BUFFER, finalNormals.size() * sizeof(vector3_t), finalNormals.data(), GL_STREAM_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    }
    else
    {
        glDisableVertexAttribArray(2);
        glVertexAttrib3f(2, 0.0f, 0.0f, 1.0f);
    }

    if(!finalTexCoords.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, vbos[3]);
        glBufferData(GL_ARRAY_BUFFER, finalTexCoords.size() * sizeof(vector2_t), finalTexCoords.data(), GL_STREAM_DRAW);
        glEnableVertexAttribArray(8);
        glVertexAttribPointer(8, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    }
    else
    {
        glDisableVertexAttribArray(8);
    }

    glDrawArrays(drawMode, 0, (GLsizei)finalVerts.size()); // WRAP?
    glBindVertexArray(0);
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
