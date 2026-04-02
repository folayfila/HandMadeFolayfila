// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#if !defined(FOLAYFILA_INTRINSICS_H)
#define FOLAYFILA_INTRINSICS_H

#include "folayfila_types.h"
#include "math.h"

// TODO: Remove math.h and write platform efecient code.

#include <stdlib.h>
inline uint32 RandomU32()
{
    return (uint32)(rand() % 1000000);
}

inline uint32 SafeTruncateUInt64(uint64 Value)
{
    Assert(Value <= 0xffffffff);
    return ((uint32)Value);
}

inline int32 Clamp32(int32 Value, int32 Min, int32 Max)
{
    if (Value < Min)
    {
        Value = Min;
    }
    else if (Value > Max)
    {
        Value = Max;
    }
    return Value;
}

inline int32 RoundFloatToInt32(float Float)
{
    int32 result = (int32)roundf(Float);
    return result;
}

inline int32 TruncateFloatToInt32(float Float)
{
    int32 result = (int32)(Float);
    return result;
}

inline int32 FloorFloatToInt32(float Float)
{
    int32 result = (int32)floorf(Float);
    return result;
}


inline uint32 RoundFloatToUInt32(float Float)
{
    uint32 result = (uint32)(Float + 0.5f);
    return result;
}

inline float Sin(float Angle)
{
    float result = sinf(Angle);
    return result;
}

inline uint32 AbsoluteInt32ToUInt32(int32 Value)
{
    uint32 result = (uint32)(Value < 0 ? Value*-1 : Value);
    return result;
}

inline float Cos(float Angle)
{
    float result = cosf(Angle);
    return result;
}


inline float ATan2(float Y, float X)
{
    float result = atan2f(Y, X);
    return result;
}

inline float LinearBlend(float A, float B, float T)
{
    float result = A + T * (B - A); // Or (1-T)A + T*B
    return result;
}

inline float Square(float A)
{
    float result = A*A;
    return result;
}

struct bit_scan_result
{
    bool32 Found;
    uint32 Index;
};
inline bit_scan_result FindLeastSignificantSetBit(uint32 Value)
{
    bit_scan_result result = {};

#if COMPILER_MSVC
    result.Found = _BitScanForward((unsigned long*)&result.Index, Value);
#else
    for (uint32 Test = 0; Test < 32; ++Test)
    {
        if (Value & (1 << Test))
        {
            result.Index = Test;
            result.Found = true;
            break;
        }
    }
#endif // COMPILER_MSVC

    return result;
}

#endif  // FOLAYFILA_INTRINSICS_H