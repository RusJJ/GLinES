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
};

struct fixed_program_t
{
    GLuint program;
    GLint uMVP = -1;
    GLint uProj = -1;
    GLint uDiffuse = -1;
};

unsigned int g_nFixedPipelineShaderFlags = 0;
GLuint g_nUberShader = 0;
fixed_program_t* activeFixedProgram = NULL;

std::unordered_map<unsigned int, fixed_program_t*> g_mapFixedPrograms;

inline void BuildShaderFlag()
{
    g_nFixedPipelineShaderFlags = 0;
    
    if(globals->render.texture) g_nFixedPipelineShaderFlags |= SF_TEXTURED;
}

std::string BuildVertexShader()
{
    std::string s = "#version 320 es\nprecision highp float;\n";
    s += "layout(location = 0) in vec3 a_position;\n";
    s += "layout(location = 1) in vec4 a_color;\n";
    s += "layout(location = 2) in vec2 a_texCoord;\n";
    s += "layout(location = 3) in vec3 a_normal;\n";
    s += "uniform mat4 u_modelViewMatrix;";
    s += "uniform mat4 u_projectionMatrix;\n";
    s += "out vec4 v_color;\n";
    s += "out vec2 v_texCoord;\n";
    
    s += "void main() {\n";
    s += "  gl_Position = u_projectionMatrix * u_modelViewMatrix * vec4(a_position, 1.0);\n";
    s += "  v_color = a_color;\n";
    s += "  v_texCoord = a_texCoord;\n";
    s += "}";
    return s;
}

std::string BuildFragmentShader()
{
    std::string s = "#version 320 es\nprecision mediump float;\n";
    s += "in vec4 v_color;\n";
    s += "in vec2 v_texCoord;\n";
    s += "uniform sampler2D u_texture;\n";
    s += "out vec4 out_FragColor;\n";
    
    s += "void main() {\n";
    if(g_nFixedPipelineShaderFlags & SF_TEXTURED)
    {
        s += "  out_FragColor = v_color * texture(u_texture, v_texCoord);\n";
    }
    else
    {
        s += "  out_FragColor = v_color;\n";
    }
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
        
        program->uMVP = glGetUniformLocation(g_nUberShader, "u_modelViewMatrix");
        program->uProj = glGetUniformLocation(g_nUberShader, "u_projectionMatrix");
        program->uDiffuse = glGetUniformLocation(g_nUberShader, "u_texture");
    }
    
    glUseProgram(g_nUberShader);
    
    if(activeFixedProgram->uMVP != -1)
    {
        glUniformMatrix4fv(activeFixedProgram->uMVP, 1, GL_FALSE, globals->matrix.modelview.Current().data());
    }
    if(activeFixedProgram->uProj != -1)
    {
        glUniformMatrix4fv(activeFixedProgram->uProj, 1, GL_FALSE, globals->matrix.projection.Current().data());
    }
    if(activeFixedProgram->uDiffuse != -1)
    {
        glUniform1i(activeFixedProgram->uDiffuse, 0);
    }
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
        glGenBuffers(4, globals->render.fixedVBO);
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
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, NULL);
    }
    else
    {
        glDisableVertexAttribArray(1);
    }

    if(!finalTexCoords.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, vbos[2]);
        glBufferData(GL_ARRAY_BUFFER, finalTexCoords.size() * sizeof(vector2_t), finalTexCoords.data(), GL_STREAM_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    }
    else
    {
        glDisableVertexAttribArray(2);
    }

    if(!finalNormals.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, vbos[3]);
        glBufferData(GL_ARRAY_BUFFER, finalNormals.size() * sizeof(vector3_t), finalNormals.data(), GL_STREAM_DRAW);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, NULL);
    }
    else
    {
        glDisableVertexAttribArray(3);
    }
    
    // TODO: UBO

    WRAP(glDrawArrays(drawMode, 0, finalVerts.size()));
    glBindVertexArray(0);
}
