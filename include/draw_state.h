#pragma once
#include "GLES.h"

struct draw_state_t
{
    GLint vao, array, program, uniform, indexedUniform;
    GLint64 uniformStart, uniformSize;
    GLfloat attributes[16][4];
    draw_state_t()
    {
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &array);
        glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        glGetIntegerv(GL_UNIFORM_BUFFER_BINDING, &uniform);
        glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, 0, &indexedUniform);
        glGetInteger64i_v(GL_UNIFORM_BUFFER_START, 0, &uniformStart);
        glGetInteger64i_v(GL_UNIFORM_BUFFER_SIZE, 0, &uniformSize);
        for(GLuint i = 0; i < 16; ++i) glGetVertexAttribfv(i, GL_CURRENT_VERTEX_ATTRIB, attributes[i]);
    }
    ~draw_state_t()
    {
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, array);
        glUseProgram(program);
        if(indexedUniform && uniformSize) glBindBufferRange(GL_UNIFORM_BUFFER, 0, indexedUniform, uniformStart, uniformSize);
        else glBindBufferBase(GL_UNIFORM_BUFFER, 0, indexedUniform);
        glBindBuffer(GL_UNIFORM_BUFFER, uniform);
        for(GLuint i = 0; i < 16; ++i) glVertexAttrib4fv(i, attributes[i]);
    }
};
