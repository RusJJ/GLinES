#include "GLES.h"

void GLIN_Wrap_glMatrixMode(GLenum mode)
{
    DBG("glMatrixMode");
    globals->lastMatrixMode = mode;
}

void GLIN_Wrap_glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal)
{
    DBG("glOrtho");
}