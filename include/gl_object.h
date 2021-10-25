#include "GLES.h"

void GLIN_Wrap_glGetInfoLog(GLuint obj, GLsizei maxLength, GLsizei *length, GLchar *infoLog);
void GLIN_Wrap_glDeleteObject(GLuint obj);
void GLIN_Wrap_glGetObjectParameterfv(GLuint obj, GLenum pname, GLfloat *params);
void GLIN_Wrap_glGetObjectParameteriv(GLuint obj, GLenum pname, GLint *params);