#include "GLES.h"

GLINAPI void WRAP(glMatrixMode(GLenum mode))
{
    DBG("glMatrixMode");
    globals->lastMatrixMode = mode;
}
GLINAPI EXPORT void glMatrixMode(GLenum mode) ALIASWRAP(glMatrixMode);

GLINAPI void WRAP(glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal))
{
    DBG("glOrtho");
}