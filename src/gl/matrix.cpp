#include "GLES.h"

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

GLINAPI void WRAP(glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal))
{
    DBG("glOrtho");
}
