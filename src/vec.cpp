#include "vec.hpp"
#include <cmath>
#include <cstdlib>

// Tvec2 definitions
template <typename T>
Tvec2<T>::Tvec2() : components{0, 0}, x(components[0]), y(components[1]) {}

template <typename T>
Tvec2<T>::Tvec2(const Tvec2<T> &copy) : Tvec2()
{
    x = copy.x;
    y = copy.y;
}

template <typename T>
Tvec2<T>::Tvec2(T _x, T _y) : Tvec2()
{
    x = _x;
    y = _y;
}

template <typename T>
Tvec2<T> &Tvec2<T>::operator=(const Tvec2<T> &copy)
{
    if (this == &copy)
        return *this;
    components[0] = copy.x;
    components[1] = copy.y;
    return *this;
}
template <typename T>
bool Tvec2<T>::operator==(const Tvec2<T> &rhs) const
{
    return x == rhs.x && y == rhs.y;
}
template <typename T>
Tvec2<T> Tvec2<T>::operator+(const Tvec2<T> &rhs) const
{
    return Tvec2(x + rhs.x, y + rhs.y);
}

template <typename T>
Tvec2<T> Tvec2<T>::operator-(const Tvec2<T> &rhs) const
{
    return Tvec2(x - rhs.x, y - rhs.y);
}

template <typename T>
Tvec2<T> Tvec2<T>::operator*(T scalar) const
{
    return Tvec2(x * scalar, y * scalar);
}

template <typename T>
T Tvec2<T>::dot(const Tvec2<T> &rhs) const
{
    return x * rhs.x + y * rhs.y;
}

// Primary template: for float (and other floating-point types)
template <typename T>
T Tvec2<T>::magnitude() const
{
    return static_cast<T>(std::sqrt(static_cast<double>(x * x + y * y)));
}

template <typename T>
Tvec2<T> Tvec2<T>::unit() const
{
    T mag = magnitude();
    return Tvec2(x / mag, y / mag);
}

// Specialization for ivec2: magnitude returns int
// Implementation: Manhattan distance (L1 norm) for integer vectors
template <>
int Tvec2<int>::magnitude() const
{
    return std::abs(x) + std::abs(y);
}

// Specialization for ivec2: unit vector with integer division
// Uses Manhattan magnitude with integer division; returns (0,0) for zero vector
template <>
Tvec2<int> Tvec2<int>::unit() const
{
    int mag = magnitude();
    if (mag == 0)
    {
        return Tvec2(0, 0); // defined behavior for zero vector
    }
    return Tvec2(x / mag, y / mag); // integer division by L1 magnitude
}
template <typename T>
T &Tvec2<T>::operator[](int index)
{
    return components[index];
}

template <typename T>
const T &Tvec2<T>::operator[](int index) const
{
    return components[index];
}
template <typename T>
Tvec2<T> &Tvec2<T>::operator+=(const Tvec2<T> &rhs)
{
    *this = *this + rhs;
    return *this;
}

template <typename T>
Tvec2<T> &Tvec2<T>::operator-=(const Tvec2<T> &rhs)
{
    *this = *this - rhs;
    return *this;
}

template <typename T>
Tvec2<T> &Tvec2<T>::operator*=(T scalar)
{
    *this = *this * scalar;
    return *this;
}

// explicit instantiations for Tvec2
template class Tvec2<float>;
template class Tvec2<int>;

//  Tvec3 definitions
template <typename T>
Tvec3<T>::Tvec3() : components{0, 0, 0}, x(components[0]), y(components[1]), z(components[2]) {}

template <typename T>
Tvec3<T>::Tvec3(const Tvec3<T> &copy) : Tvec3()
{
    x = copy.x;
    y = copy.y;
    z = copy.z;
}

template <typename T>
Tvec3<T>::Tvec3(T _x, T _y, T _z) : Tvec3()
{
    x = _x;
    y = _y;
    z = _z;
}

template <typename T>
Tvec3<T> &Tvec3<T>::operator=(const Tvec3<T> &copy)
{
    if (this == &copy)
        return *this;
    components[0] = copy.x;
    components[1] = copy.y;
    components[2] = copy.z;
    return *this;
}

template <typename T>
Tvec3<T> &Tvec3<T>::operator+=(const Tvec3<T> &rhs)
{
    *this = *this + rhs;
    return *this;
}

template <typename T>
Tvec3<T> &Tvec3<T>::operator-=(const Tvec3<T> &rhs)
{
    *this = *this - rhs;
    return *this;
}

template <typename T>
Tvec3<T> &Tvec3<T>::operator*=(T scalar)
{
    *this = *this * scalar;
    return *this;
}
template <typename T>
bool Tvec3<T>::operator==(const Tvec3<T> &rhs) const
{
    return x == rhs.x && y == rhs.y && z == rhs.z;
}

template <typename T>
Tvec3<T> Tvec3<T>::operator+(const Tvec3<T> &rhs) const
{
    return Tvec3(x + rhs.x, y + rhs.y, z + rhs.z);
}

template <typename T>
Tvec3<T> Tvec3<T>::operator-(const Tvec3<T> &rhs) const
{
    return Tvec3(x - rhs.x, y - rhs.y, z - rhs.z);
}

template <typename T>
Tvec3<T> Tvec3<T>::operator*(T scalar) const
{
    return Tvec3(x * scalar, y * scalar, z * scalar);
}

template <typename T>
T Tvec3<T>::dot(const Tvec3<T> &rhs) const
{
    return x * rhs.x + y * rhs.y + z * rhs.z;
}

// Primary template: for float (and other floating-point types)
template <typename T>
T Tvec3<T>::magnitude() const
{
    return static_cast<T>(
        std::sqrt(static_cast<double>(x * x + y * y + z * z)));
}

template <typename T>
Tvec3<T> Tvec3<T>::unit() const
{
    T mag = magnitude();
    return Tvec3(x / mag, y / mag, z / mag);
}

// Specialization for ivec3: magnitude returns int
// Implementation: Manhattan distance (L1 norm) for integer vectors
template <>
int Tvec3<int>::magnitude() const
{
    return std::abs(x) + std::abs(y) + std::abs(z);
}

// Specialization for ivec3: unit vector with integer division
// Uses Manhattan magnitude with integer division; returns (0,0,0) for zero vector
template <>
Tvec3<int> Tvec3<int>::unit() const
{
    int mag = magnitude();
    if (mag == 0)
    {
        return Tvec3(0, 0, 0); // defined behavior for zero vector
    }
    return Tvec3(x / mag, y / mag, z / mag); // integer division by L1 magnitude
}
template <typename T>
Tvec3<T> Tvec3<T>::cross(const Tvec3<T> &rhs) const
{
    return Tvec3(
        y * rhs.z - z * rhs.y,
        z * rhs.x - x * rhs.z,
        x * rhs.y - y * rhs.x);
}

template <typename T>
T &Tvec3<T>::operator[](int index)
{
    return components[index];
}

template <typename T>
const T &Tvec3<T>::operator[](int index) const
{
    return components[index];
}

// explicit instantiations for Tvec3
template class Tvec3<float>;
template class Tvec3<int>;