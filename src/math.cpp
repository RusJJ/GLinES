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