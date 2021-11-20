#include "GLES.h"

void WRAP(glGenTextures(GLsizei n, GLuint * textures));
void WRAP(glDeleteTextures(GLsizei n, GLuint * textures));
void WRAP(glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* data));
void WRAP(glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void * pixels));
void WRAP(glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels));
void WRAP(glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data));