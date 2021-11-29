#include "gl_shader.h"
#include "globals.h"

#include <stdlib.h>
#include <iostream>
#include <algorithm>
#include <stdio.h>

static char szShaderSource[65536]; // 64kb?

// Those were taken directly from GL4ES (no, they dont...)
static const char* GLES_ftransformFn = 
    "in highp vec4 GLIN_Vertex;\nuniform highp mat4 GLIN_MVP;\nhighp vec4 ftransform(){return GLIN_MVP*GLIN_Vertex;}\n";

static const char* GLES_shadow2DFn = 
    "vec4 _Sh2D_TMP=vec4(0.0);vec4 shadow2D(sampler2DShadow a,vec3 b){_Sh2D_TMP.x=texture(a,b);return _Sh2D_TMP;}\n";

static const char* GLES_multiTexCoordVars = 
    "in highp vec4 GLIN_MTC0;\nin highp vec4 GLIN_MTC1;\nin highp vec4 GLIN_MTC2;\nin highp vec4 GLIN_MTC3;\n"
    "in highp vec4 GLIN_MTC4;\nin highp vec4 GLIN_MTC5;\nin highp vec4 GLIN_MTC6;\nin highp vec4 GLIN_MTC7;\nin highp vec4 GLIN_MTC8;\n"
    "in highp vec4 GLIN_MTC9;\nin highp vec4 GLIN_MTC10;\nin highp vec4 GLIN_MTC11;\nin highp vec4 GLIN_MTC12;\nin highp vec4 GLIN_MTC13;\n"
    "in highp vec4 GLIN_MTC14;\nin highp vec4 GLIN_MTC15;\n";

static const char* GLES_fragDataVar_Part1 = 
    "#define gl_FragData GLIN_FragData\nlayout(location=0) out mediump vec4 gl_FragData[";

static const char* GLES_min_fix = 
    "float min(int a,float b){ return min(float(a),b); }\nfloat min(float a,int b){ return min(a,float(b)); }";

static const char* GLES_max_fix = 
    "float max(int a,float b){ return max(float(a),b); }\nfloat max(float a,int b){ return max(a,float(b)); }";

static const char* GLES_clamp_fix = // Kinda lazy, taken from GL4ES
    "float clamp(float f,int a,int b) { return clamp(f,float(a),float(b)); }\n"
    "float clamp(float f,float a,int b) { return clamp(f,a,float(b)); }\n"
    "float clamp(float f,int a,float b) { return clamp(f,float(a),b); }\n"
    "vec2 clamp(vec2 f,int a,int b) { return clamp(f,float(a),float(b)); }\n"
    "vec2 clamp(vec2 f,float a,int b) { return clamp(f,a,float(b)); }\n"
    "vec2 clamp(vec2 f,int a,float b) { return clamp(f,float(a),b); }\n"
    "vec3 clamp(vec3 f,int a,int b) { return clamp(f,float(a),float(b)); }\n"
    "vec3 clamp(vec3 f,float a,int b) { return clamp(f,a,float(b)); }\n"
    "vec3 clamp(vec3 f,int a,float b) { return clamp(f,float(a),b); }\n"
    "vec4 clamp(vec4 f,int a,int b) { return clamp(f,float(a),float(b)); }\n"
    "vec4 clamp(vec4 f,float a,int b) { return clamp(f,a,float(b)); }\n"
    "vec4 clamp(vec4 f,int a,float b) { return clamp(f,float(a),b); }\n";

static char pszShaderLog[280];
static char sss[256];
static GLint shaderLen; static GLuint shaderId;
static FILE* shaderFile;
void WRAP(glCompileShader(GLuint shader))
{
    static GLsizei length;
    static GLint status = 0;

    bool isVertex = globals->shaders[shader]->vertexShader;
    glGetShaderSource(shader, sizeof(szShaderSource), &length, szShaderSource);
    PreprocessShader(szShaderSource, isVertex);

    sprintf(sss, "/sdcard/srceng2/shaders_org/shader_%d.txt", shader);
    shaderFile = fopen(sss, "w+");
    fputs(szShaderSource, shaderFile);
    fclose(shaderFile);

    shaderId = shader;
    const char* pNewShader = ConvertShader(szShaderSource, isVertex);
    glShaderSource(shader, 1, (const GLchar**)&pNewShader, &shaderLen);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE)
    {
        glGetShaderInfoLog(shader, sizeof(pszShaderLog), &length, pszShaderLog);
        ERR("%s shader #%d compilation error:\n%s", isVertex?"Vertex":"Fragment", shader, pszShaderLog);
    }

        sprintf(sss, "/sdcard/srceng2/shaders_glin/shader_%d.txt", shader);
        shaderFile = fopen(sss, "w+");
        fputs(pNewShader, shaderFile);
        fclose(shaderFile);
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

void WRAP(glLinkProgram(GLuint program))
{
    glLinkProgram(program);

    static GLint isLinked;
	glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
	if(isLinked == GL_FALSE)
	{
        ERR("[LINK] Program %d linking failed. Log:", program);
		GLint maxLength = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &maxLength);

		GLchar log[4096];
		glGetProgramInfoLog( program, sizeof(log), &maxLength, log );
		if( maxLength )
		{
			ERR("[LINK] %s", log);
		}

        static GLsizei count; static GLuint shaders[16] = { 0 }; static char i;
        glGetAttachedShaders(program, sizeof(shaders), &count, shaders);
        for(i = 0; i < count; ++i)
        {
            ERR("[LINK] Shader %d prog %d", shaders[i], program);
        }
	}
}

static bool bHasVersionDefinition;
static bool bUsingTextureLODEXT, bUsingTexture3DEXT, bUsingFragDepthEXT;
static bool bUsesFTransformFn, bUsesShadow2DFn, bUsingFragData, bUsingFragColor, bUsingMultiTexCoord, bUsingFFC, bUsingFSC, bUsingBSC, bUsingC, bUsingSC, bUsingFC, bUsingBC, bUsingClipVertex;
static bool usesMin, usesMax, usesClamp;
static int cMaxFragData=-1, ctemp; const char* strFragDatas;
void PreprocessShader(char* pszShaderSource, bool bIsVertexShader)
{
// GLinES' things
    if(!globals->ext.checked_exts_for_shaders)
    {
        globals->ext.checked_exts_for_shaders = true;
        const char* extensions = (const char*)glGetString(GL_EXTENSIONS);
        globals->ext.hasAlphaFuncQCOM = strstr(extensions, "GL_QCOM_alpha_test") != NULL;
        globals->ext.hasTextureLods = strstr(extensions, "GL_EXT_shader_texture_lod") != NULL;
    }
// Shader's Must-have things
    bHasVersionDefinition = strstr(pszShaderSource, "#version") != NULL;
// Extensions
    bUsingTextureLODEXT =  (strstr(pszShaderSource, "texture2DLod") != NULL     || strstr(pszShaderSource, "texture2DProjLod") != NULL     ||
                            strstr(pszShaderSource, "textureCubeLod") != NULL   || strstr(pszShaderSource, "textureCubeGradARB") != NULL   ||
                            strstr(pszShaderSource, "texture2DGradARB") != NULL || strstr(pszShaderSource, "texture2DProjGradARB") != NULL  );
    bUsingTexture3DEXT =   (strstr(pszShaderSource, "sampler3D") != NULL        || strstr(pszShaderSource, "texture3D") != NULL);
    bUsingFragDepthEXT =    strstr(pszShaderSource, "gl_FragDepth") != NULL;
// Variables!
    bUsesFTransformFn =     strstr(pszShaderSource, "ftransform(") != NULL      || strstr(pszShaderSource, "ftransform (") != NULL;
    bUsesShadow2DFn =       strstr(pszShaderSource, "shadow2D(") != NULL        || strstr(pszShaderSource, "shadow2D (") != NULL;
    usesMin =               strstr(pszShaderSource, "min(") != NULL             || strstr(pszShaderSource, "min (") != NULL;
    usesMax =               strstr(pszShaderSource, "max(") != NULL             || strstr(pszShaderSource, "max (") != NULL;
    usesClamp =             strstr(pszShaderSource, "clamp(") != NULL           || strstr(pszShaderSource, "clamp (") != NULL;
    bUsingFragColor =       strstr(pszShaderSource, "gl_FragColor") != NULL;
    bUsingMultiTexCoord =   strstr(pszShaderSource, "gl_MultiTexCoord") != NULL;
    bUsingFFC =             strstr(pszShaderSource, "gl_FogFragCoord") != NULL;
    bUsingC =               strstr(pszShaderSource, "gl_Color") != NULL;
    bUsingFC =              strstr(pszShaderSource, "gl_FrontColor") != NULL;
    bUsingBC =              strstr(pszShaderSource, "gl_BackColor") != NULL;
    bUsingSC =              strstr(pszShaderSource, "gl_SecondaryColor") != NULL;
    bUsingFSC =             strstr(pszShaderSource, "gl_FrontSecondaryColor") != NULL;
    bUsingBSC =             strstr(pszShaderSource, "gl_BackSecondaryColor") != NULL;
    bUsingClipVertex =      strstr(pszShaderSource, "gl_ClipVertex") != NULL;

// Others
    cMaxFragData = -1; strFragDatas = pszShaderSource;
    while((strFragDatas = strstr(strFragDatas + 11, "gl_FragData")) != NULL)
    {
        if(strFragDatas[11] == '[') ctemp = atoi(&strFragDatas[12]);
        else ctemp = atoi(&strFragDatas[13]);
        if(ctemp > cMaxFragData) cMaxFragData = ctemp;
    }
}

char* ConvertARBShader(char* pszShaderSource, bool bIsVertexShader)
{
    const char *start = pszShaderSource;
    globals->arb.errorPtr = -1;

    while(start[0] != '!' && start[1] != '!')
    {
        ++start;
        if(start[0] == '\0' || start[1] == '\0')
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
    start += 11;

    while(strncmp(start, bIsVertexShader ? "//GLSLvp" : "//GLSLfp", 8) != 0) ++start;

    unsigned int len = strlen(start);
    char* pszNewShaderSource = new char[len+1];
    memcpy(pszNewShaderSource, start, len);
    pszNewShaderSource[len] = 0;
    return pszNewShaderSource;
}

static std::string newShader;
static size_t temp;
static const std::string strNewLine = "\n";

#define FILL_SHADER_HEADER \
    { \
        /* Shader Header! */ \
        newShader += "#version 300 es\n#define attribute in\n#define varying out\n" \
        "#define texture2D texture\n#define texture3D texture\n#define textureCube texture\n#define texture2DProj textureProj\n"; \
        /* Shader Extensions! */ \
        if(bUsingFragDepthEXT) newShader +=    "#extension GL_EXT_frag_depth : enable\n"; \
        if(bUsingTexture3DEXT) newShader +=    "#extension GL_OES_texture_3D : enable\n"; \
        if(bUsingTextureLODEXT) { if(globals->ext.hasTextureLods) \
        {   newShader += "#extension GL_EXT_shader_texture_lod : enable\n" \
                         "#define texture2DLod texture2DLodEXT\n#define texture2DProjLod texture2DProjLodEXT\n#define textureCubeLod textureCubeLodEXT\n" \
                         "#define texture2DProjGradARB texture2DProjGradEXT\n#define textureCubeGradARB textureCubeGradEXT\n#define texture2DGradARB texture2DGradEXT\n"; \
        } else { \
            newShader += "\n"; \
        }} \
        /* Precisions */ \
        newShader +=                           "precision highp float;precision highp int;precision highp sampler3D;\n"; \
        /* Shader Variables! */ \
        if(cMaxFragData > -1){newShader +=     GLES_fragDataVar_Part1; newShader += std::to_string(++cMaxFragData); newShader += "];\n";} \
        else if(bUsingFragColor) newShader +=  "#define gl_FragColor GLIN_FragColor\nlayout(location=0) out mediump vec4 gl_FragColor;\n"; \
        if(bUsingClipVertex) newShader +=      "#define gl_ClipVertex GLIN_ClipVertex\nvec4 gl_ClipVertex;\n"; \
        if(bUsingC) newShader +=               "#define gl_Color GLIN_FC\n"; \
        if(bUsingC || bUsingFC)   newShader += "#define gl_FrontColor GLIN_FC\nout lowp vec4 GLIN_FC;\n"; \
        if(bUsingBC)   newShader +=            "#define gl_BackColor GLIN_BC\nout lowp vec4 GLIN_BC;\n"; \
        if(bUsingSC) newShader +=              "#define gl_SecondaryColor GLIN_FSC\n"; \
        if(bUsingSC || bUsingFSC) newShader += "#define gl_FrontSecondaryColor GLIN_FSC\nout lowp vec4 GLIN_FSC;\n"; \
        if(bUsingBSC) newShader +=             "#define gl_BackSecondaryColor GLIN_BSC\nout lowp vec4 GLIN_BSC;\n"; \
        if(bUsingFFC) newShader +=             "#define gl_FogFragCoord GLIN_FFC\nout mediump float GLIN_FFC;\n"; \
        if(bUsingMultiTexCoord) newShader +=   GLES_multiTexCoordVars; \
        /* Shader Functions! */ \
        if(bUsesFTransformFn) newShader +=     GLES_ftransformFn; \
        if(bUsesShadow2DFn) newShader +=       GLES_shadow2DFn; \
        if(usesMin) newShader +=               GLES_min_fix; \
        if(usesMax) newShader +=               GLES_max_fix; \
        if(usesClamp) newShader +=             GLES_clamp_fix; \
    }

const char* ConvertShader(char* pszShaderSource, bool bIsVertexShader)
{
    newShader.clear();
    newShader = bIsVertexShader ? "//Vertex shader #" : "//Fragment shader #";
    newShader += std::to_string(shaderId);
    newShader += " has been generated by GLinES\n";
    bool bFoundVersion = !bHasVersionDefinition;
    if(bFoundVersion) FILL_SHADER_HEADER;
    char* pLine = strtok(pszShaderSource, "\n"); char* pPossiblyUselessText = nullptr;
    while(pLine != NULL)
    {
        if(!bFoundVersion)
        {
            if(strstr(pLine, "#version") != NULL)
            {
                bFoundVersion = true;
                FILL_SHADER_HEADER;
            }
            else newShader += pLine + strNewLine;
        }
        else
        {
            pPossiblyUselessText = strstr(pLine, "#");
            if(pPossiblyUselessText != NULL)
            {
                if(strstr(pPossiblyUselessText, "extension") != NULL) goto TRY_TO_CONTINUE;
            }
            if(bUsingMultiTexCoord)
            {
                pPossiblyUselessText = strstr(pLine, "gl_MultiTexCoord");
                if(pPossiblyUselessText != NULL)
                {
                    strcpy(pPossiblyUselessText, "        GLIN_MTC");
                }
            }
            newShader += pLine + strNewLine;
        }
      TRY_TO_CONTINUE:
        pLine = strtok(NULL, "\n");
    }
    shaderLen = newShader.length();
    return newShader.c_str();
}