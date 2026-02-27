#ifndef __VEC2_HPP__
#define __VEC2_HPP__

#include <cmath>

template <typename T>
class Tvec2
{
private:
	T components[2];

public:
	T &x, &y;

	Tvec2() : components{0, 0}, x(components[0]), y(components[1]) {}
	Tvec2(const Tvec2<T> &copy) : Tvec2()
	{
		x = copy.x;
		y = copy.y;
	}
	Tvec2(T _x, T _y) : Tvec2()
	{
		x = _x;
		y = _y;
	}

	Tvec2<T>& operator=(const Tvec2<T> &copy)
	{
		if (this == &copy)
		{
			return *this;
		}
		components[0] = copy.x;
		components[1] = copy.y;
		return *this;
	}
	bool operator==(const Tvec2<T> &rhs) const
	{
		return x == rhs.x && y == rhs.y;
	}
	Tvec2<T> operator+(const Tvec2<T> &rhs) const
	{
		return Tvec2(x + rhs.x, y + rhs.y);
	}
	Tvec2<T> operator-(const Tvec2<T> &rhs) const
	{
		return Tvec2(x - rhs.x, y - rhs.y);
	}
	Tvec2<T> operator*(T scalar) const
	{
		return Tvec2(x * scalar, y * scalar);
	}

	T dot(const Tvec2<T> &rhs) const
	{
		return x * rhs.x + y * rhs.y;
	}

	// Primary template: for float (and other floating-point types)
	T magnitude() const
	{
		return static_cast<T>(std::sqrt(static_cast<double>(x * x + y * y)));
	}

	Tvec2<T> unit() const
	{
		T mag = magnitude();
		if (mag == 0)
		{
			return Tvec2(0, 0); // defined behavior for zero vector
		}
		return Tvec2(x / mag, y / mag);
	}

	T& operator[](int index) 
	{
		return components[index];
	}

	Tvec2<T> operator+=(const Tvec2<T> &rhs)
	{
		*this = *this + rhs;
		return *this;
	}
	Tvec2<T> operator-=(const Tvec2<T> &rhs)
	{
		*this = *this - rhs;
		return *this;
	}
	Tvec2<T> operator*=(T scalar)
	{
		*this = *this * scalar;
		return *this;
	}
};

// Specialization for ivec2: magnitude returns int
// Implementation: Manhattan distance (L1 norm) for integer vectors
template <>
inline int Tvec2<int>::magnitude() const
{
	return std::abs(x) + std::abs(y);
}

// Specialization for ivec2: unit vector with integer division
// Uses Manhattan magnitude with integer division; returns (0,0) for zero vector
template <>
inline Tvec2<int> Tvec2<int>::unit() const
{
	int mag = magnitude();
	if (mag == 0)
	{
		return Tvec2(0, 0); // defined behavior for zero vector
	}
	return Tvec2(x / mag, y / mag); // integer division by L1 magnitude
}

typedef Tvec2<float> vec2;
typedef Tvec2<int> ivec2;

#endif
