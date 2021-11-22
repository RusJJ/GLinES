#include "gl_queries.h"

inline query_desc_t* FindQueryForTarget(GLenum target)
{
    static int i;
    query_desc_t* query;
    for(i = 1; i < MAX_COUNT_OF_SAVED_QUERIES; ++i)
    {
        query = globals->queries[i];
        if(query != NULL && query->target == target) return query;
    }
    return NULL;
}

void WRAP(glGenQueries(GLsizei n, GLuint * ids))
{
    static int i; i = 0;
    static GLuint freeId; freeId = 1;
    query_desc_t* query = NULL;
    while(i < n)
    {
        while(globals->queries[freeId] != NULL) ++freeId;
        query = new query_desc_t;
        query->id = freeId;
        globals->queries[freeId] = query;

        ids[i] = freeId;
        ++i;
    }
}

void WRAP(glDeleteQueries(GLsizei n, const GLuint* ids))
{
    static int i; i = 0;
    query_desc_t* query;
    while(i < n)
    {
        if((query = globals->queries[ids[i]]) != NULL)
        {
            delete query;
            globals->queries[ids[i]] = NULL;
        }
        ++i;
    }
}

GLboolean WRAP(glIsQuery(GLuint id))
{
    return globals->queries[id] != NULL;
}

void WRAP(glBeginQuery(GLenum target, GLuint id))
{
    query_desc_t* query = globals->queries[id];
    if(query == NULL)
    {
        query = new query_desc_t;
        globals->queries[id] = query;
    }
    else
    {
        if(query->active || FindQueryForTarget(target) != NULL)
            return;
    }

    switch(target)
    {
		case 0x8914: //GL_SAMPLES_PASSED:
		case 0x8C2F: //GL_ANY_SAMPLES_PASSED:
		case 0x8D6A: //GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
		case 0x8C87: //GL_PRIMITIVES_GENERATED:
		case 0x8C88: //GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
		case 0x88BF: //GL_TIME_ELAPSED:
			break;

		default:
			return;
	}

    query->target = target;
    query->active = true;
    query->start = GetClock() - globals->gl.queriesTimeOffset;
}

void WRAP(glEndQuery(GLenum target, GLuint id))
{
    query_desc_t* query = globals->queries[id];
    if(query == NULL) return;

    switch(target)
    {
		case 0x8914: //GL_SAMPLES_PASSED:
		case 0x8C2F: //GL_ANY_SAMPLES_PASSED:
		case 0x8D6A: //GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
		case 0x8C87: //GL_PRIMITIVES_GENERATED:
		case 0x8C88: //GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
		case 0x88BF: //GL_TIME_ELAPSED:
			break;

		default:
			return;
	}

    query->active = false;
    query->start = GetClock() - globals->gl.queriesTimeOffset - query->start;
}

void WRAP(glQueryCounter(GLuint id, GLenum target))
{
    if(target != 0x8E28) return; // GL_TIMESTAMP
    query_desc_t* query = globals->queries[id];
    if(query == NULL || query->active) return;

    query->target = target;
    query->start = GetClock() - globals->gl.queriesTimeOffset;
}

void WRAP(glGetQueryiv(GLenum target, GLenum pname, GLint* params))
{
    query_desc_t* query = FindQueryForTarget(target);
    if(query == NULL) return;

    switch (pname)
    {
		case 0x8865: //GL_CURRENT_QUERY:
			*params = (query->target==0x88BF) ? query->start : 0; // GL_TIME_ELAPSED
			break;
		case 0x8864: //GL_QUERY_COUNTER_BITS:
			*params = (query->target==0x88BF) ? 32 : 0; // GL_TIME_ELAPSED
			break;

		default:
			break;
	}
}

void WRAP(glGetQueryObjectiv(GLuint id, GLenum pname, GLint* params))
{
    query_desc_t* query = globals->queries[id];
    if(query == NULL) return;

    switch (pname)
    {
    	case 0x8867: //GL_QUERY_RESULT_AVAILABLE:
    		*params = GL_TRUE;
    		break;
		case 0x9194: //GL_QUERY_RESULT_NO_WAIT:
    	case 0x8866: //GL_QUERY_RESULT:
    		*params = (query->target==0x88BF) ? query->start : 0; // GL_TIME_ELAPSED
    		break;
    	default:
			return;
    }
}

void WRAP(glGetQueryObjectuiv(GLuint id, GLenum pname, GLuint* params))
{
    query_desc_t* query = globals->queries[id];
    if(query == NULL) return;

    switch (pname)
    {
    	case 0x8867: //GL_QUERY_RESULT_AVAILABLE:
    		*params = GL_TRUE;
    		break;
		case 0x9194: //GL_QUERY_RESULT_NO_WAIT:
    	case 0x8866: //GL_QUERY_RESULT:
    		*params = (query->target==0x88BF) ? query->start : 0; // GL_TIME_ELAPSED
    		break;
    	default:
			return;
    }
}

void WRAP(glGetQueryObjecti64v(GLuint id, GLenum pname, GLint64 * params))
{
    query_desc_t* query = globals->queries[id];
    if(query == NULL) return;

    switch (pname)
    {
    	case 0x8867: //GL_QUERY_RESULT_AVAILABLE:
    		*params = GL_TRUE;
    		break;
		case 0x9194: //GL_QUERY_RESULT_NO_WAIT:
    	case 0x8866: //GL_QUERY_RESULT:
    		*params = (query->target==0x88BF) ? query->start : 0; // GL_TIME_ELAPSED
    		break;
    	default:
			return;
    }
}

void WRAP(glGetQueryObjectui64v(GLuint id, GLenum pname, GLint64 * params))
{
    query_desc_t* query = globals->queries[id];
    if(query == NULL) return;

    switch (pname)
    {
    	case 0x8867: //GL_QUERY_RESULT_AVAILABLE:
    		*params = GL_TRUE;
    		break;
		case 0x9194: //GL_QUERY_RESULT_NO_WAIT:
    	case 0x8866: //GL_QUERY_RESULT:
    		*params = (query->target==0x88BF) ? query->start : 0; // GL_TIME_ELAPSED
    		break;
    	default:
			return;
    }
}