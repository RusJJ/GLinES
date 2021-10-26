#include <math.h>

void MatrixToVector(const float *a1, const float *a2, float *out) // GL4ES
{
#if defined(__ARM_NEON__)
    const float* a = a1 + 8;
    asm volatile (
        "vld4.f32 {d0,d2,d4,d6}, [%1]            \n"
        "vld4.f32 {d1,d3,d5,d7}, [%2]            \n"
        "vld1.f32 {q4}, [%3]                     \n"
        "vmul.f32 q0, q0, d8[0]                  \n"
        "vmla.f32 q0, q1, d8[1]                  \n"
        "vmla.f32 q0, q2, d9[0]                  \n"
        "vmla.f32 q0, q3, d9[1]                  \n"
        "vst1.f32 {q0}, [%0]                     \n"
        ::"r"(out), "r"(a1), "r"(a), "r"(a2)
        : "q0", "q1", "q2", "q3", "q4", "memory"
    );
#else
    out[0] = a1[0] *  a2[0] + a1[1]  * a2[1] + a1[2]  * a2[2] + a1[3]  * a2[3];
    out[1] = a1[4] *  a2[0] + a1[5]  * a2[1] + a1[6]  * a2[2] + a1[7]  * a2[3];
    out[2] = a1[8] *  a2[0] + a1[9]  * a2[1] + a1[10] * a2[2] + a1[11] * a2[3];
    out[3] = a1[12] * a2[0] + a1[13] * a2[1] + a1[14] * a2[2] + a1[15] * a2[3];
#endif
}

void NormalizeVector(float *a) // GL4ES
{
#if defined(__ARM_NEON__)
    asm volatile (
        "vld1.32                {d4}, [%0]       \n"
        "flds                   s10, [%0, #8]    \n"
        "vsub.f32               s11, s11, s11    \n"

        "vmul.f32               d0, d4, d4       \n"
        "vpadd.f32              d0, d0           \n"
        "vmla.f32               d0, d5, d5       \n"
        
        "vmov.f32               d1, d0           \n"
        "vrsqrte.f32    		d0, d0           \n"
        "vmul.f32               d2, d0, d1       \n"
        "vrsqrts.f32    		d3, d2, d0       \n"  
        "vmul.f32               d0, d0, d3       \n"

        "vmul.f32               q2, q2, d0[0]    \n"
        "vst1.32                {d4}, [%0]       \n"
        "fsts                   s10, [%0, #8]    \n"
        
        :"+&r"(a): 
        : "d0", "d1", "d2", "d3", "d4", "d5", "memory"
    );
#else
    float det = 1.0f / sqrtf(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
    a[0] *= det;
    a[1] *= det;
    a[2] *= det;
#endif
}

void InverseMatrix(const float *m, float *r) // GL4ES
{
    r[0] = m[5]*m[10]*m[15] - m[5]*m[14]*m[11] - m[6]*m[9]*m[15] + m[6]*m[13]*m[11] + m[7]*m[9]*m[14] - m[7]*m[13]*m[10];
    r[1] = -m[1]*m[10]*m[15] + m[1]*m[14]*m[11] + m[2]*m[9]*m[15] - m[2]*m[13]*m[11] - m[3]*m[9]*m[14] + m[3]*m[13]*m[10];
    r[2] = m[1]*m[6]*m[15] - m[1]*m[14]*m[7] - m[2]*m[5]*m[15] + m[2]*m[13]*m[7] + m[3]*m[5]*m[14] - m[3]*m[13]*m[6];
    r[3] = -m[1]*m[6]*m[11] + m[1]*m[10]*m[7] + m[2]*m[5]*m[11] - m[2]*m[9]*m[7] - m[3]*m[5]*m[10] + m[3]*m[9]*m[6];

    r[4] = -m[4]*m[10]*m[15] + m[4]*m[14]*m[11] + m[6]*m[8]*m[15] - m[6]*m[12]*m[11] - m[7]*m[8]*m[14] + m[7]*m[12]*m[10];
    r[5] = m[0]*m[10]*m[15] - m[0]*m[14]*m[11] - m[2]*m[8]*m[15] + m[2]*m[12]*m[11] + m[3]*m[8]*m[14] - m[3]*m[12]*m[10];
    r[6] = -m[0]*m[6]*m[15] + m[0]*m[14]*m[7] + m[2]*m[4]*m[15] - m[2]*m[12]*m[7] - m[3]*m[4]*m[14] + m[3]*m[12]*m[6];
    r[7] = m[0]*m[6]*m[11] - m[0]*m[10]*m[7] - m[2]*m[4]*m[11] + m[2]*m[8]*m[7] + m[3]*m[4]*m[10] - m[3]*m[8]*m[6];

    r[8] = m[4]*m[9]*m[15] - m[4]*m[13]*m[11] - m[5]*m[8]*m[15] + m[5]*m[12]*m[11] + m[7]*m[8]*m[13] - m[7]*m[12]*m[9];
    r[9] = -m[0]*m[9]*m[15] + m[0]*m[13]*m[11] + m[1]*m[8]*m[15] - m[1]*m[12]*m[11] - m[3]*m[8]*m[13] + m[3]*m[12]*m[9];
    r[10] = m[0]*m[5]*m[15] - m[0]*m[13]*m[7] - m[1]*m[4]*m[15] + m[1]*m[12]*m[7] + m[3]*m[4]*m[13] - m[3]*m[12]*m[5];
    r[11] = -m[0]*m[5]*m[11] + m[0]*m[9]*m[7] + m[1]*m[4]*m[11] - m[1]*m[8]*m[7] - m[3]*m[4]*m[9] + m[3]*m[8]*m[5];

    r[12] = -m[4]*m[9]*m[14] + m[4]*m[13]*m[10] + m[5]*m[8]*m[14] - m[5]*m[12]*m[10] - m[6]*m[8]*m[13] + m[6]*m[12]*m[9];
    r[13] = m[0]*m[9]*m[14] - m[0]*m[13]*m[10] - m[1]*m[8]*m[14] + m[1]*m[12]*m[10] + m[2]*m[8]*m[13] - m[2]*m[12]*m[9];
    r[14] = -m[0]*m[5]*m[14] + m[0]*m[13]*m[6] + m[1]*m[4]*m[14] - m[1]*m[12]*m[6] - m[2]*m[4]*m[13] + m[2]*m[12]*m[5];
    r[15] = m[0]*m[5]*m[10] - m[0]*m[9]*m[6] - m[1]*m[4]*m[10] + m[1]*m[8]*m[6] + m[2]*m[4]*m[9] - m[2]*m[8]*m[5];

    float det = 1.0f/(m[0]*r[0] + m[1]*r[4] + m[2]*r[8] + m[3]*r[12]);
    for (int i = 0; i < 16; i++) r[i] *= det;
}

void MultiplyMatrix(const float *a, const float *b, float *c) // GL4ES
{
#if defined(__ARM_NEON__)
    const float* a1 = a + 8;
	const float* b1 = b + 8;
    float* c1 = c + 8;
    asm volatile (
        "vld1.32  {d16-d19}, [%2]       \n" 
        "vld1.32  {d20-d23}, [%3]       \n"
        "vld1.32  {d0-d3}, [%4]         \n"
        "vld1.32  {d4-d7}, [%5]         \n"
        "vmul.f32 q12, q8, d0[0]        \n"
        "vmul.f32 q13, q8, d2[0]        \n"
        "vmul.f32 q14, q8, d4[0]        \n"
        "vmul.f32 q15, q8, d6[0]        \n"
        "vmla.f32 q12, q9, d0[1]        \n"
        "vmla.f32 q13, q9, d2[1]        \n"
        "vmla.f32 q14, q9, d4[1]        \n"
        "vmla.f32 q15, q9, d6[1]        \n"
        "vmla.f32 q12, q10, d1[0]       \n"
        "vmla.f32 q13, q10, d3[0]       \n"
        "vmla.f32 q14, q10, d5[0]       \n"
        "vmla.f32 q15, q10, d7[0]       \n"
        "vmla.f32 q12, q11, d1[1]       \n"
        "vmla.f32 q13, q11, d3[1]       \n"
        "vmla.f32 q14, q11, d5[1]       \n"
        "vmla.f32 q15, q11, d7[1]       \n"
        "vst1.32  {d24-d27}, [%0]       \n"
        "vst1.32  {d28-d31}, [%1]       \n"
    ::"r"(c), "r"(c1), "r"(a), "r"(a1), "r"(b), "r"(b1)
    : "q0", "q1", "q2", "q3", 
      "q8", "q9", "q10", "q11", "q12", "q13", "q14", "q15", "memory"
        );
#else
   float a00 = a[0], a01 = a[1], a02 = a[2], a03 = a[3],
        a10 = a[4], a11 = a[5], a12 = a[6], a13 = a[7],
        a20 = a[8], a21 = a[9], a22 = a[10], a23 = a[11],
        a30 = a[12], a31 = a[13], a32 = a[14], a33 = a[15];

    float b0  = b[0], b1 = b[1], b2 = b[2], b3 = b[3];
    c[0] = b0*a00 + b1*a10 + b2*a20 + b3*a30;
    c[1] = b0*a01 + b1*a11 + b2*a21 + b3*a31;
    c[2] = b0*a02 + b1*a12 + b2*a22 + b3*a32;
    c[3] = b0*a03 + b1*a13 + b2*a23 + b3*a33;

    b0 = b[4]; b1 = b[5]; b2 = b[6]; b3 = b[7];
    c[4] = b0*a00 + b1*a10 + b2*a20 + b3*a30;
    c[5] = b0*a01 + b1*a11 + b2*a21 + b3*a31;
    c[6] = b0*a02 + b1*a12 + b2*a22 + b3*a32;
    c[7] = b0*a03 + b1*a13 + b2*a23 + b3*a33;

    b0 = b[8]; b1 = b[9]; b2 = b[10]; b3 = b[11];
    c[8] = b0*a00 + b1*a10 + b2*a20 + b3*a30;
    c[9] = b0*a01 + b1*a11 + b2*a21 + b3*a31;
    c[10] = b0*a02 + b1*a12 + b2*a22 + b3*a32;
    c[11] = b0*a03 + b1*a13 + b2*a23 + b3*a33;

    b0 = b[12]; b1 = b[13]; b2 = b[14]; b3 = b[15];
    c[12] = b0*a00 + b1*a10 + b2*a20 + b3*a30;
    c[13] = b0*a01 + b1*a11 + b2*a21 + b3*a31;
    c[14] = b0*a02 + b1*a12 + b2*a22 + b3*a32;
    c[15] = b0*a03 + b1*a13 + b2*a23 + b3*a33;
#endif
}