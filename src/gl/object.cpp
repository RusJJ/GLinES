#include "gl_object.h"
#include "gl_shader.h"

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
    if(pname == 0x8B82)
    {
        *params = GL_TRUE;
        return;
    }

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
    prog->src = new char[len + 1]; memcpy(prog->src, string, len);
    glShaderSource(prog->shader, 1, (const GLchar**)&pNewShader, NULL);
    WRAP(glCompileShader(prog->shader));
}

void WRAP(glGetProgramString(GLenum target, GLenum pname, const void *string))
{
    if(pname != 0x8628) //GL_PROGRAM_STRING_ARB
    {
        ERR("glGetProgramString(0x%X); is unknown!", target);
        return;
    }
    program_arb_t* prog = NULL;
    switch(target)
    {
        case 0x8620: // GL_VERTEX_PROGRAM_ARB:
            prog = globals->arb.activeVert;
            break;

        case 0x8804: // GL_FRAGMENT_PROGRAM_ARB:
            prog = globals->arb.activeFrag;
            break;
    }
    if(prog != NULL && prog->src) strcpy((char*)string, prog->src);
}

void WRAP(glBindProgram(GLenum target, GLuint program))
{
    program_arb_t* prog = NULL;
    if(program > 0)
    {
        prog = globals->programsARB[program];
        if(!prog)
        {
            prog = new program_arb_t;
            prog->id = program;
            prog->type = 0;
        }
    }

    switch(target)
    {
        case 0x8620: // GL_VERTEX_PROGRAM_ARB:
            if(!program)
            {
                globals->arb.activeVert = NULL;
                return;
            }
            globals->arb.activeVert = prog;
            if(!prog->type)
            {
                prog->type = target;
                prog->vertexShader = true;
                prog->shader = WRAP(glCreateShader(GL_VERTEX_SHADER));
            }
            break;

        case 0x8804: // GL_FRAGMENT_PROGRAM_ARB:
            if(!program)
            {
                globals->arb.activeFrag = NULL;
                return;
            }
            globals->arb.activeFrag = prog;
            if(!prog->type)
            {
                prog->type = target;
                prog->vertexShader = false;
                prog->shader = WRAP(glCreateShader(GL_FRAGMENT_SHADER));
            }
            break;
    }
}

void WRAP(glGenPrograms(GLsizei n, GLuint *programs))
{
    static int i; i = 0;
    static GLuint freeId; freeId = 1;
    program_arb_t* program = NULL;
    while(i < n)
    {
        while(globals->programsARB[freeId] != NULL) ++freeId;
        program = new program_arb_t;
        program->id = freeId;
        program->type = 0;
        globals->programsARB[freeId] = program;

        programs[i] = freeId;
        ++i;
    }
}

void WRAP(glDeletePrograms(GLsizei n, const GLuint *programs))
{
    static int i; i = 0;
    program_arb_t* program;
    while(i < n)
    {
        if((program = globals->programsARB[programs[i]]) != NULL)
        {
            glDeleteShader(program->shader);
            delete[] program->src;
            delete program;
            globals->programsARB[programs[i]] = NULL;
        }
        ++i;
    }
}