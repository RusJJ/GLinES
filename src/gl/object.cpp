#include "gl_object.h"
#include "gl_shader.h"

void WRAP(glGetInfoLog(GLuint obj, GLsizei maxLength, GLsizei *length, GLchar *infoLog))
{
    if(glIsShader(obj))
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
    if(glIsShader(obj))
    {
        WRAP(glDeleteShader(obj));
    }
    else
    {
        glDeleteProgram(obj);
    }
}

void WRAP(glGetObjectParameterfv(GLuint obj, GLenum pname, GLfloat *params))
{
    GLint p[4];
    if(glIsShader(obj))
    {
        glGetShaderiv(obj, pname, p);
    }
    else
    {
        glGetProgramiv(obj, pname, p);
    }
    params[0] = p[0];
}

void WRAP(glGetObjectParameteriv(GLuint obj, GLenum pname, GLint *params))
{
    if(pname == 0x8B4E) { *params = glIsShader(obj) ? 0x8B48 : 0x8B40; return; }
    if(pname == 0x8B4F) pname = GL_SHADER_TYPE;

    if(glIsShader(obj))
    {
        glGetShaderiv(obj, pname, params);
    }
    else
    {
        glGetProgramiv(obj, pname, params);
    }


}

GLboolean WRAP(glIsProgram(GLuint program))
{
    return (globals->programsARB[program] != NULL) ? GL_TRUE : GL_FALSE;
}

void WRAP(glProgramString(GLenum target, GLenum format, GLsizei len, const GLvoid *string)) // ARB only
{
    if(len < 0 || (len && !string)) { SetError(GL_INVALID_VALUE); return; }
    if(format != 0x8875) { SetError(GL_INVALID_ENUM); return; }
    std::string input((const char*)string, (size_t)len);
    program_arb_t* prog = NULL;
    char* pNewShader = NULL;
    switch(target)
    {
        case 0x8620: // GL_VERTEX_PROGRAM_ARB:
            prog = globals->arb.activeVert;
            pNewShader = ConvertARBShader(input.c_str(), true);
            break;

        case 0x8804: // GL_FRAGMENT_PROGRAM_ARB:
            prog = globals->arb.activeFrag;
            pNewShader = ConvertARBShader(input.c_str(), false);
            break;
    }

    if(prog == NULL || pNewShader == NULL)
    {
        delete[] pNewShader;
        return;
    }

    if(prog->src != NULL) delete[] prog->src;
    prog->src = new char[len + 1];
    memcpy(prog->src, string, len);
    prog->src[len] = 0;

    glShaderSource(prog->shader, 1, (const GLchar**)&pNewShader, NULL);
    WRAP(glCompileShader(prog->shader));
    delete[] pNewShader;
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
            globals->programsARB[program] = prog;
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
    int i = 0;
    GLuint freeId = 1;
    program_arb_t* program = NULL;
    while(i < n)
    {
        while(freeId < MAX_COUNT_OF_SAVED_ARB_PROGS && globals->programsARB[freeId] != NULL) ++freeId;
        if(freeId >= MAX_COUNT_OF_SAVED_ARB_PROGS) break; // out of slots
        program = new program_arb_t;
        program->id = freeId;
        program->type = 0;
        globals->programsARB[freeId] = program;

        programs[i] = freeId;
        ++i;
        ++freeId;
    }
}

void WRAP(glDeletePrograms(GLsizei n, const GLuint *programs))
{
    int i = 0;
    program_arb_t* program;
    while(i < n)
    {
        if((program = globals->programsARB[programs[i]]) != NULL)
        {
            if(globals->arb.activeVert == program) globals->arb.activeVert = nullptr;
            if(globals->arb.activeFrag == program) globals->arb.activeFrag = nullptr;
            glDeleteShader(program->shader);
            if(program->src) delete[] program->src;
            delete program;
            globals->programsARB[programs[i]] = NULL;
        }
        ++i;
    }
}

void WRAP(glProgramEnvParameters4fv(GLenum target, GLuint index, GLsizei count, const GLfloat *params))
{
    DBG("glProgramEnvParameters4fv");
}

void WRAP(glGetProgramiv(GLenum target,GLenum pname,GLint *params)) // ARB only
{
    program_arb_t* prog = NULL;
    switch(target)
    {
        case 0x8620: //GL_VERTEX_PROGRAM_ARB:
            prog = globals->arb.activeVert;
            break;
        case 0x8804: //GL_FRAGMENT_PROGRAM_ARB:
            prog = globals->arb.activeFrag;
            break;
    }
    switch(pname)
    {
        case 0x8627: //GL_PROGRAM_LENGTH_ARB:
            *params = (prog != NULL && prog->src != NULL) ? strlen(prog->src) : 0;
            break;

        case 0x8876: //GL_PROGRAM_FORMAT_ARB:
            *params = 0x8875; //GL_PROGRAM_FORMAT_ASCII_ARB;
            break;

        case 0x8677: //GL_PROGRAM_BINDING_ARB:
            *params = (prog != NULL) ? prog->shader : 0;
            break;

        case 0x88B4: //GL_MAX_PROGRAM_LOCAL_PARAMETERS_ARB:
            *params = (target == 0x8620) ? MAX_VTX_PROG_LOC_PARAMS : MAX_FRG_PROG_LOC_PARAMS;
            break;

        case 0x88B5: //GL_MAX_PROGRAM_ENV_PARAMETERS_ARB:
            *params = (target == 0x8620) ? MAX_VTX_PROG_ENV_PARAMS : MAX_FRG_PROG_ENV_PARAMS;
            break;

        case 0x88AF: //GL_MAX_PROGRAM_NATIVE_ATTRIBS_ARB:
        case 0x88AD: //GL_MAX_PROGRAM_ATTRIBS_ARB:
            *params = MAX_ARB_ATTRIBUTES;
            break;
            
        case 0x88A3: //GL_MAX_PROGRAM_NATIVE_INSTRUCTIONS_ARB:
        case 0x88A1: //GL_MAX_PROGRAM_INSTRUCTIONS_ARB:
            *params = 4096;
            break;

        case 0x88A7: //GL_MAX_PROGRAM_NATIVE_TEMPORARIES_ARB:
        case 0x88A5: //GL_MAX_PROGRAM_TEMPORARIES_ARB:
            *params = 64;
            break;

        case 0x88AB: //GL_MAX_PROGRAM_NATIVE_PARAMETERS_ARB:
        case 0x88A9: //GL_MAX_PROGRAM_PARAMETERS_ARB:
            *params = 64;
            break;

        case 0x88B3: //GL_MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB:
        case 0x88B1: //GL_MAX_PROGRAM_ADDRESS_REGISTERS_ARB:
            *params = 4;
            break;

        case 0x880E: //GL_MAX_PROGRAM_NATIVE_ALU_INSTRUCTIONS_ARB:
        case 0x880B: //GL_MAX_PROGRAM_ALU_INSTRUCTIONS_ARB:
            *params = 1024;
            break;

        case 0x880F: //GL_MAX_PROGRAM_NATIVE_TEX_INSTRUCTIONS_ARB:
        case 0x880C: //GL_MAX_PROGRAM_TEX_INSTRUCTIONS_ARB:
            *params = 32;
            break;

        case 0x880D: //GL_MAX_PROGRAM_TEX_INDIRECTIONS_ARB:
        case 0x8810: //GL_MAX_PROGRAM_NATIVE_TEX_INDIRECTIONS_ARB:
            *params = 8;
            break;

        case 0x88B6: //GL_PROGRAM_UNDER_NATIVE_LIMITS_ARB:
            *params = 1;
            break;
    }
}
