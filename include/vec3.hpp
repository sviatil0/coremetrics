#ifndef __VEC3_HPP__
#define __VEC3_HPP__

#include <cmath>

template <typename T>
class Tvec3
{
private:
	T components[3];

public:
	T &x, &y, &z;

	Tvec3() : components{0, 0, 0}, x(components[0]), y(components[1]), z(components[2]) {}
    Tvec3(const Tvec3<T> &copy) : Tvec3()
    {
        x = copy.x;
        y = copy.y;
        z = copy.z;
    }
    Tvec3(T _x, T _y, T _z) : Tvec3()
    {
        x = _x;
        y = _y;
        z = _z;
    }

    Tvec3<T> operator=(const Tvec3<T> &copy)
    {
        if (this == &copy)
            return *this;
        components[0] = copy.x;
        components[1] = copy.y;
        components[2] = copy.z;
        return *this;
    }
    Tvec3<T> operator+=(const Tvec3<T> &rhs)
    {
        *this = *this + rhs;
        return *this;
    }
    Tvec3<T> operator-=(const Tvec3<T> &rhs)
    {
        *this = *this - rhs;
        return *this;
    }
    Tvec3<T> operator*=(T scalar)
    {
        *this = *this * scalar;
        return *this;
    }
    bool operator==(const Tvec3<T> &rhs) const
    {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }
    Tvec3<T> operator+(const Tvec3<T> &rhs) const
    {
        return Tvec3(x + rhs.x, y + rhs.y, z + rhs.z);
    }
    Tvec3<T> operator-(const Tvec3<T> &rhs) const
    {
        return Tvec3(x - rhs.x, y - rhs.y, z - rhs.z);
    }
    Tvec3<T> operator*(T scalar) const
    {
        return Tvec3(x * scalar, y * scalar, z * scalar);
    }

    T dot(const Tvec3<T> &rhs) const
    {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }

    // Primary template: for float (and other floating-point types)
    T magnitude() const
    {
        return static_cast<T>(
            std::sqrt(static_cast<double>(x * x + y * y + z * z)));
    }

    Tvec3<T> unit() const
    {
        T mag = magnitude();
        if (mag == 0)
        {
            return Tvec3(0, 0, 0); // defined behavior for zero vector
        }
        return Tvec3(x / mag, y / mag, z / mag);
    }

    Tvec3<T> cross(const Tvec3<T> &rhs) const
    {
        return Tvec3(
            y * rhs.z - z * rhs.y,
            z * rhs.x - x * rhs.z,
            x * rhs.y - y * rhs.x);
    }

    T& operator[](int index)
    {
        return components[index];
    }

};

// Specialization for ivec3: magnitude returns int
// Implementation: Manhattan distance (L1 norm) for integer vectors
template <>
inline int Tvec3<int>::magnitude() const
{
    return std::abs(x) + std::abs(y) + std::abs(z);
}

// Specialization for ivec3: unit vector with integer division
// Uses Manhattan magnitude with integer division; returns (0,0,0) for zero vector
template <>
inline Tvec3<int> Tvec3<int>::unit() const
{
    int mag = magnitude();
    if (mag == 0)
    {
        return Tvec3(0, 0, 0); // defined behavior for zero vector
    }
    return Tvec3(x / mag, y / mag, z / mag); // integer division by L1 magnitude
}

typedef Tvec3<float> vec3;
typedef Tvec3<int> ivec3;

#endif
