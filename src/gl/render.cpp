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

static GLuint latestFramebuffer = 0;
void WRAP(glBindFramebuffer(GLenum target, GLuint framebuffer))
{
    glBindFramebuffer(target, framebuffer);
    latestFramebuffer = framebuffer;
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
    glBindFramebuffer(target, latestFramebuffer);
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
    programs = new GLuint[n];
    GLuint freeId = 1;
    program_arb_t* program;
    while(i < n)
    {
        while(globals->programsARB[freeId] != NULL) ++freeId;
        program = new program_arb_t;
        program->id = freeId;
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
            delete program;
            globals->programsARB[programs[i]] = NULL;
        }
        ++i;
    }
}

void WRAP(glVertex3f(GLfloat x, GLfloat y, GLfloat z))
{
    DBG("glVertex3f");
}

void WRAP(glBindTexture(GLenum target, GLuint texture))
{
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