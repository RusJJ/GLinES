// DXT1
void DecompressBlockDXT1(uint32_t x, uint32_t y, uint32_t width,
	const uint8_t* blockStorage,
	int transparent0, int* simpleAlpha, int *complexAlpha,
	uint32_t* image);
// DXT3
void DecompressBlockDXT3(uint32_t x, uint32_t y, uint32_t width,
	const uint8_t* blockStorage,
	int transparent0, int* simpleAlpha, int *complexAlpha,
	uint32_t* image);
// DXT5
void DecompressBlockDXT5(uint32_t x, uint32_t y, uint32_t width,
	const uint8_t* blockStorage,
	int transparent0, int* simpleAlpha, int *complexAlpha,
	uint32_t* image);