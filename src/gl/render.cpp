#include "GLES.h"
#include "gl_render.h"
#include "gl_shader.h"
#include "globals.h"

void WRAP(glBegin(GLenum mode))
{
    globals->lastPrimitiveMode = mode;
    globals->render.begin = true;
}

void WRAP(glEnd())
{
    globals->render.begin = false;
}

void WRAP(glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha))
{
    static vector4_t& clr = globals->render.color;
    clr.x = red;
    clr.y = green;
    clr.z = blue;
    clr.w = alpha;
}

const double __1DIV32767 = ( 1.0 / 32767.0 );
void WRAP(glColor4sv(const GLshort* v))
{
    vector4_t& clr = globals->render.color;
    clr.x = __1DIV32767 * v[0];
    clr.y = __1DIV32767 * v[1];
    clr.z = __1DIV32767 * v[2];
    clr.w = __1DIV32767 * v[3];
}

void WRAP(glColorMaskIndexed(GLuint framebuffer, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha))
{
    glColorMaski(framebuffer, red, green, blue, alpha);
}

void WRAP(glVertex3f(GLfloat x, GLfloat y, GLfloat z))
{
    DBG("glVertex3f");
}

void WRAP(glBindTexture(GLenum target, GLuint texture))
{
    globals->gl.activeTexture = globals->textures[texture];
    switch(target)
    {
        case 0x0DE0: //GL_TEXTURE_1D:
        case 0x84F5: //GL_TEXTURE_RECTANGLE_ARB:
            glBindTexture(GL_TEXTURE_2D, texture);
            break;

        default:
            glBindTexture(target, texture);
            break;
    }
}

void WRAP(glDrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, void *indices, GLint basevertex))
{
    if(!basevertex) return glDrawRangeElements(mode, start, end, count, type, indices);
    glDrawRangeElementsBaseVertex(mode, start, end, count, type, indices, basevertex);
}

void WRAP(glClipPlane(GLenum plane, const GLdouble *equation))
{
    //MatrixToVector(, equation, globals->gl.clipPlanes[plane - GL_CLIP_PLANE0]);
}

void WRAP(glClipPlanef(GLenum plane, const GLfloat *equation))
{
    //globals->gl.clipPlanes[plane - GL_CLIP_PLANE0];
}

void WRAP(glClearDepth(double value))
{
    glClearDepthf((float)value);
}

void WRAP(glDepthRange(double n, double f))
{
    glDepthRangef((float)n, (float)f);
}

void WRAP(glVertexAttribLPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer))
{
    // TODO: glVertexAttribLPointer
    MSG("glVertexAttribLPointer(...)");
}

void WRAP(glVertexAttrib3d(GLuint index, GLdouble x, GLdouble y, GLdouble z))
{
    glVertexAttrib3f(index, x, y, z);
}

void WRAP(glMultiDrawArrays(GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount))
{
    for(GLsizei i = 0; i < drawcount; ++i)
    {
        glDrawArrays(mode, first[i], count[i]);
    }
}

void WRAP(glMultiDrawElements(GLenum mode, const GLsizei* count, GLenum type, const void* const* indices, GLsizei drawcount))
{
    for(GLsizei i = 0; i < drawcount; ++i)
    {
        glDrawElements(mode, count[i], type, indices[i]);
    }
}

void WRAP(glMultiDrawElementsBaseVertex(GLenum mode, const GLsizei* count, GLenum type, const void* const* indices, GLsizei drawcount, const GLint* basevertex))
{
    for(GLsizei i = 0; i < drawcount; ++i)
    {
        glDrawElementsBaseVertex(mode, count[i], type, indices[i], basevertex[i]);
    }
}
