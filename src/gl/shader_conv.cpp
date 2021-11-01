#include "gl_shader_conv.h"
#include "globals.h"

#include <sstream>
#include <iostream>
#include <algorithm>
#include <fstream>
#include <stdio.h>

static char szShaderSource[16384]; // 16kb?

// Those were taken directly from GL4ES
static const char* GLES_ftransformFn = 
    "highp vec4 ftransform() {\n"
    "  return GLIN_ModelViewProjectionMatrix * GLIN_Vertex;\n"
    "}\n";

static const char* GLES_shadow2DFn = 
    "vec4 _Shadow2D_TMP = vec4(0.0);\n"
    "vec4 shadow2D(sampler2DShadow a, vec3 b) {\n"
    "  _Shadow2D_TMP.x = texture(a, b); return _Shadow2D_TMP;\n"
    "}\n";

static char pszShaderLog[280];
void WRAP(glCompileShader(GLuint shader))
{
    static GLsizei length; // Useless variable. Do not initialize it every time.

    glGetShaderSource(shader, sizeof(szShaderSource), &length, szShaderSource);

    PreprocessShader(szShaderSource);
    const char* pNewShader = ConvertShader(szShaderSource, globals->shaders[shader]->vertexShader);

    glShaderSource(shader, 1, (const GLchar**)&pNewShader, NULL);
    glCompileShader(shader);

    static GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

    if(status != GL_TRUE)
    {
        glGetShaderInfoLog(shader, sizeof(pszShaderLog), &length, pszShaderLog);
        ERR("%s shader #%d compilation error:\n%s", globals->shaders[shader]->vertexShader ? "Vertex" : "Fragment", shader, pszShaderLog);

        //char sss[128];
        //sprintf(sss, "/sdcard/srceng2/shaders_org/shader_%d.txt", shader);
        //std::ofstream a1(sss);
        //a1 << szShaderSource;
        //a1.close();
        //sprintf(sss, "/sdcard/srceng2/shadersGLIN/shader_%d.txt", shader);
        //std::ofstream a2(sss);
        //a2 << pNewShader;
        //a2.close();
    }
    delete[] pNewShader;
}

GLuint WRAP(glCreateShader(GLenum type))
{
    GLuint nShader = glCreateShader(type);

    auto pShader = globals->shaders[nShader];
    if(pShader == NULL)
    {
        pShader = new shader_desc_t;
        globals->shaders[nShader] = pShader;
    }
    pShader->shader = nShader;
    pShader->vertexShader = (type == GL_VERTEX_SHADER);

    return nShader;
}

bool bUsingTextureLODEXT, bUsingTexture3DEXT,
     bUsingFragDepthEXT, bUsingStandardDerivativesEXT,
     bUsesFTransformFn, bUsesShadow2DFn, bUsingFragData;
void PreprocessShader(char* pszShaderSource)
{
    bUsingTextureLODEXT =  (strstr(pszShaderSource, "texture2DLod") != NULL     || strstr(pszShaderSource, "texture2DProjLod") != NULL     ||
                            strstr(pszShaderSource, "textureCubeLod") != NULL   || strstr(pszShaderSource, "textureCubeGradARB") != NULL   ||
                            strstr(pszShaderSource, "texture2DGradARB") != NULL || strstr(pszShaderSource, "texture2DProjGradARB") != NULL  );

    bUsingTexture3DEXT =   (strstr(pszShaderSource, "sampler3D") != NULL        || strstr(pszShaderSource, "texture3D") != NULL);

    bUsingFragDepthEXT =    strstr(pszShaderSource, "gl_FragDepth") != NULL;

    // We can just enable it, huh?
    // GLSL will use it if it needs to be used...
    //bUsingStandardDerivativesEXT = (strstr(pszShaderSource, "dFdx(") != NULL    || strstr(pszShaderSource, "dFdy(") != NULL  || strstr(pszShaderSource, "fwidth(") != NULL ||
    //                                strstr(pszShaderSource, "dFdx (") != NULL   || strstr(pszShaderSource, "dFdy (") != NULL || strstr(pszShaderSource, "fwidth (") != NULL);
 
    bUsesFTransformFn =     strstr(pszShaderSource, "ftransform(") != NULL      || strstr(pszShaderSource, "ftransform (") != NULL;
    bUsesShadow2DFn =       strstr(pszShaderSource, "shadow2D(") != NULL        || strstr(pszShaderSource, "shadow2D (") != NULL;
    bUsingFragData =        strstr(pszShaderSource, "gl_FragData") != NULL;
}

char* ConvertARBShader(char* pszShaderSource, bool bIsVertexShader)
{
    const char *start = pszShaderSource;
    globals->arb.errorPtr = -1;
    char* pszNewShaderSource = new char[16384];

    while(start[0] != '!' && start[1] != '!')
    {
        ++start;
        if(start[1] == '\0')
        {
            globals->arb.errorPtr = 0;
          INVALID_PROG_START:
            if(globals->arb.errorStr != NULL) delete[] globals->arb.errorStr;
            globals->arb.errorStr = strdup("Invalid program start");
            return NULL;
        }
    }
    if (strncmp(start, bIsVertexShader ? "!!ARBvp1.0" : "!!ARBfp1.0", 10))
    {
        globals->arb.errorPtr = start - pszShaderSource;
        goto INVALID_PROG_START;
    }
    start += 10;

    memcpy(pszNewShaderSource, pszShaderSource, strlen(pszShaderSource));

    return pszNewShaderSource;
}

static std::string line;
static size_t temp;
char* ConvertShader(char* pszShaderSource, bool bIsVertexShader)
{
    std::istringstream f(pszShaderSource);

    char* pszNewShaderSource = new char[16384];

    // HEADER
    strcpy(pszNewShaderSource, bIsVertexShader ? _SHADER_HEADER_VERTEX : _SHADER_HEADER_FRAGMENT);
    if(bUsingTextureLODEXT) EXT_EN(EXT_shader_texture_lod);            // texture ... Lod
    if(bUsingTexture3DEXT) EXT_EN(OES_texture_3D);                     // sampler3D texture3D
    if(bUsingFragDepthEXT) EXT_EN(EXT_frag_depth);                     // gl_FragDepth
    /*if(bUsingStandardDerivativesEXT)*/ EXT_EN(OES_standard_derivatives); // dFdx() dFdy() fwidth()
                                                                           // I think all potato phones have it, 16+yo extension
    ADD(_SHADER_HEADER_SECOND);
    if(!bIsVertexShader)
    {
        if(!bUsingFragData) ADD("layout(location = 0) out vec4 gl_FragColor;\n"); // gl_FragColor = gl_FragData[0] = ...
        else ADD("layout(location = 0) out vec4 GLIN_FragData[8];\n"); // MAX_DRAW_BUFFERS = 8
    }
    else
    {
        if(bUsingFragData) ADD("layout(location = 0) out vec4 GLIN_FragData[8];\n"); // MAX_DRAW_BUFFERS = 8
    }

    ADD("\nout lowp vec4 GLIN_FrontColor;\n"
        "out lowp vec4 GLIN_BackColor;\n"
        "out lowp vec4 GLIN_FrontSecondaryColor;\n"
        "out lowp vec4 GLIN_BackSecondaryColor;\n"
        "out mediump vec4  GLIN_TexCoord[32];\n"
        "out mediump float GLIN_FogFragCoord;\n"
        "vec4 GLIN_ClipVertex;\n"
        "uniform highp mat4 GLIN_ModelViewProjectionMatrix;\n");

    if(bIsVertexShader)
    {
        ADD("in highp vec4 GLIN_Vertex;\n"
            "in lowp vec4 GLIN_Color;\n"
            "in lowp vec4 GLIN_SecondaryColor;\n"
            "in highp vec3 GLIN_Normal;\n"
            "in highp float GLIN_FogCoord;\n"
            "in highp vec4 GLIN_MultiTexCoord0;\n"
            "in highp vec4 GLIN_MultiTexCoord1;\n"
            "in highp vec4 GLIN_MultiTexCoord2;\n"
            "in highp vec4 GLIN_MultiTexCoord3;\n"
            "in highp vec4 GLIN_MultiTexCoord4;\n"
            "in highp vec4 GLIN_MultiTexCoord5;\n"
            "in highp vec4 GLIN_MultiTexCoord6;\n"
            "in highp vec4 GLIN_MultiTexCoord7;\n"
            "in highp vec4 GLIN_MultiTexCoord8;\n"
            "in highp vec4 GLIN_MultiTexCoord9;\n"
            "in highp vec4 GLIN_MultiTexCoord10;\n"
            "in highp vec4 GLIN_MultiTexCoord11;\n"
            "in highp vec4 GLIN_MultiTexCoord12;\n"
            "in highp vec4 GLIN_MultiTexCoord13;\n"
            "in highp vec4 GLIN_MultiTexCoord14;\n"
            "in highp vec4 GLIN_MultiTexCoord15;\n");
    }
    else
    {
        ADD("out lowp vec4 GLIN_Color;\n"
            "out lowp vec4 GLIN_SecondaryColor;\n");
    }

    // FUNCTIONS
    if(bUsesFTransformFn) ADD(GLES_ftransformFn);
    if(bUsesShadow2DFn) ADD(GLES_shadow2DFn);
    // FUNCTIONS END

    ADD("\n"); // Be beautiful
    // HEADER END

    while (std::getline(f, line))
    {
        if(line[1] == '/') goto CONTINUE;
        if((line[0] == '#' && (line[1] == 'v' || line[1] == 'e'))) continue;

        REPLACE("gl_FogCoord", "GLIN_FogCoord");
        REPLACE("gl_MultiTexCoord0", "GLIN_MultiTexCoord0");
        REPLACE("gl_MultiTexCoord1", "GLIN_MultiTexCoord1");
        REPLACE("gl_MultiTexCoord2", "GLIN_MultiTexCoord2");
        REPLACE("gl_MultiTexCoord3", "GLIN_MultiTexCoord3");
        REPLACE("gl_MultiTexCoord4", "GLIN_MultiTexCoord4");
        REPLACE("gl_MultiTexCoord5", "GLIN_MultiTexCoord5");
        REPLACE("gl_MultiTexCoord6", "GLIN_MultiTexCoord6");
        REPLACE("gl_MultiTexCoord7", "GLIN_MultiTexCoord7");
        REPLACE("gl_MultiTexCoord8", "GLIN_MultiTexCoord8");
        REPLACE("gl_MultiTexCoord9", "GLIN_MultiTexCoord9");
        REPLACE("gl_MultiTexCoord10", "GLIN_MultiTexCoord10");
        REPLACE("gl_MultiTexCoord11", "GLIN_MultiTexCoord11");
        REPLACE("gl_MultiTexCoord12", "GLIN_MultiTexCoord12");
        REPLACE("gl_MultiTexCoord13", "GLIN_MultiTexCoord13");
        REPLACE("gl_MultiTexCoord14", "GLIN_MultiTexCoord14");
        REPLACE("gl_MultiTexCoord15", "GLIN_MultiTexCoord15");

        REPLACE("gl_TexCoord[", "GLIN_TexCoord[");
        REPLACE("gl_FogFragCoord", "GLIN_FogFragCoord");
        REPLACE("gl_ClipVertex", "GLIN_ClipVertex");
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
            REPLACE_BREAK("textureCubeGradARB",   "textureCubeGradEXT");
        }

        // Everything is okay and dont need changes
      CONTINUE:
        ADD((line + "\n").c_str());
    }

    return pszNewShaderSource;
}