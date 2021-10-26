#include "GLES.h"
#include "gl_render.h"
#include "globals.h"

void GLIN_Wrap_glBegin(GLenum mode)
{
    DBG("glBegin(0x%X)", mode);
    globals->lastPrimitiveMode = mode;
    globals->render.begin = true;
}

void GLIN_Wrap_glEnd()
{
    DBG("glEnd");
    globals->render.begin = false;
}

void GLIN_Wrap_glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    DBG("glColor4f");
    static vector4_t& clr = globals->render.color;
    clr.x = red;
    clr.y = green;
    clr.z = blue;
    clr.a = alpha;
}

#define _1_DIV_255   0.00392156863f
void GLIN_Wrap_glColor4sv(const GLshort* v)
{
    DBG("glColor4sv");
    static vector4_t& clr = globals->render.color;
    clr.x = _1_DIV_255 * v[0];
    clr.y = _1_DIV_255 * v[1];
    clr.z = _1_DIV_255 * v[2];
    clr.a = _1_DIV_255 * v[3];
}

void GLIN_Wrap_glColorMaskIndexed(GLuint framebuffer, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    DBG("glColorMaskIndexed");
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer); // Any way for GL_DRAW_FRAMEBUFFER ?
    glColorMask(red, green, blue, alpha);
    //glBindFramebuffer(GL_FRAMEBUFFER, oldf);
}

void GLIN_Wrap_glBindProgram(GLenum target, GLuint program)
{
    DBG("glBindProgram");
    switch(target)
    {
        case 0x8620: // GL_VERTEX_PROGRAM_ARB:
            break;

        case 0x8804: // GL_FRAGMENT_PROGRAM_ARB:
            break;
    }
}

void GLIN_Wrap_glGenPrograms(GLsizei n, GLuint *programs)
{
    DBG("glGenPrograms");
    //static int i; int = 0;
    //programs = new GLuint[n];
    //while(i < n)
    //{
    //    
    //    ++i;
    //}
}

void GLIN_Wrap_glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
    DBG("glVertex3f");
}