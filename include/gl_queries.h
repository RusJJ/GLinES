#include "GLES.h"

void WRAP(glBeginConditionalRender(GLuint id, GLenum mode));
void WRAP(glEndConditionalRender());
void WRAP(glClear(GLbitfield mask));
void WRAP(glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a));
void WRAP(glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint* value));
void WRAP(glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint* value));
void WRAP(glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat* value));
void WRAP(glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil));
void WRAP(glBlitFramebuffer(GLint sx0, GLint sy0, GLint sx1, GLint sy1, GLint dx0, GLint dy0, GLint dx1, GLint dy1, GLbitfield mask, GLenum filter));

void WRAP(glGenQueries(GLsizei n, GLuint * ids));
void WRAP(glDeleteQueries(GLsizei n, const GLuint* ids));
GLboolean WRAP(glIsQuery(GLuint id));
void WRAP(glBeginQuery(GLenum target, GLuint id));
void WRAP(glEndQuery(GLenum target));
void WRAP(glQueryCounter(GLuint id, GLenum target));
void WRAP(glGetQueryiv(GLenum target, GLenum pname, GLint* params));
void WRAP(glGetQueryObjectiv(GLuint id, GLenum pname, GLint* params));
void WRAP(glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint* params));
void WRAP(glGetQueryObjecti64v(GLuint id, GLenum pname, GLint64 * params));
void WRAP(glGetQueryObjectui64v(GLuint id, GLenum pname, GLuint64 * params));
