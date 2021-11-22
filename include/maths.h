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

#define _1_DIV_255 0.00392156862f
#define _255_DIV_12_92 19.7368421053f
extern "C" float powf(float basis, float exponent );
inline char ConvertSRGBPixelToRGB(char srgb_pixel)
{
	static float linear;
    linear = _1_DIV_255 * srgb_pixel;
	if (linear <= 0.04045f)
        return _255_DIV_12_92 * linear;
	return 255.0f * powf((linear + 0.055f) / 1.055f, 1.6f);
}

inline void ConvertSRGBPixelsToRGB(char* pixels, unsigned int width, unsigned int height)
{
    static unsigned int x, y;
    for(x = 0, y = 4*width*height; x < y; ++x)
    {
        pixels[x] = ConvertSRGBPixelToRGB(pixels[x]);
    }
}