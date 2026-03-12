// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#if !defined(FOLAYFILA_INTRINSICS_H)
#define FOLAYFILA_INTRINSICS_H

#include "math.h"

// TODO: Remove math.h and write platform efecient code.

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
    int32 Result = (int32)(Float + 0.5f);
    return Result;
}

inline int32 TruncateFloatToInt32(float Float)
{
    int32 Result = (int32)(Float);
    return Result;
}

inline int32 FloorFloatToInt32(float Float)
{
    int32 Result = (int32)floorf(Float);
    return Result;
}


inline uint32 RoundFloatToUInt32(float Float)
{
    uint32 Result = (uint32)(Float + 0.5f);
    return Result;
}

inline float Sin(float Angle)
{
    float Result = sinf(Angle);
    return Result;
}


inline float Cos(float Angle)
{
    float Result = cosf(Angle);
    return Result;
}


inline float ATan2(float Y, float X)
{
    float Result = atan2f(Y, X);
    return Result;
}

#endif  // FOLAYFILA_INTRINSICS_H