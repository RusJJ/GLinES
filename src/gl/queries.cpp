#include "gl_queries.h"
#include <limits>

GLINAPI void* GLIN_GetBackendProc(const char* name);

static bool IsTimer(GLenum target) { return target == 0x88BF || target == 0x8E28; }

void WRAP(glGenQueries(GLsizei n, GLuint* ids)) { glGenQueries(n, ids); }
void WRAP(glDeleteQueries(GLsizei n, const GLuint* ids)) { glDeleteQueries(n, ids); }
GLboolean WRAP(glIsQuery(GLuint id)) { return glIsQuery(id); }

void WRAP(glBeginQuery(GLenum target, GLuint id))
{
    if(IsTimer(target))
    {
        auto fn = (void(*)(GLenum, GLuint))GLIN_GetBackendProc("glBeginQueryEXT");
        if(fn) fn(target, id);
        else SetError(GL_INVALID_ENUM);
        return;
    }
    glBeginQuery(target, id);
}

void WRAP(glEndQuery(GLenum target))
{
    if(IsTimer(target))
    {
        auto fn = (void(*)(GLenum))GLIN_GetBackendProc("glEndQueryEXT");
        if(fn) fn(target);
        else SetError(GL_INVALID_ENUM);
        return;
    }
    glEndQuery(target);
}

void WRAP(glQueryCounter(GLuint id, GLenum target))
{
    if(target != 0x8E28) { SetError(GL_INVALID_ENUM); return; }
    auto fn = (void(*)(GLuint, GLenum))GLIN_GetBackendProc("glQueryCounterEXT");
    if(fn) fn(id, target);
    else SetError(GL_INVALID_OPERATION);
}

void WRAP(glGetQueryiv(GLenum target, GLenum pname, GLint* params))
{
    if(IsTimer(target))
    {
        auto fn = (void(*)(GLenum, GLenum, GLint*))GLIN_GetBackendProc("glGetQueryivEXT");
        if(fn) fn(target, pname, params);
        else SetError(GL_INVALID_ENUM);
        return;
    }
    glGetQueryiv(target, pname, params);
}

void WRAP(glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint* params)) { glGetQueryObjectuiv(id, pname, params); }

void WRAP(glGetQueryObjectui64v(GLuint id, GLenum pname, GLuint64* params))
{
    auto fn = (void(*)(GLuint, GLenum, GLuint64*))GLIN_GetBackendProc("glGetQueryObjectui64vEXT");
    if(fn) { fn(id, pname, params); return; }
    GLuint value = 0;
    glGetQueryObjectuiv(id, pname, &value);
    *params = value;
}

void WRAP(glGetQueryObjectiv(GLuint id, GLenum pname, GLint* params))
{
    GLuint value = 0;
    glGetQueryObjectuiv(id, pname, &value);
    *params = value > (GLuint)std::numeric_limits<GLint>::max() ? std::numeric_limits<GLint>::max() : (GLint)value;
}

void WRAP(glGetQueryObjecti64v(GLuint id, GLenum pname, GLint64* params))
{
    GLuint64 value = 0;
    WRAP(glGetQueryObjectui64v(id, pname, &value));
    *params = value > (GLuint64)std::numeric_limits<GLint64>::max() ? std::numeric_limits<GLint64>::max() : (GLint64)value;
}
