#include "GLES.h"

extern GLuint nCurrentFB;

void WRAP(glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void * indices));
void WRAP(glBegin(GLenum mode));
void WRAP(glEnd());
void WRAP(glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha));
void WRAP(glColor4sv(const GLshort* v));
void WRAP(glColorMaskIndexed(GLuint framebuffer, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha));
void WRAP(glVertex3f(GLfloat x, GLfloat y, GLfloat z));
void WRAP(glBindFramebuffer(GLenum target, GLuint framebuffer));
void WRAP(glBindTexture(GLenum target, GLuint texture));