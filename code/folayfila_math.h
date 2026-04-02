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
    vec2 result;

    result.X = A.X + B.X;
    result.Y = A.Y + B.Y;

    return result;
}
inline vec2 operator-(vec2 A, vec2 B)
{
    vec2 result;

    result.X = A.X - B.X;
    result.Y = A.Y - B.Y;

    return result;
}
inline vec2 operator*(float A, vec2 B)
{
    vec2 result;

    result.X = A*B.X;
    result.Y = A*B.Y;

    return result;
}
inline vec2 operator*(vec2 A, float B)
{
    vec2 result;

    result.X = A.X*B;
    result.Y = A.Y*B;

    return result;
}

inline float Dot(vec2 A, vec2 B)
{
    float result = (A.X * B.X) + (A.Y * B.Y);
    return result;
}

struct vec3
{
    float X;
    float Y;
    float Z;
};

#endif  // FOLAYFILA_MATH_H
