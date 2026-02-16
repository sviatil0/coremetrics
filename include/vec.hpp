#ifndef __VEC_HPP__
#define __VEC_HPP__

#include <cmath>

template <typename T>
class Tvec2
{
private:
	T components[2];

public:
	T &x, &y;

	Tvec2();
	Tvec2(const Tvec2 &copy);
	Tvec2(T _x, T _y);

	Tvec2 &operator=(const Tvec2 &copy);
	Tvec2 operator+(const Tvec2 &rhs) const;
	Tvec2 operator-(const Tvec2 &rhs) const;
	Tvec2 operator*(T scalar) const;
	bool operator==(const Tvec2 &rhs) const;
	Tvec2 &operator+=(const Tvec2 &rhs);
	Tvec2 &operator-=(const Tvec2 &rhs);
	Tvec2 &operator*=(T scalar);
	T &operator[](int index);
	const T &operator[](int index) const;

	T dot(const Tvec2 &rhs) const;
	T magnitude() const;
	Tvec2 unit() const;
};

typedef Tvec2<float> vec2;
typedef Tvec2<int> ivec2;

template <typename T>
class Tvec3
{
private:
	T components[3];

public:
	T &x, &y, &z;

	Tvec3();
	Tvec3(const Tvec3 &copy);
	Tvec3(T _x, T _y, T _z);

	Tvec3 &operator=(const Tvec3 &copy);
	bool operator==(const Tvec3 &rhs) const;
	Tvec3 operator+(const Tvec3 &rhs) const;
	Tvec3 operator-(const Tvec3 &rhs) const;
	Tvec3 operator*(T scalar) const;
	T &operator[](int index);
	const T &operator[](int index) const;
	Tvec3 &operator+=(const Tvec3 &rhs);
	Tvec3 &operator-=(const Tvec3 &rhs);
	Tvec3 &operator*=(T scalar);

	T dot(const Tvec3 &rhs) const;
	T magnitude() const;
	Tvec3 unit() const;
	Tvec3 cross(const Tvec3 &rhs) const;
};

typedef Tvec3<float> vec3;
typedef Tvec3<int> ivec3;

#endif
