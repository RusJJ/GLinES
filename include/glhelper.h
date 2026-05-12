#include "GLES.h"

void UseFixedProgram();
void TransformFixedVerts();
void TransposeMatrix(const float* src, float* dst);
extern GLuint g_nUberShader;

inline int GetGLTypeSize(GLenum type)
{
    switch(type)
    {
        case GL_BYTE: case GL_UNSIGNED_BYTE: return 1;
        case GL_SHORT: case GL_UNSIGNED_SHORT: return 2;
        case GL_INT: case GL_UNSIGNED_INT: case GL_FLOAT: return 4;
        case 0x140A: return 8; // GL_DOUBLE
        default: return 4;
    }
}
