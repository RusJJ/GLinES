#include "GLES.h"

void WRAP(glGetInfoLog(GLuint obj, GLsizei maxLength, GLsizei *length, GLchar *infoLog));
void WRAP(glDeleteObject(GLuint obj));
void WRAP(glGetObjectParameterfv(GLuint obj, GLenum pname, GLfloat *params));
void WRAP(glGetObjectParameteriv(GLuint obj, GLenum pname, GLint *params));
GLboolean WRAP(glIsProgram(GLuint program));
void WRAP(glProgramString(GLenum target, GLenum format, GLsizei len, const GLvoid *string));