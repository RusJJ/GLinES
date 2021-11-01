#include "GLES.h"

void* WRAP(glMapBuffer(GLenum target, GLenum access));
void WRAP(glDrawBuffer(GLenum buf));
void WRAP(glCopyBufferSubData(GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size));