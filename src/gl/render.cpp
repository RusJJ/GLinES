#include "GLES.h"
#include "gl_render.h"
#include "gl_shader_conv.h"
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

#define __1DIV255 0.00392156863f
void WRAP(glColor4sv(const GLshort* v))
{
    static vector4_t& clr = globals->render.color;
    clr.x = __1DIV255 * v[0];
    clr.y = __1DIV255 * v[1];
    clr.z = __1DIV255 * v[2];
    clr.w = __1DIV255 * v[3];
}

GLuint nCurrentFB = 0;
void WRAP(glBindFramebuffer(GLenum target, GLuint framebuffer))
{
    nCurrentFB = framebuffer;
    glBindFramebuffer(target, framebuffer);
}

// GL4ES
void WRAP(glColorMaskIndexed(GLuint framebuffer, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha))
{
    static GLint drawFboId = 0, readFboId = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFboId);
    GLenum target = (drawFboId == readFboId)?GL_FRAMEBUFFER:GL_DRAW_FRAMEBUFFER;

    glBindFramebuffer(target, framebuffer);
    glColorMask(red, green, blue, alpha);
    glBindFramebuffer(target, nCurrentFB);
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