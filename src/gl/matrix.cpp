#include "GLES.h"
#include "glhelper.h"
#include <cmath>

GLINAPI void WRAP(glMatrixMode(GLenum mode))
{
    DLREC(WRAP(glMatrixMode(mode)));
    if(mode != GL_MODELVIEW && mode != GL_PROJECTION && mode != GL_TEXTURE) { SetError(GL_INVALID_ENUM); return; }
    globals->matrix.mode = mode;
}
GLINAPI EXPORT void glMatrixMode(GLenum mode) ALIASWRAP(glMatrixMode);

GLINAPI void WRAP(glLoadIdentity())
{
    DLREC(WRAP(glLoadIdentity()));
    globals->matrix.Current() = matrix4_t::Identity();
}

GLINAPI void WRAP(glPushMatrix())
{
    // https://registry.khronos.org/OpenGL-Refpages/gl2.1/xhtml/glPushMatrix.xml
    DLREC(WRAP(glPushMatrix()));
    matrix_stack_t& stack = globals->matrix.mode == GL_PROJECTION ? globals->matrix.projection : globals->matrix.mode == GL_TEXTURE ? globals->matrix.texture : globals->matrix.modelview;
    if(stack.pos == MAX_COUNT_OF_MATRIX_STACK - 1) { SetError(GL_STACK_OVERFLOW); return; }
    matrix4_t& current = globals->matrix.Current();
    globals->matrix.Push();
    globals->matrix.Current() = current;
}

GLINAPI void WRAP(glPopMatrix())
{
    DLREC(WRAP(glPopMatrix()));
    matrix_stack_t& stack = globals->matrix.mode == GL_PROJECTION ? globals->matrix.projection : globals->matrix.mode == GL_TEXTURE ? globals->matrix.texture : globals->matrix.modelview;
    if(stack.pos == 0) { SetError(GL_STACK_UNDERFLOW); return; }
    globals->matrix.Pop();
}

GLINAPI void WRAP(glLoadMatrixf(const GLfloat* m))
{
    DLREC_MAT4(m, WRAP(glLoadMatrixf(_m.data())));
    memcpy(&globals->matrix.Current(), m, sizeof(matrix4_t));
}

GLINAPI void WRAP(glLoadMatrixd(const GLdouble *m))
{
    GLfloat f[16];
    for(int i = 0; i < 16; i++) f[i] = (GLfloat)m[i];
    WRAP(glLoadMatrixf(f));
}

GLINAPI void WRAP(glMultMatrixf(const GLfloat *m))
{
    DLREC_MAT4(m, WRAP(glMultMatrixf(_m.data())));
    globals->matrix.Current() *= m;
}

GLINAPI void WRAP(glMultMatrixd(const GLdouble *m))
{
    GLfloat f[16];
    for(int i = 0; i < 16; i++) f[i] = (GLfloat)m[i];
    WRAP(glMultMatrixf(f));
}

GLINAPI void WRAP(glTranslatef(GLfloat x, GLfloat y, GLfloat z))
{
    DLREC(WRAP(glTranslatef(x, y, z)));
    GLfloat m[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, x,y,z,1};
    globals->matrix.Current() *= m;
}

GLINAPI void WRAP(glTranslated(GLdouble x, GLdouble y, GLdouble z))
{
    WRAP(glTranslatef((GLfloat)x, (GLfloat)y, (GLfloat)z));
}

GLINAPI void WRAP(glScalef(GLfloat x, GLfloat y, GLfloat z))
{
    DLREC(WRAP(glScalef(x, y, z)));
    GLfloat m[16] = {x,0,0,0, 0,y,0,0, 0,0,z,0, 0,0,0,1};
    globals->matrix.Current() *= m;
}

GLINAPI void WRAP(glScaled(GLdouble x, GLdouble y, GLdouble z))
{
    WRAP(glScalef((GLfloat)x, (GLfloat)y, (GLfloat)z));
}

GLINAPI void WRAP(glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z))
{
    DLREC(WRAP(glRotatef(angle, x, y, z)));
    float c = cosf(angle * M_PI / 180.0f), s = sinf(angle * M_PI / 180.0f), t = 1.0f - c;
    float len = sqrtf(x*x + y*y + z*z);
    if (len > 0.0f)
    {
        x /= len; y /= len; z /= len;
        GLfloat m[16] = {
            t*x*x+c,   t*x*y+s*z, t*x*z-s*y, 0,
            t*x*y-s*z, t*y*y+c,   t*y*z+s*x, 0,
            t*x*z+s*y, t*y*z-s*x, t*z*z+c,   0,
            0,         0,         0,          1
        };
        globals->matrix.Current() *= m;
    }
}

GLINAPI void WRAP(glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z))
{
    WRAP(glRotatef((GLfloat)angle, (GLfloat)x, (GLfloat)y, (GLfloat)z));
}

GLINAPI void WRAP(glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal))
{
    DLREC(WRAP(glFrustum(left, right, bottom, top, nearVal, farVal)));
    if(left == right || bottom == top || nearVal == farVal) { SetError(GL_INVALID_VALUE); return; }
    GLfloat m[16] = { 0.0f };
    m[0]  = (GLfloat)(2.0 * nearVal / (right - left));
    m[5]  = (GLfloat)(2.0 * nearVal / (top - bottom));
    m[8]  = (GLfloat)((right + left) / (right - left));
    m[9]  = (GLfloat)((top + bottom) / (top - bottom));
    m[10] = (GLfloat)(-(farVal + nearVal) / (farVal - nearVal));
    m[11] = -1.0f;
    m[14] = (GLfloat)(-(2.0 * farVal * nearVal) / (farVal - nearVal));
    globals->matrix.Current() *= m;
}

GLINAPI void WRAP(glFrustumf(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat nearVal, GLfloat farVal))
{
    WRAP(glFrustum(left, right, bottom, top, nearVal, farVal));
}

GLINAPI void WRAP(glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal))
{
    DLREC(WRAP(glOrtho(left, right, bottom, top, nearVal, farVal)));
    if(left == right || bottom == top || nearVal == farVal) { SetError(GL_INVALID_VALUE); return; }
    GLfloat m[16] = { 0.0f };
    m[0]  = (GLfloat)(2.0 / (right - left));
    m[5]  = (GLfloat)(2.0 / (top - bottom));
    m[10] = (GLfloat)(-2.0 / (farVal - nearVal));
    m[12] = (GLfloat)(-(right + left) / (right - left));
    m[13] = (GLfloat)(-(top + bottom) / (top - bottom));
    m[14] = (GLfloat)(-(farVal + nearVal) / (farVal - nearVal));
    m[15] = 1.0f;
    globals->matrix.Current() *= m;
}

GLINAPI void WRAP(glOrthof(GLfloat left, GLfloat right, GLfloat bottom, GLfloat top, GLfloat nearVal, GLfloat farVal))
{
    WRAP(glOrtho(left, right, bottom, top, nearVal, farVal));
}

GLINAPI void WRAP(glLoadTransposeMatrixf(const GLfloat *m))
{
    GLfloat t[16];
    TransposeMatrix(m, t);
    WRAP(glLoadMatrixf(t));  // DLREC_MAT4 is inside glLoadMatrixf
}

GLINAPI void WRAP(glLoadTransposeMatrixd(const GLdouble *m))
{
    GLfloat f[16];
    for(int i = 0; i < 16; i++) f[i] = (GLfloat)m[i];
    WRAP(glLoadTransposeMatrixf(f));
}

GLINAPI void WRAP(glMultTransposeMatrixf(const GLfloat *m))
{
    GLfloat t[16];
    TransposeMatrix(m, t);
    WRAP(glMultMatrixf(t));  // DLREC_MAT4 is inside glMultMatrixf
}

GLINAPI void WRAP(glMultTransposeMatrixd(const GLdouble *m))
{
    GLfloat f[16];
    for(int i = 0; i < 16; i++) f[i] = (GLfloat)m[i];
    WRAP(glMultTransposeMatrixf(f));
}
