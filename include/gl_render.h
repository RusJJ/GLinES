#include "GLES.h"

void WRAP(glBegin(GLenum mode));
void WRAP(glEnd());
void WRAP(glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha));
void WRAP(glColor4sv(const GLshort* v));
void WRAP(glColorMaskIndexed(GLuint framebuffer, GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha));
void WRAP(glVertex3f(GLfloat x, GLfloat y, GLfloat z));
void WRAP(glBindTexture(GLenum target, GLuint texture));
void WRAP(glDrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, void *indices, GLint basevertex));
void WRAP(glClipPlane(GLenum plane, const GLdouble *equation));
void WRAP(glClipPlanef(GLenum plane, const GLfloat *equation));
void WRAP(glClearDepth(double value));
void WRAP(glDepthRange(double n, double f));
void WRAP(glVertexAttribLPointer(GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer));
void WRAP(glVertexAttrib3d(GLuint index, GLdouble x, GLdouble y, GLdouble z));
void WRAP(glMultiDrawArrays(GLenum mode, const GLint* first, const GLsizei* count, GLsizei drawcount));
void WRAP(glMultiDrawElements(GLenum mode, const GLsizei* count, GLenum type, const void* const* indices, GLsizei drawcount));
void WRAP(glMultiDrawElementsBaseVertex(GLenum mode, const GLsizei* count, GLenum type, const void* const* indices, GLsizei drawcount, const GLint* basevertex));
