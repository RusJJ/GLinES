#include "GLES.h"

void WRAP(glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data));

void WRAP(glGenTextures(GLsizei n, GLuint * textures));
void WRAP(glDeleteTextures(GLsizei n, GLuint * textures));
void WRAP(glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* data));
void WRAP(glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void * pixels));
void WRAP(glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels));
void WRAP(glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data));
void WRAP(glTexImage2DMultisample(GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLboolean fixedSampleLocations));
void WRAP(glTexImage3DMultisample(GLenum target, GLsizei samples, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedSampleLocations));
void WRAP(glFramebufferTexture3D(GLenum target, GLenum attachment,  GLenum textarget, GLuint texture, GLint level, GLint layer));
void WRAP(glActiveTexture(GLenum texunit));
void WRAP(glBindMultiTexture(GLenum texunit, GLenum target, GLuint texture));

void WRAP(glTexStorage2D(GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height));
void WRAP(glBindTextureUnit(GLuint unit, GLuint texture));

void WRAP(glTexParameteri(GLenum target, GLenum pname, GLint value));
void WRAP(glTexParameterf(GLenum target, GLenum pname, GLfloat value));
void WRAP(glTexParameteriv(GLenum target, GLenum pname, const GLint* values));
void WRAP(glTexParameterfv(GLenum target, GLenum pname, const GLfloat* values));
void WRAP(glGetTexParameteriv(GLenum target, GLenum pname, GLint* values));
void WRAP(glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* values));
