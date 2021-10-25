#include "gl_object.h"

void GLIN_Wrap_glGetInfoLog(GLuint obj, GLsizei maxLength, GLsizei *length, GLchar *infoLog)
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

void GLIN_Wrap_glDeleteObject(GLuint obj)
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

void GLIN_Wrap_glGetObjectParameterfv(GLuint obj, GLenum pname, GLfloat *params)
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

void GLIN_Wrap_glGetObjectParameteriv(GLuint obj, GLenum pname, GLint *params)
{
    if(globals->shaders[obj] != NULL)
    {
        glGetShaderiv(obj, pname, params);
    }
    else
    {
        glGetProgramiv(obj, pname, params);
    }

    // GL4ES
    if(pname == GL_INFO_LOG_LENGTH) (*params)++;
}