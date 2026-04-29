#include "GLES.h"

void* WRAP(glMapBuffer(GLenum target, GLenum access));
void WRAP(glDrawBuffer(GLenum buf));
void WRAP(glGetBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, void* dst));
void WRAP(glBindFramebuffer(GLenum target, GLuint framebuffer));
GLenum WRAP(glCheckFramebufferStatus(GLenum target));
