void MatrixToVector(const float *v1, const float *v2, float *out); // GL4ES
void NormalizeVector(float *in_out); // GL4ES
void InverseMatrix(const float *mat, float *out); // GL4ES
void MultiplyMatrix(const float *mat1, const float *mat2, float *out);

// Inline Funcs
inline void MultiplyVec3(const float *v1, const float *v2, float *out)
{
    out[0] = v1[0] * v2[0];
    out[1] = v1[1] * v2[1];
    out[2] = v1[2] * v2[2];
}

inline void MultiplyVec4(const float *v1, const float *v2, float *out)
{
    out[0] = v1[0] * v2[0];
    out[1] = v1[1] * v2[1];
    out[2] = v1[2] * v2[2];
    out[3] = v1[3] * v2[3];
}