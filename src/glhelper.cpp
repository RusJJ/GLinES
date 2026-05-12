#include "GLES.h"
#include "wrapped.h"
#include "gl_buffer.h"
#include "gl_matrix.h"
#include "gl_object.h"
#include "gl_queries.h"
#include "gl_render.h"
#include "gl_shader.h"
#include "gl_texture.h"

#include <unordered_map>
#include <string>

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
    SF_LIGHTING = (1 << 19)
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
    inline void Apply(const matrix4_t& v)
    {
        if(id == -1 || mat4 == v) return;
        glUniformMatrix4fv(id, 1, GL_FALSE, v.m);
        mat4 = v;
    }
};

struct fixed_program_t
{
    GLuint program;
    fixed_uniform_t uModelView;
    fixed_uniform_t uProj;
    fixed_uniform_t uDiffuse;
    fixed_uniform_t uFogColor;
    fixed_uniform_t uFogValues;
    fixed_uniform_t uAmbientColor;
    fixed_uniform_t uTexCoords;
    fixed_uniform_t uTexColors;
    fixed_uniform_t uTexModes;
    fixed_uniform_t uTexIDs;
};

unsigned int g_nFixedPipelineShaderFlags = 0;
#define FL(x) ((g_nFixedPipelineShaderFlags & x) != 0)
#define EFL(x) (g_nFixedPipelineShaderFlags |= x)

GLuint g_nUberShader = 0;
fixed_program_t* activeFixedProgram = NULL;

std::unordered_map<unsigned int, fixed_program_t*> g_mapFixedPrograms;

inline void BuildShaderFlag()
{
    g_nFixedPipelineShaderFlags = 0;
    
    if(globals->render.texture || globals->client.texCoord[0].enabled) EFL(SF_TEXTURED);
    if(globals->ff.lightingEnabled)
    {
        EFL(SF_LIGHTING);
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
    s += "uniform mat4 u_modelview;";
    s += "uniform mat4 u_proj;\n";
    s += "out lowp vec4 v_color;\n";
    s += "out vec2 v_texCoord;\n";
    s += "out vec4 v_position;\n";
    if(FL(SF_TEXUNIT1) || FL(SF_TEXUNIT2) || FL(SF_TEXUNIT3) || FL(SF_TEXUNIT4) ||
        FL(SF_TEXUNIT5) || FL(SF_TEXUNIT6) || FL(SF_TEXUNIT7))
    {
        s += "layout(location = 9) vec2 a_texCoords[7];\n";
        s += "out vec2 v_texCoords[7];\n";
    }
    if(FL(SF_LIGHTING))
    {
        s += "uniform vec4 u_ambientColor;\n";
    }
    
    // Body
    s += "void main() {\n";
    s += "  v_position = u_proj * u_modelview * vec4(a_position, 1.0);\n";
    if(FL(SF_LIGHTING))
    {
        s += "  lowp vec4 finalLight = u_ambientColor;\n";
        // TODO: lights
        s += "  v_color = finalLight * a_color;\n";
    }
    else
    {
        s += "  v_color = a_color;\n";
    }
    if(FL(SF_TEXUNIT1)) s += "  v_texCoords[0] = a_texCoords[0];\n";
    if(FL(SF_TEXUNIT2)) s += "  v_texCoords[1] = a_texCoords[1];\n";
    if(FL(SF_TEXUNIT3)) s += "  v_texCoords[2] = a_texCoords[2];\n";
    if(FL(SF_TEXUNIT4)) s += "  v_texCoords[3] = a_texCoords[3];\n";
    if(FL(SF_TEXUNIT5)) s += "  v_texCoords[4] = a_texCoords[4];\n";
    if(FL(SF_TEXUNIT6)) s += "  v_texCoords[5] = a_texCoords[5];\n";
    if(FL(SF_TEXUNIT7)) s += "  v_texCoords[6] = a_texCoords[6];\n";
    s += "  v_texCoord = a_texCoord;\n";
    s += "  gl_Position = v_position;\n";
    s += "}";
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
    s += "uniform mat4 u_proj;\n";
    s += "out lowp vec4 out_FragColor;\n";
    s += "lowp vec4 finalColor;\n";
    if(FL(SF_FOG_EXP2) || FL(SF_FOG_LINEAR) || FL(SF_FOG_EXP))
    {
        s += "uniform vec4 u_fogColor;\n";
        s += "uniform vec3 u_fogValues;\n";
        s += "float getFogValue() {\n";
        if(FL(SF_FOG_LINEAR))
        {
            s += "  float factor = (u_fogValues.y - v_position.w) / (u_fogValues.y - u_fogValues.x);\n";
        }
        else if(FL(SF_FOG_EXP))
        {
            s += "  float factor = exp(-u_fogValues.z * v_position.w);\n";
        }
        else if(FL(SF_FOG_EXP2))
        {
            s += "  float factor = exp(-pow(u_fogValues.z * v_position.w, 2.0));\n";
        }
        s += "  return clamp(factor, 0.0, 1.0);\n";
        s += "}\n";
    }
    if(FL(SF_TEXUNIT1) || FL(SF_TEXUNIT2) || FL(SF_TEXUNIT3) || FL(SF_TEXUNIT4) ||
        FL(SF_TEXUNIT5) || FL(SF_TEXUNIT6) || FL(SF_TEXUNIT7))
    {
        s += "in vec2 v_texCoords[7];\n";
        s += "uniform vec4 u_texColor[7];\n";
        s += "uniform int u_texMode[7];\n";
        s += "uniform sampler2D u_texId[7];\n";
        s += "void mixUnitColor(int unit) {\n";
        s += "  int mode = u_texMode[unit];\n";
        s += "  lowp vec4 texColor = texture(u_texId[unit], v_texCoords[unit]);\n";
        s += "  if(mode == 0) { finalColor = texColor; return; }\n"; // GL_REPLACE
        s += "  if(mode == 1) { finalColor *= texColor; return; }\n"; // GL_MODULATE (default)
        s += "  if(mode == 2) { finalColor = vec4(finalColor.rgb + texColor.rgb, finalColor.a * texColor.a); return; }\n"; // GL_ADD
        s += "  if(mode == 3) { finalColor = vec4(mix(finalColor.rgb, u_texColor[unit].rgb, texColor.rgb), finalColor.a * texColor.a); return; }\n"; // GL_BLEND
        s += "  if(mode == 4) { finalColor = vec4(mix(finalColor.rgb, texColor.rgb, texColor.a), finalColor.a); return; }\n"; // GL_DECAL
        s += "}\n"; // TODO: GL_COMBINE..?
    }
    
    // Body
    s += "void main() {\n";
    if(FL(SF_TEXTURED))
    {
        s += "  finalColor = v_color * texture(u_texture, v_texCoord);\n";
    }
    else
    {
        s+= "  finalColor = v_color;\n";
    }
    if(FL(SF_TEXUNIT1)) s += "  mixUnitColor(0);\n"; // TODO: also TEXUNIT0...
    if(FL(SF_TEXUNIT2)) s += "  mixUnitColor(1);\n";
    if(FL(SF_TEXUNIT3)) s += "  mixUnitColor(2);\n";
    if(FL(SF_TEXUNIT4)) s += "  mixUnitColor(3);\n";
    if(FL(SF_TEXUNIT5)) s += "  mixUnitColor(4);\n";
    if(FL(SF_TEXUNIT6)) s += "  mixUnitColor(5);\n";
    if(FL(SF_TEXUNIT7)) s += "  mixUnitColor(6);\n";
    if(FL(SF_FOG_EXP2) || FL(SF_FOG_LINEAR) || FL(SF_FOG_EXP))
    {
        s+= "  finalColor.rgb = mix(u_fogColor.rgb, finalColor.rgb, getFogValue());\n";
    }
    s += "  out_FragColor = finalColor;\n";
    s += "}";
    return s;
}

unsigned int BuildFixedProgram()
{
    std::string vertexS = BuildVertexShader();
    std::string fragS = BuildFragmentShader();
    
    const char* vs = vertexS.c_str();
    const char* fs = fragS.c_str();

    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vs, 0);
    glCompileShader(vertex);

    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fs, 0);
    glCompileShader(fragment);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);

    glDeleteShader(vertex);
    glDeleteShader(fragment);
    return program;
}

void UseFixedProgram()
{
    // If we have our own shader. // TODO: pass uniforms!
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
        
        program->uModelView.id = glGetUniformLocation(g_nUberShader, "u_modelview");
        program->uProj.id = glGetUniformLocation(g_nUberShader, "u_proj");
        program->uDiffuse.id = glGetUniformLocation(g_nUberShader, "u_texture");
        program->uFogColor.id = glGetUniformLocation(g_nUberShader, "u_fogColor");
        program->uFogValues.id = glGetUniformLocation(g_nUberShader, "u_fogValues");
        program->uAmbientColor.id = glGetUniformLocation(g_nUberShader, "u_ambientColor");
        program->uTexCoords.id = glGetUniformLocation(g_nUberShader, "u_texCoords");
        program->uTexColors.id = glGetUniformLocation(g_nUberShader, "u_texColor");
        program->uTexModes.id = glGetUniformLocation(g_nUberShader, "u_texMode");
        program->uTexIDs.id = glGetUniformLocation(g_nUberShader, "u_texId");
    }
    
    glUseProgram(g_nUberShader);
    
    activeFixedProgram->uModelView.Apply(globals->matrix.modelview.Current());
    activeFixedProgram->uProj.Apply(globals->matrix.projection.Current());
    activeFixedProgram->uDiffuse.Apply(0);
    activeFixedProgram->uFogColor.Apply(globals->ff.fogColor);
    activeFixedProgram->uFogValues.Apply(vector3_t{globals->ff.fogStart, globals->ff.fogEnd, globals->ff.fogDensity});
    activeFixedProgram->uAmbientColor.Apply(globals->render.ambient);
    // TODO: texture units ;(
}

void TransformFixedVerts()
{
    GLenum drawMode = globals->lastPrimitiveMode;
    std::vector<vector3_t> finalVerts;
    std::vector<vector4_t> finalColors;
    std::vector<vector2_t> finalTexCoords;
    std::vector<vector3_t> finalNormals;

    if(drawMode == GL_QUADS) // GLES doesnt really have GL_QUADS
    {
        drawMode = GL_TRIANGLES;
        for(size_t i = 0; i + 3 < globals->render.vertices.size(); i += 4)
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
    else if(drawMode == 0x0009) // GL_POLYGON
    {
        drawMode = GL_TRIANGLE_FAN; 
        finalVerts = globals->render.vertices;
        finalColors = globals->render.colors;
        finalTexCoords = globals->render.texcoords;
        finalNormals = globals->render.normals;
    }
    else
    {
        finalVerts = globals->render.vertices;
        finalColors = globals->render.colors;
        finalTexCoords = globals->render.texcoords;
        finalNormals = globals->render.normals;
    }
    
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
    
    // TODO: UBO

    WRAP(glDrawArrays(drawMode, 0, finalVerts.size()));
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
