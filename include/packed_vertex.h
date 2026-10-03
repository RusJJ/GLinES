#pragma once
#include <algorithm>
#include <cstdint>

inline bool IsPackedVertex(uint32_t type)
{
    return type == 0x8368 || type == 0x8D9F;
}

inline bool UnpackVertex(uint32_t type, uint32_t packed, bool normalized, float* values)
{
    if(!IsPackedVertex(type)) return false;
    for(int i = 0; i < 4; ++i)
    {
        unsigned int bits = i == 3 ? 2 : 10, shift = i * 10;
        uint32_t mask = (1u << bits) - 1, value = (packed >> shift) & mask;
        if(type == 0x8D9F)
        {
            int32_t signedValue = value & (1u << (bits - 1)) ? (int32_t)value - (1 << bits) : (int32_t)value;
            values[i] = normalized ? std::max(-1.0f, (float)signedValue / (float)(mask >> 1)) : (float)signedValue;
        }
        else values[i] = normalized ? (float)value / (float)mask : (float)value;
    }
    return true;
}
