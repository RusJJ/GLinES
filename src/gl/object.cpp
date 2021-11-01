#include "gl_object.h"
#include "gl_shader_conv.h"

void WRAP(glGetInfoLog(GLuint obj, GLsizei maxLength, GLsizei *length, GLchar *infoLog))
{
    if(globals->shaders[obj] != NULL)
    {
        glGetShaderInfoLog(obj, maxLength, length, infoLog);
    }
    else
    {
        glGetProgramInfoLog(obj, maxLength, length, infoLog);
    }
}

void WRAP(glDeleteObject(GLuint obj))
{
    if(globals->shaders[obj] != NULL)
    {
        glDeleteShader(obj);
    }
    else
    {
        glDeleteProgram(obj);
    }
}

void WRAP(glGetObjectParameterfv(GLuint obj, GLenum pname, GLfloat *params))
{
    GLint p[4];
    if(globals->shaders[obj] != NULL)
    {
        glGetShaderiv(obj, pname, p);
    }
    else
    {
        glGetProgramiv(obj, pname, p);
    }

    // GL4ES
    params[0] = p[0];
}

void WRAP(glGetObjectParameteriv(GLuint obj, GLenum pname, GLint *params))
{
    if(globals->shaders[obj] != NULL)
    {
        glGetShaderiv(obj, pname, params);
        __android_log_print(ANDROID_LOG_INFO, "SRCENG", "Shader requested glGetObjectParameterivARB");
    }
    else
    {
        glGetProgramiv(obj, pname, params);
        __android_log_print(ANDROID_LOG_INFO, "SRCENG", "Program requested glGetObjectParameterivARB");
    }

    // GL4ES
    if(pname == GL_INFO_LOG_LENGTH) (*params)++;
}

GLboolean WRAP(glIsProgram(GLuint program))
{
    return (globals->programsARB[program] != NULL) ? GL_TRUE : GL_FALSE;
}

void WRAP(glProgramString(GLenum target, GLenum format, GLsizei len, const GLvoid *string)) // ARB only
{
    program_arb_t* prog = NULL;
    char* pNewShader;
    switch(target)
    {
        case 0x8620: // GL_VERTEX_PROGRAM_ARB:
            prog = globals->arb.activeVert;
            pNewShader = ConvertARBShader((char*)string, true);
            break;

        case 0x8804: // GL_FRAGMENT_PROGRAM_ARB:
            prog = globals->arb.activeFrag;
            pNewShader = ConvertARBShader((char*)string, false);
            break;
    }
    if(prog->src != NULL) delete[] prog->src;
    prog->src = pNewShader;
    glShaderSource(prog->shader, 1, (const GLchar**)&(prog->src), NULL);
    WRAP(glCompileShader(prog->shader));
}