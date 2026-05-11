#include "GLES.h"

void WRAP(glGetCompressedTexImage(GLenum target, GLint lod, GLvoid *img))
{
    DBG("glGetCompressedTexImage");
}
void WRAP(glPopAttrib())
{
    DBG("glPopAttrib");
}
void WRAP(glPushAttrib(GLbitfield mask))
{
    DBG("glPushAttrib");
}
void WRAP(glClientActiveTexture(GLenum texture))
{
    DBG("glClientActiveTexture");
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
void WRAP(glLogicOp(GLenum opcode))
{
    globals->ff.logicOpMode = opcode;
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
            
        case GL_LIGHTING:
            globals->ff.lightingEnabled = true;
            break;
            
        case GL_NORMALIZE:
            globals->ff.normalizeEnabled = true;
            break;
            
        case GL_FOG:
            globals->ff.fogEnabled = true;
            break;
            
        case 0x0BF1: //GL_LOGIC_OP
            globals->ff.logicOpEnabled = true;
            break;
            
        case GL_CLIP_PLANE0: case GL_CLIP_PLANE1: case GL_CLIP_PLANE2:
        case GL_CLIP_PLANE3: case GL_CLIP_PLANE4: case GL_CLIP_PLANE5:
            globals->ff.clipPlaneOn[cap - GL_CLIP_PLANE0] = true;
            break;
            
        case GL_LIGHT0: case GL_LIGHT1: case GL_LIGHT2: case GL_LIGHT3:
        case GL_LIGHT4: case GL_LIGHT5: case GL_LIGHT6: case GL_LIGHT7:
            globals->ff.lightEnabled[cap - GL_LIGHT0] = true;
            break;
            
        case GL_TEXTURE_2D:
            globals->render.texture = true;
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
            
        case GL_LIGHTING:
            globals->ff.lightingEnabled = false;
            break;
            
        case GL_NORMALIZE:
            globals->ff.normalizeEnabled = false;
            break;
            
        case GL_FOG:
            globals->ff.fogEnabled = false;
            break;
            
        case 0x0BF1: //GL_LOGIC_OP
            globals->ff.logicOpEnabled = false;
            break;
            
        case GL_CLIP_PLANE0: case GL_CLIP_PLANE1: case GL_CLIP_PLANE2:
        case GL_CLIP_PLANE3: case GL_CLIP_PLANE4: case GL_CLIP_PLANE5:
            globals->ff.clipPlaneOn[cap - GL_CLIP_PLANE0] = false;
            break;
            
        case GL_LIGHT0: case GL_LIGHT1: case GL_LIGHT2: case GL_LIGHT3:
        case GL_LIGHT4: case GL_LIGHT5: case GL_LIGHT6: case GL_LIGHT7:
            globals->ff.lightEnabled[cap - GL_LIGHT0] = false;
            break;
            
        case GL_TEXTURE_2D:
            globals->render.texture = false;
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
            
        case GL_LIGHTING:
            return globals->ff.lightingEnabled;
            
        case GL_NORMALIZE:
            return globals->ff.normalizeEnabled;
            
        case GL_FOG:
            return globals->ff.fogEnabled;
            
        case 0x0BF1: //GL_LOGIC_OP
            return globals->ff.logicOpEnabled;
            
        case GL_CLIP_PLANE0: case GL_CLIP_PLANE1: case GL_CLIP_PLANE2:
        case GL_CLIP_PLANE3: case GL_CLIP_PLANE4: case GL_CLIP_PLANE5:
            return globals->ff.clipPlaneOn[cap - GL_CLIP_PLANE0];
            
        case GL_LIGHT0: case GL_LIGHT1: case GL_LIGHT2: case GL_LIGHT3:
        case GL_LIGHT4: case GL_LIGHT5: case GL_LIGHT6: case GL_LIGHT7:
            return globals->ff.lightEnabled[cap - GL_LIGHT0];
            
        case GL_TEXTURE_2D:
            return globals->render.texture;
            
        // TODO: more

        default:
            return glIsEnabled(cap);
    }
}

void WRAP(glClear(GLbitfield mask))
{
    glClear((mask & (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT)));
}

void WRAP(glGetFloatv(GLenum pname, GLfloat* data))
{
    switch(pname)
    {
        default:
            glGetFloatv(pname, data);
            break;
        
        case GL_MODELVIEW_MATRIX:
            memcpy(data, globals->matrix.modelview.Current().data(), 16 * sizeof(GLfloat));
            break;
        
        case GL_PROJECTION_MATRIX:
            memcpy(data, globals->matrix.projection.Current().data(), 16 * sizeof(GLfloat));
            break;
        
        case GL_TEXTURE_MATRIX:
            memcpy(data, globals->matrix.texture.Current().data(), 16 * sizeof(GLfloat));
            break;
            
        // TODO: more
    }
}

void WRAP(glGetDoublev(GLenum pname, GLdouble* data))
{
    int num = 0;
    GLfloat ret[16];
    switch(pname)
    {
        case GL_MODELVIEW_MATRIX:
        case GL_PROJECTION_MATRIX:
        case GL_TEXTURE_MATRIX:
            num = 4;
            break;
            
        // TODO: more
    }
    
    WRAP(glGetFloatv(pname, ret));
    for(int i = 0; i < num; ++i) data[i] = ret[i];
}

void WRAP(glPixelStoref(GLenum pname, GLfloat param))
{
    glPixelStorei(pname, param);
}

void WRAP(glDrawArrays(GLenum mode, GLint first, GLsizei count))
{
    if(globals->gl.lastPolygonMode != 0) mode = GL_LINE_STRIP;
    WRAPCALL(glDrawArrays(mode, first, count));
}

void WRAP(glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices))
{
    if(globals->gl.lastPolygonMode != 0) mode = GL_LINE_STRIP;
    WRAPCALL(glDrawElements(mode, count, type, indices));
}

void WRAP(glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices))
{
    if(globals->gl.lastPolygonMode != 0) mode = GL_LINE_STRIP;
    WRAPCALL(glDrawRangeElements(mode, start, end, count, type, indices));
}
