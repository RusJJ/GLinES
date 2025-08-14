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

GLenum WRAP(glCheckFramebufferStatus(GLenum target))
{
    GLenum ret = glCheckFramebufferStatus(target);
    switch(ret)
    {
        case GL_FRAMEBUFFER_COMPLETE:
            MSG("glCheckFramebufferStatus(0x%X) = GL_FRAMEBUFFER_COMPLETE", target);
            break;
            
        case GL_FRAMEBUFFER_UNDEFINED:
            MSG("glCheckFramebufferStatus(0x%X) = GL_FRAMEBUFFER_UNDEFINED", target);
            break;
            
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            MSG("glCheckFramebufferStatus(0x%X) = GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT", target);
            break;
            
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            MSG("glCheckFramebufferStatus(0x%X) = GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT", target);
            break;
            
        case GL_FRAMEBUFFER_UNSUPPORTED:
            MSG("glCheckFramebufferStatus(0x%X) = GL_FRAMEBUFFER_UNSUPPORTED", target);
            break;
            
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            MSG("glCheckFramebufferStatus(0x%X) = GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE", target);
            break;

        default:
            MSG("glCheckFramebufferStatus(0x%X) = UNKNOWN", target);
            break;
    }
    return ret;
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