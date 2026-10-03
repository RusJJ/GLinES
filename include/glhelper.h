#include "GLES.h"
#include "packed_vertex.h"

void UseFixedProgram(GLenum mode);
bool FragmentColorClamped();
void TransformFixedVerts();
void TransposeMatrix(const float* src, float* dst);
void GetNormalMatrix(const float* mview, float* normalMat);
matrix3_t GetNormalMatrix(const float* mview);

extern thread_local GLuint g_nUberShader;

inline int GetGLTypeSize(GLenum type)
{
    switch(type)
    {
        case GL_BYTE: case GL_UNSIGNED_BYTE: return 1;
        case GL_SHORT: case GL_UNSIGNED_SHORT: case GL_HALF_FLOAT: return 2;
        case GL_INT: case GL_UNSIGNED_INT: case GL_FLOAT: return 4;
        case 0x140A: return 8; // GL_DOUBLE
        default: return 0;
    }
}

void MultiplyMatrix2(const float* matrix, const float* vector, float* output);

inline size_t GetArrayElementSize(GLint size, GLenum type)
{
    return IsPackedVertex(type) ? 4 : (size == 0x80E1 ? 4 : size) * GetGLTypeSize(type);
}
