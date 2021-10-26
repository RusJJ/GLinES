#include "gl_shader_conv.h"
#include "globals.h"

#include <sstream>
#include <iostream>
#include <algorithm>

static char szShaderSource[16384]; // 16kb?
static char szNewShaderSource[16384];

/*static char pszNewShaderSource[16384] =
    "// Converted VERTEX shader, GLinES\n"
    "#version 320 es\n"
	"layout(location = 0) in vec4 vPosition;  \n"
    "void main() {                            \n"
    "   gl_Position = vPosition;              \n"
    "}                                        \n";
static char pszNewShaderSource2[16384] =
    "// Converted FRAGMENT shader, GLinES\n"
    "#version 320 es                                   \n"
    "precision mediump float;                          \n"
    "out vec4 fragColor;                               \n"
    "void main() {                                     \n"
    "    fragColor = vec4 ( 1.0, 0.0, 0.0, 1.0 );      \n"
    "}                                                 \n";*/


static char pszShaderLog[256];
void GLIN_Wrap_glCompileShader(GLuint shader)
{
    static GLsizei len;

    glGetShaderSource(shader, sizeof(szShaderSource), &len, szShaderSource);
    PreprocessShader(szShaderSource);
    ConvertShader(szShaderSource, globals->shaders[shader]->isVertexShader, szNewShaderSource);
    DBG("Convert Shader: %d length", strlen(szNewShaderSource));
    glShaderSource(shader, 1, (const GLchar**)&szNewShaderSource, NULL);
    glCompileShader(shader);

    static GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(status != GL_TRUE)
    {
        static GLint length; // Useless variable. Do not initialize it every time.
        glGetShaderInfoLog(shader, sizeof(pszShaderLog), &length, pszShaderLog);
        //ERR("%s shader #%d compilation error:\n%s", globals->shaders[shader]->isVertexShader ? "Vertex" : "Fragment", shader, pszShaderLog);
    }
}

GLuint GLIN_Wrap_glCreateShader(GLenum type)
{
    GLuint nShader = glCreateShader(type);

    auto pShader = globals->shaders[nShader];
    if(pShader == NULL)
    {
        pShader = new shader_desc_t;
        globals->shaders[nShader] = pShader;
    }
    pShader->shader = nShader;
    pShader->isVertexShader = (type == GL_VERTEX_SHADER);

    return nShader;
}

bool bUsingTextureLODEXT, bUsingTexture3DEXT, bUsingDrawBuffersEXT,
     bUsingFragDepthEXT, bUsingStandardDerivativesEXT;
void PreprocessShader(char* pszShaderSource)
{
    bUsingTextureLODEXT =  (strstr(pszShaderSource, "texture2DLod") != NULL     || strstr(pszShaderSource, "texture2DProjLod") != NULL     ||
                            strstr(pszShaderSource, "textureCubeLod") != NULL   || strstr(pszShaderSource, "textureCubeGradARB") != NULL   ||
                            strstr(pszShaderSource, "texture2DGradARB") != NULL || strstr(pszShaderSource, "texture2DProjGradARB") != NULL  );

    bUsingTexture3DEXT =   (strstr(pszShaderSource, "sampler3D") != NULL         || strstr(pszShaderSource, "texture3D") != NULL);

    bUsingDrawBuffersEXT = (strstr(pszShaderSource, "gl_FragData[") != NULL    || strstr(pszShaderSource, "gl_FragData [") != NULL);

    bUsingFragDepthEXT =    strstr(pszShaderSource, "gl_FragDepth") != NULL;

    bUsingStandardDerivativesEXT = (strstr(pszShaderSource, "dFdx(") != NULL   || strstr(pszShaderSource, "dFdy(") != NULL || strstr(pszShaderSource, "fwidth(") != NULL ||
                                    strstr(pszShaderSource, "dFdx (") != NULL  || strstr(pszShaderSource, "dFdy (") != NULL || strstr(pszShaderSource, "fwidth (") != NULL);
}

void ConvertShader(char* pszShaderSource, bool bIsVertexShader, char* pszNewShaderSource)
{
    std::istringstream f(pszShaderSource);
    std::string line;
    size_t temp;

    // HEADER START
    strcpy(pszNewShaderSource, bIsVertexShader ? _SHADER_HEADER_VERTEX : _SHADER_HEADER_FRAGMENT);
    if(bUsingTextureLODEXT) EXT_EN(EXT_shader_texture_lod);            // sampler3D texture3D
    if(bUsingTexture3DEXT) EXT_EN(OES_texture_3D);                     // sampler3D texture3D
    if(bUsingDrawBuffersEXT) EXT_EN(EXT_draw_buffers);                 // gl_FragData[n]
    if(bUsingFragDepthEXT) EXT_EN(EXT_frag_depth);                     // gl_FragDepth
    if(bUsingStandardDerivativesEXT) EXT_EN(OES_standard_derivatives); // dFdx() dFdy() fwidth()
    ADD(bIsVertexShader ? _SHADER_HEADER2_VERTEX : _SHADER_HEADER2_FRAGMENT);

    while (std::getline(f, line))
    {
        if (line.find("#version") != std::string::npos || line.find("#extension") != std::string::npos)
            continue; // SKIP

        if(bIsVertexShader)
        {
            
        }
        else
        {
            REPLACE_BREAK("texture2DLod",         "texture2DLodEXT");
            REPLACE_BREAK("texture2DProjLod",     "texture2DProjLodEXT");
            REPLACE_BREAK("textureCubeLod",       "textureCubeLodEXT");
            REPLACE_BREAK("texture2DGradARB",     "texture2DGradEXT");
            REPLACE_BREAK("texture2DProjGradARB", "texture2DProjGradEXT");
            REPLACE_BREAK("textureCubeGradARB",   "textureCubeGradEXT");
        }

        // Everything is okay and dont need changes
      CONTINUE:
        ADD((line + "\n").c_str());
    }
    // HEADER END
}