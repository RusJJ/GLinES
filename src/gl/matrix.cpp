#include "GLES.h"
#include "glhelper.h"
#include <cmath>

GLINAPI void WRAP(glMatrixMode(GLenum mode))
{
    globals->matrix.mode = mode;
}
GLINAPI EXPORT void glMatrixMode(GLenum mode) ALIASWRAP(glMatrixMode);

GLINAPI void WRAP(glLoadIdentity())
{
    globals->matrix.Current() = matrix4_t::Identity();
}

GLINAPI void WRAP(glPushMatrix())
{
    globals->matrix.Push();
}

GLINAPI void WRAP(glPopMatrix())
{
    globals->matrix.Pop();
}

GLINAPI void WRAP(glLoadMatrixf(const GLfloat* m))
{
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
    GLfloat m[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, x,y,z,1};
    globals->matrix.Current() *= m;
}

GLINAPI void WRAP(glTranslated(GLdouble x, GLdouble y, GLdouble z))
{
    WRAP(glTranslatef((GLfloat)x, (GLfloat)y, (GLfloat)z));
}

GLINAPI void WRAP(glScalef(GLfloat x, GLfloat y, GLfloat z))
{ 
    GLfloat m[16] = {x,0,0,0, 0,y,0,0, 0,0,z,0, 0,0,0,1};
    globals->matrix.Current() *= m;
}

GLINAPI void WRAP(glScaled(GLdouble x, GLdouble y, GLdouble z))
{
    WRAP(glScalef((GLfloat)x, (GLfloat)y, (GLfloat)z));
}

GLINAPI void WRAP(glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z))
{ 
    float c = cosf(angle * M_PI / 180.0f), s = sinf(angle * M_PI / 180.0f), t = 1.0f - c;
    float len = sqrtf(x*x + y*y + z*z);
    if (len > 0.0f)
    {
        x/=len; y/=len; z/=len; 
        GLfloat m[16] = {t*x*x+c, t*x*y-s*z, t*x*z+s*y, 0, t*x*y+s*z, t*y*y+c, t*y*z-s*x, 0, t*x*z-s*y, t*y*z+s*x, t*z*z+c, 0, 0,0,0,1};
        globals->matrix.Current() *= m;
    }
}

GLINAPI void WRAP(glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z))
{
    WRAP(glRotatef((GLfloat)angle, (GLfloat)x, (GLfloat)y, (GLfloat)z));
}

GLINAPI void WRAP(glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal))
{
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
    WRAP(glLoadMatrixf(t));
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
    WRAP(glMultMatrixf(t));
}

GLINAPI void WRAP(glMultTransposeMatrixd(const GLdouble *m))
{
    GLfloat f[16];
    for(int i = 0; i < 16; i++) f[i] = (GLfloat)m[i];
    WRAP(glMultTransposeMatrixf(f));
}
