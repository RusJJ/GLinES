#include "GLES.h"
#include <vector>

#define MATRIX_WRAPPER(cols, rows, suffix) \
void WRAP(glUniformMatrix##suffix##fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)) \
{ \
    if(count < 0) { SetError(GL_INVALID_VALUE); return; } \
    if(!transpose || !count) { glUniformMatrix##suffix##fv(location, count, GL_FALSE, value); return; } \
    if(!value) { SetError(GL_INVALID_VALUE); return; } \
    std::vector<GLfloat> converted((size_t)count * cols * rows); \
    for(GLsizei i = 0; i < count; ++i) for(int c = 0; c < cols; ++c) for(int r = 0; r < rows; ++r) \
        converted[(size_t)i * cols * rows + c * rows + r] = value[(size_t)i * cols * rows + r * cols + c]; \
    glUniformMatrix##suffix##fv(location, count, GL_FALSE, converted.data()); \
}

MATRIX_WRAPPER(2, 2, 2)
MATRIX_WRAPPER(3, 3, 3)
MATRIX_WRAPPER(4, 4, 4)
MATRIX_WRAPPER(2, 3, 2x3)
MATRIX_WRAPPER(3, 2, 3x2)
MATRIX_WRAPPER(2, 4, 2x4)
MATRIX_WRAPPER(4, 2, 4x2)
MATRIX_WRAPPER(3, 4, 3x4)
MATRIX_WRAPPER(4, 3, 4x3)
