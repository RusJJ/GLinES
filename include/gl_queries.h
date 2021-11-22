#include "GLES.h"

void WRAP(glGenQueries(GLsizei n, GLuint * ids));
void WRAP(glDeleteQueries(GLsizei n, const GLuint* ids));
GLboolean WRAP(glIsQuery(GLuint id));
void WRAP(glBeginQuery(GLenum target, GLuint id));
void WRAP(glEndQuery(GLenum target, GLuint id));
void WRAP(glQueryCounter(GLuint id, GLenum target));
void WRAP(glGetQueryiv(GLenum target, GLenum pname, GLint* params));
void WRAP(glGetQueryObjectiv(GLuint id, GLenum pname, GLint* params));
void WRAP(glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint* params));
void WRAP(glGetQueryObjecti64v(GLuint id, GLenum pname, GLint64 * params));
void WRAP(glGetQueryObjectui64v(GLuint id, GLenum pname, GLint64 * params));