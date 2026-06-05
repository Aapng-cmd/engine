#pragma once

#include <iostream>
#include <random>
#include <cmath>

static std::mt19937 rng(static_cast<unsigned>(time(nullptr)));

template <typename Type = double>
Type sign(Type A) { return A < 0 ? -1 : 1; }

template <typename Type = double, int N = 3>
class vec
{
public:
    Type x, y, z;
    vec(Type x = 0, Type y = 0, Type z = 0) : x(x), y(y), z(z) {}

    static Type R(Type min, Type max)
    {
        Type r = (Type)rand() / RAND_MAX * (max - min) + min;
        return r;
    }

    static vec Rnd(Type x = R(-100, 100), Type y = R(10, 20), Type z = R(-100, 100))
    {
        return vec<>(x, y, z);
    }

    static vec RndCol(Type x = Type(-1), Type y = Type(-1), Type z = Type(-1)) {
        if (x == -1) x = vec<>().R(0, 255) / 255;
        if (y == -1) y = vec<>().R(0, 255) / 255;
        if (z == -1) z = vec<>().R(0, 255) / 255;
        return vec<>(x, y, z);
    }

    double len2() const { return x * x + y * y + z * z; }

    double len() const { return std::sqrt(len2()); }

    double dot(const vec& v) const { return x * v.x + y * v.y + z * v.z; }

    vec operator-(const vec& v) const { return vec(x - v.x, y - v.y, z - v.z); }
    vec operator+(const vec& v) const { return vec(x + v.x, y + v.y, z + v.z); }
    vec& operator+=(const vec& v) { x += v.x; y += v.y; z += v.z; return *this; }
    vec& operator-=(const vec& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
    vec operator!() const {
        const double l = len();
        return l > 0 ? vec(x / l, y / l, z / l) : vec();
    }
    vec operator-() const { return vec(-x, -y, -z); }
    vec operator^(const vec& v) const { return vec(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x); }

    template <typename MultiType>
    vec operator*(const MultiType& c) const { return vec(x * c, y * c, z * c); }

    template <typename MultiType>
    vec operator/(const MultiType& c) const { return vec(x / (double)c, y / (double)c, z / (double)c); }
};

template <typename Type = double, int N = 3>
std::ostream& operator<<(std::ostream& os, const vec<Type, N>& v)
{
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}
