#include "gl_queries.h"
#include <limits>

GLINAPI void* GLIN_GetBackendProc(const char* name);

static bool IsTimer(GLenum target) { return target == 0x88BF || target == 0x8E28; }

template<class F> static bool QueryCall(F&& call)
{
    GLenum previous = glGetError();
    if(previous != GL_NO_ERROR) SetError(previous);
    call();
    GLenum error = glGetError();
    if(error != GL_NO_ERROR) SetError(error);
    return error == GL_NO_ERROR;
}

void WRAP(glGenQueries(GLsizei n, GLuint* ids)) { glGenQueries(n, ids); }
void WRAP(glDeleteQueries(GLsizei n, const GLuint* ids))
{
    if(QueryCall([&]() { glDeleteQueries(n, ids); }))
        for(GLsizei i = 0; i < n; ++i) globals->gl.queryTargets.erase(ids[i]);
}
GLboolean WRAP(glIsQuery(GLuint id)) { return glIsQuery(id); }

void WRAP(glBeginQuery(GLenum target, GLuint id))
{
    if(target == 0x8E28) { SetError(GL_INVALID_ENUM); return; }
    if(id && id == globals->gl.conditionalQuery) { SetError(GL_INVALID_OPERATION); return; }
    if(IsTimer(target))
    {
        auto fn = GLIN_HasExtension("GL_EXT_disjoint_timer_query") ? (void(*)(GLenum, GLuint))GLIN_GetBackendProc("glBeginQueryEXT") : nullptr;
        if(fn) { if(QueryCall([&]() { fn(target, id); })) globals->gl.queryTargets[id] = target; }
        else SetError(GL_INVALID_ENUM);
        return;
    }
    if(QueryCall([&]() { glBeginQuery(target, id); })) globals->gl.queryTargets[id] = target;
}

void WRAP(glEndQuery(GLenum target))
{
    if(target == 0x8E28) { SetError(GL_INVALID_ENUM); return; }
    if(IsTimer(target))
    {
        auto fn = GLIN_HasExtension("GL_EXT_disjoint_timer_query") ? (void(*)(GLenum))GLIN_GetBackendProc("glEndQueryEXT") : nullptr;
        if(fn) fn(target);
        else SetError(GL_INVALID_ENUM);
        return;
    }
    glEndQuery(target);
}

void WRAP(glQueryCounter(GLuint id, GLenum target))
{
    if(target != 0x8E28) { SetError(GL_INVALID_ENUM); return; }
    if(id && id == globals->gl.conditionalQuery) { SetError(GL_INVALID_OPERATION); return; }
    auto fn = GLIN_HasExtension("GL_EXT_disjoint_timer_query") ? (void(*)(GLuint, GLenum))GLIN_GetBackendProc("glQueryCounterEXT") : nullptr;
    if(fn) { if(QueryCall([&]() { fn(id, target); })) globals->gl.queryTargets[id] = target; }
    else SetError(GL_INVALID_OPERATION);
}

void WRAP(glGetQueryiv(GLenum target, GLenum pname, GLint* params))
{
    if(IsTimer(target))
    {
        auto fn = GLIN_HasExtension("GL_EXT_disjoint_timer_query") ? (void(*)(GLenum, GLenum, GLint*))GLIN_GetBackendProc("glGetQueryivEXT") : nullptr;
        if(fn) fn(target, pname, params);
        else SetError(GL_INVALID_ENUM);
        return;
    }
    glGetQueryiv(target, pname, params);
}

void WRAP(glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint* params)) { glGetQueryObjectuiv(id, pname, params); }

void WRAP(glGetQueryObjectui64v(GLuint id, GLenum pname, GLuint64* params))
{
    auto fn = GLIN_HasExtension("GL_EXT_disjoint_timer_query") ? (void(*)(GLuint, GLenum, GLuint64*))GLIN_GetBackendProc("glGetQueryObjectui64vEXT") : nullptr;
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

void WRAP(glBeginConditionalRender(GLuint id, GLenum mode))
{
    DLREC(WRAP(glBeginConditionalRender(id, mode)));
    auto& state = globals->gl;
    if(state.conditionalQuery || globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(mode < 0x8E13 || mode > 0x8E16) { SetError(GL_INVALID_ENUM); return; }
    if(!glIsQuery(id)) { SetError(GL_INVALID_VALUE); return; }
    auto query = state.queryTargets.find(id);
    if(query == state.queryTargets.end() || (query->second != 0x8914 && query->second != GL_ANY_SAMPLES_PASSED && query->second != GL_ANY_SAMPLES_PASSED_CONSERVATIVE)) { SetError(GL_INVALID_OPERATION); return; }
    GLint active = 0;
    glGetQueryiv(query->second, GL_CURRENT_QUERY, &active);
    if((GLuint)active == id) { SetError(GL_INVALID_OPERATION); return; }
    auto begin = GLIN_HasExtension("GL_NV_conditional_render") ? (void(*)(GLuint, GLenum))GLIN_GetBackendProc("glBeginConditionalRenderNV") : nullptr;
    auto end = begin ? (void(*)())GLIN_GetBackendProc("glEndConditionalRenderNV") : nullptr;
    bool discard = false;
    if(begin && end)
    {
        if(!QueryCall([&]() { begin(id, mode); })) return;
    }
    else
    {
        GLuint available = GL_TRUE, result = GL_TRUE;
        if(mode == 0x8E14 || mode == 0x8E16)
            if(!QueryCall([&]() { glGetQueryObjectuiv(id, GL_QUERY_RESULT_AVAILABLE, &available); })) return;
        if(available)
        {
            if(!QueryCall([&]() { glGetQueryObjectuiv(id, GL_QUERY_RESULT, &result); })) return;
            discard = result == 0;
        }
    }
    state.conditionalQuery = id;
    state.conditionalNative = begin && end;
    state.conditionalDiscard = discard;
}

void WRAP(glEndConditionalRender())
{
    DLREC(WRAP(glEndConditionalRender()));
    auto& state = globals->gl;
    if(!state.conditionalQuery || globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(state.conditionalNative)
    {
        auto end = (void(*)())GLIN_GetBackendProc("glEndConditionalRenderNV");
        if(!QueryCall([&]() { end(); })) return;
    }
    state.conditionalQuery = 0;
    state.conditionalDiscard = state.conditionalNative = false;
}

void WRAP(glClear(GLbitfield mask))
{
    DLREC(WRAP(glClear(mask)));
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(!globals->gl.conditionalDiscard) glClear(mask);
}

void WRAP(glClearColor(GLfloat r, GLfloat g, GLfloat b, GLfloat a))
{
    DLREC(WRAP(glClearColor(r, g, b, a)));
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    glClearColor(r, g, b, a);
}

void WRAP(glClearBufferiv(GLenum buffer, GLint drawbuffer, const GLint* value))
{
    DLREC_DATA(value, (buffer == GL_COLOR ? 4 : 1), WRAP(glClearBufferiv(buffer, drawbuffer, _values.data())));
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(!globals->gl.conditionalDiscard) glClearBufferiv(buffer, drawbuffer, value);
}

void WRAP(glClearBufferuiv(GLenum buffer, GLint drawbuffer, const GLuint* value))
{
    DLREC_DATA(value, (buffer == GL_COLOR ? 4 : 1), WRAP(glClearBufferuiv(buffer, drawbuffer, _values.data())));
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(!globals->gl.conditionalDiscard) glClearBufferuiv(buffer, drawbuffer, value);
}

void WRAP(glClearBufferfv(GLenum buffer, GLint drawbuffer, const GLfloat* value))
{
    DLREC_DATA(value, (buffer == GL_COLOR ? 4 : 1), WRAP(glClearBufferfv(buffer, drawbuffer, _values.data())));
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(!globals->gl.conditionalDiscard) glClearBufferfv(buffer, drawbuffer, value);
}

void WRAP(glClearBufferfi(GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil))
{
    DLREC(WRAP(glClearBufferfi(buffer, drawbuffer, depth, stencil)));
    if(globals->render.begin) { SetError(GL_INVALID_OPERATION); return; }
    if(!globals->gl.conditionalDiscard) glClearBufferfi(buffer, drawbuffer, depth, stencil);
}

void WRAP(glBlitFramebuffer(GLint sx0, GLint sy0, GLint sx1, GLint sy1, GLint dx0, GLint dy0, GLint dx1, GLint dy1, GLbitfield mask, GLenum filter))
{
    if(!globals->gl.conditionalDiscard) glBlitFramebuffer(sx0, sy0, sx1, sy1, dx0, dy0, dx1, dy1, mask, filter);
}
