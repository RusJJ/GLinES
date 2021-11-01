#include "GLES.h"

void WRAP(glMatrixMode(GLenum mode))
{
    DBG("glMatrixMode");
    globals->lastMatrixMode = mode;
}

void WRAP(glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearVal, GLdouble farVal))
{
    DBG("glOrtho");
}