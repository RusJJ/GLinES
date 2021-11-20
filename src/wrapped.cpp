#include "GLES.h"

void WRAP(glClipPlane(GLenum plane, const GLdouble *equation))
{
    DBG("glClipPlane");
}
void WRAP(glGetCompressedTexImage(GLenum target, GLint lod, GLvoid *img))
{
    DBG("glGetCompressedTexImage");
}
void WRAP(glTexCoord2f(GLfloat s, GLfloat t))
{
    DBG("glTexCoord2f");
}
void WRAP(glPolygonMode(GLenum face, GLenum mode))
{
    DBG("glPolygonMode");
}
void WRAP(glPopAttrib())
{
    DBG("glPopAttrib");
}
void WRAP(glPushAttrib(GLbitfield mask))
{
    DBG("glPushAttrib");
}
void WRAP(glEnableClientState(GLenum array))
{
    DBG("glEnableClientState");
}
void WRAP(glDisableClientState(GLenum array))
{
    DBG("glDisableClientState");
}
void WRAP(glClientActiveTexture(GLenum texture))
{
    DBG("glClientActiveTexture");
}
void WRAP(glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer))
{
    DBG("glVertexPointer");
}
void WRAP(glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer))
{
    DBG("glTexCoordPointer");
}
void WRAP(glProgramEnvParameters4fv(GLenum target, GLuint index, GLsizei count, const GLfloat *params))
{
    DBG("glProgramEnvParameters4fv");
}
void WRAP(glEnableIndexed(GLenum target, GLuint index))
{
    DBG("glEnableIndexed");
}
void WRAP(glDisableIndexed(GLenum target, GLuint index))
{
    DBG("glDisableIndexed");
}
void WRAP(glGetBooleanIndexedv(GLenum target, GLuint index, GLboolean *data))
{
    DBG("glGetBooleanIndexedv");
}
void WRAP(glPopClientAttrib())
{
    DBG("glPopClientAttrib");
}
void WRAP(glPushClientAttrib(GLbitfield mask))
{
    DBG("glPushClientAttrib");
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
            *params = (target == 0x8620) ? MAX_VTX_PROG_LOC_PARAMS : MAX_FRG_PROG_LOC_PARAMS; // GL_VERTEX_PROGRAM_ARB
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

void WRAP(glGetIntegerv( GLenum pname, GLint* params))
{
    switch(pname)
    {
        case 0x0C01: //GL_DRAW_BUFFER:
            *params = GL_FRONT;
            break;

        case 0x84EF: //GL_TEXTURE_COMPRESSION_HINT:
            *params = GL_DONT_CARE;
            break;

        case 0x862F: //GL_MAX_PROGRAM_MATRICES_ARB:
            *params = MAX_ARB_MATRIX;
            break;

        case 0x864B: //GL_PROGRAM_ERROR_POSITION_ARB:
            *params = globals->arb.errorPtr;
            break;

        default:
            glGetIntegerv( pname, params );
            break;
    }
}

void WRAP(glEnable(GLenum cap))
{
    switch(cap)
    {
        case 0x8620: //GL_VERTEX_PROGRAM_ARB:
            globals->gl.enabledVertProgARB = true;
            break;
            
        case 0x8804: //GL_FRAGMENT_PROGRAM_ARB:
            globals->gl.enabledFragProgARB = true;
            break;

        default:
            glEnable(cap);
            break;
    }
}

void WRAP(glDisable(GLenum cap))
{
    switch(cap)
    {
        case 0x8620: //GL_VERTEX_PROGRAM_ARB:
            globals->gl.enabledVertProgARB = false;
            break;
            
        case 0x8804: //GL_FRAGMENT_PROGRAM_ARB:
            globals->gl.enabledFragProgARB = false;
            break;

        default:
            glDisable(cap);
            break;
    }
}

GLboolean WRAP(glIsEnabled(GLenum cap))
{
    switch(cap)
    {
        case 0x8620: //GL_VERTEX_PROGRAM_ARB:
            return globals->gl.enabledVertProgARB;
            
        case 0x8804: //GL_FRAGMENT_PROGRAM_ARB:
            return globals->gl.enabledFragProgARB;

        default:
            return glIsEnabled(cap);
    }
}

void WRAP(glClearColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a))
{
    glClearColor(0.5f, 0.5f, 0.5f, a);
}

void WRAP(glClear(GLbitfield mask))
{
    glClear((mask & (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)));
}