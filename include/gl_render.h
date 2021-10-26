#include "GLES.h"

void GLIN_Wrap_glBegin(GLenum mode);
void GLIN_Wrap_glEnd();
void GLIN_Wrap_glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void GLIN_Wrap_glColor4sv(const GLshort* v);
void GLIN_Wrap_glColorMaskIndexed(GLuint framebuffer, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void GLIN_Wrap_glBindProgram(GLenum target, GLuint program);
void GLIN_Wrap_glGenPrograms(GLsizei n, GLuint *programs);
void GLIN_Wrap_glVertex3f(GLfloat x, GLfloat y, GLfloat z);