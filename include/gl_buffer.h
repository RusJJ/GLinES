#include "GLES.h"

void* WRAP(glMapBuffer(GLenum target, GLenum access));
void WRAP(glDrawBuffer(GLenum buf));
void WRAP(glGetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, void* dst));
void WRAP(glBindFramebuffer(GLenum target, GLuint framebuffer));
GLenum WRAP(glCheckFramebufferStatus(GLenum target));
void WRAP(glBindBuffer(GLenum target, GLuint buffer));
void WRAP(glBindBufferRange(GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size));
void WRAP(glBindBufferBase(GLenum target, GLuint index, GLuint buffer));
