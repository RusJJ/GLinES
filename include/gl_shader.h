#pragma once
#include "GLES.h"

void WRAP(glDeleteShader(GLuint shader));
void WRAP(glCompileShader(GLuint shader));
GLuint WRAP(glCreateShader(GLenum type));
void WRAP(glLinkProgram(GLuint program));
void WRAP(glGetActiveUniformName(GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei* length, char* name));
void WRAP(glUseProgram(GLuint program));

void PreprocessShader(char* pszShaderSource, bool bIsVertexShader);
const char* ConvertShader(const char* pszShaderSource, bool bIsVertexShader);
char* ConvertARBShader(const char* pszShaderSource, bool bIsVertexShader);

void WRAP(glShaderSource(GLuint shader, GLsizei count, const GLchar* const* strings, const GLint* lengths));
void WRAP(glGetShaderSource(GLuint shader, GLsizei size, GLsizei* length, GLchar* output));
void WRAP(glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value));
void WRAP(glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value));
void WRAP(glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value));
void WRAP(glUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value));
void WRAP(glUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value));
void WRAP(glUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value));
void WRAP(glUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value));
void WRAP(glUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value));
void WRAP(glUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value));
