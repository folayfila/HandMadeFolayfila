// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#ifndef FOLAYFILA_MATH_H
#define FOLAYFILA_MATH_H
union vec2
{
    struct { float X, Y; };
    float E[2];

    inline vec2& operator=(const vec2& A);
    inline vec2& operator=(const float& A);
    inline vec2& operator+=(const vec2& A);
    inline vec2& operator*=(float A);
    inline vec2 operator-() const;
};

inline vec2& vec2::operator=(const vec2& A)
{
    X = A.X;
    Y = A.Y;
    return *this;
}

inline vec2& vec2::operator=(const float& A)
{
    X = A;
    Y = A;
    return *this;
}

inline vec2& vec2::operator+=(const vec2& A)
{
    X += A.X;
    Y += A.Y;
    return *this;
}

inline vec2& vec2::operator*=(float A)
{
    X *= A;
    Y *= A;
    return *this;
}

inline vec2 vec2::operator-() const
{
    return { -X, -Y };
}

// Free functions
inline vec2 operator+(vec2 A, vec2 B)
{
    vec2 Result;

    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;

    return Result;
}
inline vec2 operator-(vec2 A, vec2 B)
{
    vec2 Result;

    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;

    return Result;
}
inline vec2 operator*(float A, vec2 B)
{
    vec2 Result;

    Result.X = A*B.X;
    Result.Y = A*B.Y;

    return Result;
}
inline vec2 operator*(vec2 A, float B)
{
    vec2 Result;

    Result.X = A.X*B;
    Result.Y = A.Y*B;

    return Result;
}

struct vec3
{
    float X;
    float Y;
    float Z;
};

#endif  // FOLAYFILA_MATH_H
