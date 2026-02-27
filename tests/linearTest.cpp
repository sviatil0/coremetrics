#include <iostream>
#include <cmath>
#include "linear.hpp"

// Helper function for floating point comparison
bool floatEqual(float a, float b, float epsilon = 0.0001f)
{
    return std::fabs(a - b) < epsilon;
}

void testVec2()
{
    std::cout << "=== Testing vec2 ===" << '\n';

    // Test default constructor
    vec2 v0;
    if (v0.x == 0.0f && v0.y == 0.0f)
    {
        std::cout << "vec2 default constructor: PASS" << '\n';
    }
    else
    {
        std::cout << "vec2 default constructor: FAIL" << '\n';
    }

    // Test parameterized constructor
    vec2 v1(3.0f, 4.0f);
    if (v1.x == 3.0f && v1.y == 4.0f)
    {
        std::cout << "vec2 parameterized constructor: PASS" << '\n';
    }
    else
    {
        std::cout << "vec2 parameterized constructor: FAIL" << '\n';
    }

    // Test copy constructor
    vec2 v_copy(v1);
    if (v_copy.x == 3.0f && v_copy.y == 4.0f)
    {
        std::cout << "vec2 copy constructor: PASS" << '\n';
    }
    else
    {
        std::cout << "vec2 copy constructor: FAIL" << '\n';
    }

    // Test assignment operator
    vec2 v_assign;
    v_assign = v1;
    if (v_assign.x == 3.0f && v_assign.y == 4.0f)
    {
        std::cout << "vec2 assignment operator: PASS" << '\n';
    }
    else
    {
        std::cout << "vec2 assignment operator: FAIL" << '\n';
    }

    // Test equality operator
    vec2 v_eq(3.0f, 4.0f);
    if (v1 == v_eq)
    {
        std::cout << "vec2 operator==: PASS" << '\n';
    }
    else
    {
        std::cout << "vec2 operator==: FAIL" << '\n';
    }

    // Test addition
    vec2 v2(1.0f, 2.0f);
    vec2 result = v1 + v2;
    if (result.x == 4.0f && result.y == 6.0f)
    {
        std::cout << "vec2 operator+: PASS" << '\n';
    }
    else
    {
        std::cout << "vec2 operator+: FAIL" << '\n';
    }

    // Test subtraction
    result = v1 - v2;
    if (result.x == 2.0f && result.y == 2.0f)
    {
        std::cout << "vec2 operator-: PASS" << '\n';
    }
    else
    {
        std::cout << "vec2 operator-: FAIL" << '\n';
    }

    // Test scalar multiplication
    result = v1 * 2.0f;
    if (result.x == 6.0f && result.y == 8.0f)
    {
        std::cout << "vec2 operator*: PASS" << '\n';
    }
    else
    {
        std::cout << "vec2 operator*: FAIL" << '\n';
    }

    // Test operator+=
    vec2 v3(1.0f, 1.0f);
    v3 += vec2(2.0f, 3.0f);
    if (v3.x == 3.0f && v3.y == 4.0f)
    {
        std::cout << "vec2 operator+=: PASS" << '\n';
    }
    else
    {
        std::cout << "vec2 operator+=: FAIL" << '\n';
    }

    // Test operator-=
    vec2 v4(5.0f, 7.0f);
    v4 -= vec2(1.0f, 1.0f);
    if (v4.x == 4.0f && v4.y == 6.0f)
    {
        std::cout << "vec2 operator-=: PASS" << '\n';
    }
    else
    {
        std::cout << "vec2 operator-=: FAIL" << '\n';
    }

    // Test operator*=
    vec2 v5(2.0f, 3.0f);
    v5 *= 2.0f;
    if (v5.x == 4.0f && v5.y == 6.0f)
    {
        std::cout << "vec2 operator*=: PASS" << '\n';
    }
    else
    {
        std::cout << "vec2 operator*=: FAIL" << '\n';
    }

    // Test operator[]
    vec2 v6(5.0f, 7.0f);
    if (v6[0] == 5.0f && v6[1] == 7.0f)
    {
        std::cout << "vec2 operator[]: PASS" << '\n';
    }
    else
    {
        std::cout << "vec2 operator[]: FAIL" << '\n';
    }

    // Test operator[] modification
    vec2 v7(1.0f, 2.0f);
    v7[0] = 10.0f;
    v7[1] = 20.0f;
    if (v7.x == 10.0f && v7.y == 20.0f)
    {
        std::cout << "vec2 operator[] modification: PASS" << '\n';
    }
    else
    {
        std::cout << "vec2 operator[] modification: FAIL" << '\n';
    }

    // Test dot product
    vec2 v8(3.0f, 4.0f);
    vec2 v9(1.0f, 2.0f);
    float dot = v8.dot(v9);
    if (floatEqual(dot, 11.0f))
    {
        std::cout << "vec2 dot product: PASS" << '\n';
    }
    else
    {
        std::cout << "vec2 dot product: FAIL (got " << dot << ", expected 11)" << '\n';
    }

    // Test magnitude
    vec2 v10(3.0f, 4.0f);
    float mag = v10.magnitude();
    if (floatEqual(mag, 5.0f))
    {
        std::cout << "vec2 magnitude: PASS" << '\n';
    }
    else
    {
        std::cout << "vec2 magnitude: FAIL (got " << mag << ", expected 5)" << '\n';
    }

    // Test unit vector
    vec2 v11(3.0f, 4.0f);
    vec2 unit = v11.unit();
    if (floatEqual(unit.x, 0.6f) && floatEqual(unit.y, 0.8f))
    {
        std::cout << "vec2 unit: PASS" << '\n';
    }
    else
    {
        std::cout << "vec2 unit: FAIL" << '\n';
    }

    // Test reference fields (x, y)
    vec2 v12(10.0f, 20.0f);
    v12.x = 15.0f;
    v12.y = 25.0f;
    if (v12[0] == 15.0f && v12[1] == 25.0f)
    {
        std::cout << "vec2 reference fields (x, y): PASS" << '\n';
    }
    else
    {
        std::cout << "vec2 reference fields (x, y): FAIL" << '\n';
    }

    std::cout << '\n';
}

void testIvec2()
{
    std::cout << "=== Testing ivec2 ===" << '\n';

    // Test default constructor
    ivec2 v0;
    if (v0.x == 0 && v0.y == 0)
    {
        std::cout << "ivec2 default constructor: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec2 default constructor: FAIL" << '\n';
    }

    // Test parameterized constructor
    ivec2 v1(3, 4);
    if (v1.x == 3 && v1.y == 4)
    {
        std::cout << "ivec2 parameterized constructor: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec2 parameterized constructor: FAIL" << '\n';
    }

    // Test copy constructor
    ivec2 v_copy(v1);
    if (v_copy.x == 3 && v_copy.y == 4)
    {
        std::cout << "ivec2 copy constructor: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec2 copy constructor: FAIL" << '\n';
    }

    // Test assignment operator
    ivec2 v_assign;
    v_assign = v1;
    if (v_assign.x == 3 && v_assign.y == 4)
    {
        std::cout << "ivec2 assignment operator: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec2 assignment operator: FAIL" << '\n';
    }

    // Test equality operator
    ivec2 v_eq(3, 4);
    if (v1 == v_eq)
    {
        std::cout << "ivec2 operator==: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec2 operator==: FAIL" << '\n';
    }

    // Test addition
    ivec2 v2(1, 2);
    ivec2 result = v1 + v2;
    if (result.x == 4 && result.y == 6)
    {
        std::cout << "ivec2 operator+: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec2 operator+: FAIL" << '\n';
    }

    // Test subtraction
    result = v1 - v2;
    if (result.x == 2 && result.y == 2)
    {
        std::cout << "ivec2 operator-: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec2 operator-: FAIL" << '\n';
    }

    // Test scalar multiplication
    result = v1 * 2;
    if (result.x == 6 && result.y == 8)
    {
        std::cout << "ivec2 operator*: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec2 operator*: FAIL" << '\n';
    }

    // Test operator+=
    ivec2 v3(1, 1);
    v3 += ivec2(2, 3);
    if (v3.x == 3 && v3.y == 4)
    {
        std::cout << "ivec2 operator+=: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec2 operator+=: FAIL" << '\n';
    }

    // Test operator-=
    ivec2 v4(5, 7);
    v4 -= ivec2(1, 1);
    if (v4.x == 4 && v4.y == 6)
    {
        std::cout << "ivec2 operator-=: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec2 operator-=: FAIL" << '\n';
    }

    // Test operator*=
    ivec2 v5(2, 3);
    v5 *= 2;
    if (v5.x == 4 && v5.y == 6)
    {
        std::cout << "ivec2 operator*=: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec2 operator*=: FAIL" << '\n';
    }

    // Test operator[]
    ivec2 v6(5, 7);
    if (v6[0] == 5 && v6[1] == 7)
    {
        std::cout << "ivec2 operator[]: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec2 operator[]: FAIL" << '\n';
    }

    // Test operator[] modification
    ivec2 v7(1, 2);
    v7[0] = 10;
    v7[1] = 20;
    if (v7.x == 10 && v7.y == 20)
    {
        std::cout << "ivec2 operator[] modification: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec2 operator[] modification: FAIL" << '\n';
    }

    // Test dot product
    ivec2 v8(3, 4);
    ivec2 v9(1, 2);
    int dot = v8.dot(v9);
    if (dot == 11)
    {
        std::cout << "ivec2 dot product: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec2 dot product: FAIL (got " << dot << ", expected 11)" << '\n';
    }

    // Test magnitude (Manhattan distance: |3| + |4| = 7)
    ivec2 v10(3, 4);
    int mag = v10.magnitude();
    if (mag == 7)
    {
        std::cout << "ivec2 magnitude (Manhattan L1): PASS" << '\n';
    }
    else
    {
        std::cout << "ivec2 magnitude (Manhattan L1): FAIL (got " << mag << ", expected 7)" << '\n';
    }

    // Test magnitude with negative values
    ivec2 v11(-3, -4);
    mag = v11.magnitude();
    if (mag == 7)
    {
        std::cout << "ivec2 magnitude with negatives: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec2 magnitude with negatives: FAIL" << '\n';
    }

    // Test unit vector (integer division by Manhattan magnitude)
    ivec2 v12(14, 21); // magnitude = 35
    ivec2 unit = v12.unit();
    if (unit.x == 0 && unit.y == 0) // 14/35=0, 21/35=0 due to integer division
    {
        std::cout << "ivec2 unit: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec2 unit: FAIL" << '\n';
    }

    // Test unit vector with zero magnitude
    ivec2 v13(0, 0);
    ivec2 zeroUnit = v13.unit();
    if (zeroUnit.x == 0 && zeroUnit.y == 0)
    {
        std::cout << "ivec2 unit (zero vector): PASS" << '\n';
    }
    else
    {
        std::cout << "ivec2 unit (zero vector): FAIL" << '\n';
    }

    std::cout << '\n';
}

void testVec3()
{
    std::cout << "=== Testing vec3 ===" << '\n';

    // Test default constructor
    vec3 v0;
    if (v0.x == 0.0f && v0.y == 0.0f && v0.z == 0.0f)
    {
        std::cout << "vec3 default constructor: PASS" << '\n';
    }
    else
    {
        std::cout << "vec3 default constructor: FAIL" << '\n';
    }

    // Test parameterized constructor
    vec3 v1(1.0f, 2.0f, 3.0f);
    if (v1.x == 1.0f && v1.y == 2.0f && v1.z == 3.0f)
    {
        std::cout << "vec3 parameterized constructor: PASS" << '\n';
    }
    else
    {
        std::cout << "vec3 parameterized constructor: FAIL" << '\n';
    }

    // Test copy constructor
    vec3 v_copy(v1);
    if (v_copy.x == 1.0f && v_copy.y == 2.0f && v_copy.z == 3.0f)
    {
        std::cout << "vec3 copy constructor: PASS" << '\n';
    }
    else
    {
        std::cout << "vec3 copy constructor: FAIL" << '\n';
    }

    // Test assignment operator
    vec3 v_assign;
    v_assign = v1;
    if (v_assign.x == 1.0f && v_assign.y == 2.0f && v_assign.z == 3.0f)
    {
        std::cout << "vec3 assignment operator: PASS" << '\n';
    }
    else
    {
        std::cout << "vec3 assignment operator: FAIL" << '\n';
    }

    // Test equality operator
    vec3 v_eq(1.0f, 2.0f, 3.0f);
    if (v1 == v_eq)
    {
        std::cout << "vec3 operator==: PASS" << '\n';
    }
    else
    {
        std::cout << "vec3 operator==: FAIL" << '\n';
    }

    // Test addition
    vec3 v2(4.0f, 5.0f, 6.0f);
    vec3 result = v1 + v2;
    if (result.x == 5.0f && result.y == 7.0f && result.z == 9.0f)
    {
        std::cout << "vec3 operator+: PASS" << '\n';
    }
    else
    {
        std::cout << "vec3 operator+: FAIL" << '\n';
    }

    // Test subtraction
    result = v2 - v1;
    if (result.x == 3.0f && result.y == 3.0f && result.z == 3.0f)
    {
        std::cout << "vec3 operator-: PASS" << '\n';
    }
    else
    {
        std::cout << "vec3 operator-: FAIL" << '\n';
    }

    // Test scalar multiplication
    result = v1 * 2.0f;
    if (result.x == 2.0f && result.y == 4.0f && result.z == 6.0f)
    {
        std::cout << "vec3 operator*: PASS" << '\n';
    }
    else
    {
        std::cout << "vec3 operator*: FAIL" << '\n';
    }

    // Test operator+=
    vec3 v3(1.0f, 2.0f, 3.0f);
    v3 += vec3(1.0f, 1.0f, 1.0f);
    if (v3.x == 2.0f && v3.y == 3.0f && v3.z == 4.0f)
    {
        std::cout << "vec3 operator+=: PASS" << '\n';
    }
    else
    {
        std::cout << "vec3 operator+=: FAIL" << '\n';
    }

    // Test operator-=
    vec3 v4(5.0f, 7.0f, 9.0f);
    v4 -= vec3(1.0f, 1.0f, 1.0f);
    if (v4.x == 4.0f && v4.y == 6.0f && v4.z == 8.0f)
    {
        std::cout << "vec3 operator-=: PASS" << '\n';
    }
    else
    {
        std::cout << "vec3 operator-=: FAIL" << '\n';
    }

    // Test operator*=
    vec3 v5(1.0f, 2.0f, 3.0f);
    v5 *= 2.0f;
    if (v5.x == 2.0f && v5.y == 4.0f && v5.z == 6.0f)
    {
        std::cout << "vec3 operator*=: PASS" << '\n';
    }
    else
    {
        std::cout << "vec3 operator*=: FAIL" << '\n';
    }

    // Test operator[]
    vec3 v6(5.0f, 7.0f, 9.0f);
    if (v6[0] == 5.0f && v6[1] == 7.0f && v6[2] == 9.0f)
    {
        std::cout << "vec3 operator[]: PASS" << '\n';
    }
    else
    {
        std::cout << "vec3 operator[]: FAIL" << '\n';
    }

    // Test operator[] modification
    vec3 v7(1.0f, 2.0f, 3.0f);
    v7[0] = 10.0f;
    v7[1] = 20.0f;
    v7[2] = 30.0f;
    if (v7.x == 10.0f && v7.y == 20.0f && v7.z == 30.0f)
    {
        std::cout << "vec3 operator[] modification: PASS" << '\n';
    }
    else
    {
        std::cout << "vec3 operator[] modification: FAIL" << '\n';
    }

    // Test dot product
    vec3 v8(1.0f, 2.0f, 3.0f);
    vec3 v9(4.0f, 5.0f, 6.0f);
    float dot = v8.dot(v9);
    if (floatEqual(dot, 32.0f)) // 1*4 + 2*5 + 3*6 = 32
    {
        std::cout << "vec3 dot product: PASS" << '\n';
    }
    else
    {
        std::cout << "vec3 dot product: FAIL (got " << dot << ", expected 32)" << '\n';
    }

    // Test magnitude
    vec3 v10(3.0f, 4.0f, 0.0f);
    float mag = v10.magnitude();
    if (floatEqual(mag, 5.0f))
    {
        std::cout << "vec3 magnitude: PASS" << '\n';
    }
    else
    {
        std::cout << "vec3 magnitude: FAIL (got " << mag << ", expected 5)" << '\n';
    }

    // Test unit vector
    vec3 v11(3.0f, 4.0f, 0.0f);
    vec3 unit = v11.unit();
    if (floatEqual(unit.x, 0.6f) && floatEqual(unit.y, 0.8f) && floatEqual(unit.z, 0.0f))
    {
        std::cout << "vec3 unit: PASS" << '\n';
    }
    else
    {
        std::cout << "vec3 unit: FAIL" << '\n';
    }

    // Test cross product (i x j = k)
    vec3 i(1.0f, 0.0f, 0.0f);
    vec3 j(0.0f, 1.0f, 0.0f);
    vec3 cross = i.cross(j);
    if (cross.x == 0.0f && cross.y == 0.0f && cross.z == 1.0f)
    {
        std::cout << "vec3 cross product (i x j = k): PASS" << '\n';
    }
    else
    {
        std::cout << "vec3 cross product (i x j = k): FAIL" << '\n';
    }

    // Test cross product (j x k = i)
    vec3 k(0.0f, 0.0f, 1.0f);
    cross = j.cross(k);
    if (cross.x == 1.0f && cross.y == 0.0f && cross.z == 0.0f)
    {
        std::cout << "vec3 cross product (j x k = i): PASS" << '\n';
    }
    else
    {
        std::cout << "vec3 cross product (j x k = i): FAIL" << '\n';
    }

    // Test cross product (k x i = j)
    cross = k.cross(i);
    if (cross.x == 0.0f && cross.y == 1.0f && cross.z == 0.0f)
    {
        std::cout << "vec3 cross product (k x i = j): PASS" << '\n';
    }
    else
    {
        std::cout << "vec3 cross product (k x i = j): FAIL" << '\n';
    }

    // Test reference fields (x, y, z)
    vec3 v12(10.0f, 20.0f, 30.0f);
    v12.x = 15.0f;
    v12.y = 25.0f;
    v12.z = 35.0f;
    if (v12[0] == 15.0f && v12[1] == 25.0f && v12[2] == 35.0f)
    {
        std::cout << "vec3 reference fields (x, y, z): PASS" << '\n';
    }
    else
    {
        std::cout << "vec3 reference fields (x, y, z): FAIL" << '\n';
    }

    std::cout << '\n';
}

void testIvec3()
{
    std::cout << "=== Testing ivec3 ===" << '\n';

    // Test default constructor
    ivec3 v0;
    if (v0.x == 0 && v0.y == 0 && v0.z == 0)
    {
        std::cout << "ivec3 default constructor: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec3 default constructor: FAIL" << '\n';
    }

    // Test parameterized constructor
    ivec3 v1(1, 2, 3);
    if (v1.x == 1 && v1.y == 2 && v1.z == 3)
    {
        std::cout << "ivec3 parameterized constructor: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec3 parameterized constructor: FAIL" << '\n';
    }

    // Test copy constructor
    ivec3 v_copy(v1);
    if (v_copy.x == 1 && v_copy.y == 2 && v_copy.z == 3)
    {
        std::cout << "ivec3 copy constructor: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec3 copy constructor: FAIL" << '\n';
    }

    // Test assignment operator
    ivec3 v_assign;
    v_assign = v1;
    if (v_assign.x == 1 && v_assign.y == 2 && v_assign.z == 3)
    {
        std::cout << "ivec3 assignment operator: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec3 assignment operator: FAIL" << '\n';
    }

    // Test equality operator
    ivec3 v_eq(1, 2, 3);
    if (v1 == v_eq)
    {
        std::cout << "ivec3 operator==: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec3 operator==: FAIL" << '\n';
    }

    // Test addition
    ivec3 v2(4, 5, 6);
    ivec3 result = v1 + v2;
    if (result.x == 5 && result.y == 7 && result.z == 9)
    {
        std::cout << "ivec3 operator+: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec3 operator+: FAIL" << '\n';
    }

    // Test subtraction
    result = v2 - v1;
    if (result.x == 3 && result.y == 3 && result.z == 3)
    {
        std::cout << "ivec3 operator-: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec3 operator-: FAIL" << '\n';
    }

    // Test scalar multiplication
    result = v1 * 2;
    if (result.x == 2 && result.y == 4 && result.z == 6)
    {
        std::cout << "ivec3 operator*: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec3 operator*: FAIL" << '\n';
    }

    // Test operator+=
    ivec3 v3(1, 2, 3);
    v3 += ivec3(1, 1, 1);
    if (v3.x == 2 && v3.y == 3 && v3.z == 4)
    {
        std::cout << "ivec3 operator+=: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec3 operator+=: FAIL" << '\n';
    }

    // Test operator-=
    ivec3 v4(5, 7, 9);
    v4 -= ivec3(1, 1, 1);
    if (v4.x == 4 && v4.y == 6 && v4.z == 8)
    {
        std::cout << "ivec3 operator-=: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec3 operator-=: FAIL" << '\n';
    }

    // Test operator*=
    ivec3 v5(1, 2, 3);
    v5 *= 2;
    if (v5.x == 2 && v5.y == 4 && v5.z == 6)
    {
        std::cout << "ivec3 operator*=: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec3 operator*=: FAIL" << '\n';
    }

    // Test operator[]
    ivec3 v6(5, 7, 9);
    if (v6[0] == 5 && v6[1] == 7 && v6[2] == 9)
    {
        std::cout << "ivec3 operator[]: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec3 operator[]: FAIL" << '\n';
    }

    // Test operator[] modification
    ivec3 v7(1, 2, 3);
    v7[0] = 10;
    v7[1] = 20;
    v7[2] = 30;
    if (v7.x == 10 && v7.y == 20 && v7.z == 30)
    {
        std::cout << "ivec3 operator[] modification: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec3 operator[] modification: FAIL" << '\n';
    }

    // Test dot product
    ivec3 v8(1, 2, 3);
    ivec3 v9(4, 5, 6);
    int dot = v8.dot(v9);
    if (dot == 32) // 1*4 + 2*5 + 3*6 = 32
    {
        std::cout << "ivec3 dot product: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec3 dot product: FAIL (got " << dot << ", expected 32)" << '\n';
    }

    // Test magnitude (Manhattan distance: |3| + |4| + |5| = 12)
    ivec3 v10(3, 4, 5);
    int mag = v10.magnitude();
    if (mag == 12)
    {
        std::cout << "ivec3 magnitude (Manhattan L1): PASS" << '\n';
    }
    else
    {
        std::cout << "ivec3 magnitude (Manhattan L1): FAIL (got " << mag << ", expected 12)" << '\n';
    }

    // Test magnitude with negative values
    ivec3 v11(-3, -4, -5);
    mag = v11.magnitude();
    if (mag == 12)
    {
        std::cout << "ivec3 magnitude with negatives: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec3 magnitude with negatives: FAIL" << '\n';
    }

    // Test unit vector
    ivec3 v12(12, 24, 36); // magnitude = 72
    ivec3 unit = v12.unit();
    if (unit.x == 0 && unit.y == 0 && unit.z == 0) // All become 0 due to integer division
    {
        std::cout << "ivec3 unit: PASS" << '\n';
    }
    else
    {
        std::cout << "ivec3 unit: FAIL" << '\n';
    }

    // Test unit vector with zero magnitude
    ivec3 v13(0, 0, 0);
    ivec3 zeroUnit = v13.unit();
    if (zeroUnit.x == 0 && zeroUnit.y == 0 && zeroUnit.z == 0)
    {
        std::cout << "ivec3 unit (zero vector): PASS" << '\n';
    }
    else
    {
        std::cout << "ivec3 unit (zero vector): FAIL" << '\n';
    }

    // Test cross product (i x j = k)
    ivec3 i(1, 0, 0);
    ivec3 j(0, 1, 0);
    ivec3 cross = i.cross(j);
    if (cross.x == 0 && cross.y == 0 && cross.z == 1)
    {
        std::cout << "ivec3 cross product (i x j = k): PASS" << '\n';
    }
    else
    {
        std::cout << "ivec3 cross product (i x j = k): FAIL" << '\n';
    }

    // Test cross product (j x k = i)
    ivec3 k(0, 0, 1);
    cross = j.cross(k);
    if (cross.x == 1 && cross.y == 0 && cross.z == 0)
    {
        std::cout << "ivec3 cross product (j x k = i): PASS" << '\n';
    }
    else
    {
        std::cout << "ivec3 cross product (j x k = i): FAIL" << '\n';
    }

    std::cout << '\n';
}

void testMatrix()
{
    std::cout << "=== Testing Matrix ===" << '\n';

    // Test default constructor (should initialize to zeros)
    Matrix m0;
    float zeroData[3][3] = {
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0}};
    Matrix zeroExpected(zeroData);
    if (m0 == zeroExpected)
    {
        std::cout << "Matrix default constructor: PASS" << '\n';
    }
    else
    {
        std::cout << "Matrix default constructor: FAIL" << '\n';
    }

    // Test parameterized constructor
    float data1[3][3] = {
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9}};
    Matrix m1(data1);

    // Test copy constructor
    Matrix m_copy(m1);
    if (m_copy == m1)
    {
        std::cout << "Matrix copy constructor: PASS" << '\n';
    }
    else
    {
        std::cout << "Matrix copy constructor: FAIL" << '\n';
    }

    // Test assignment operator
    Matrix m_assign;
    m_assign = m1;
    if (m_assign == m1)
    {
        std::cout << "Matrix assignment operator: PASS" << '\n';
    }
    else
    {
        std::cout << "Matrix assignment operator: FAIL" << '\n';
    }

    // Test equality operator
    float data1_copy[3][3] = {
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9}};
    Matrix m1_eq(data1_copy);
    if (m1 == m1_eq)
    {
        std::cout << "Matrix operator==: PASS" << '\n';
    }
    else
    {
        std::cout << "Matrix operator==: FAIL" << '\n';
    }

    // Test matrix multiplication (identity matrix)
    float identityData[3][3] = {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1}};
    Matrix identity(identityData);
    Matrix result = m1 * identity;
    if (result == m1)
    {
        std::cout << "Matrix multiplication (identity): PASS" << '\n';
    }
    else
    {
        std::cout << "Matrix multiplication (identity): FAIL" << '\n';
    }

    // Test matrix multiplication (general case)
    float data2[3][3] = {
        {9, 8, 7},
        {6, 5, 4},
        {3, 2, 1}};
    Matrix m2(data2);
    result = m1 * m2;

    // Expected result:
    // Row 0: 1*9+2*6+3*3=30, 1*8+2*5+3*2=24, 1*7+2*4+3*1=18
    // Row 1: 4*9+5*6+6*3=84, 4*8+5*5+6*2=69, 4*7+5*4+6*1=54
    // Row 2: 7*9+8*6+9*3=138, 7*8+8*5+9*2=114, 7*7+8*4+9*1=90
    float expectedData[3][3] = {
        {30, 24, 18},
        {84, 69, 54},
        {138, 114, 90}};
    Matrix expected(expectedData);
    if (result == expected)
    {
        std::cout << "Matrix multiplication: PASS" << '\n';
    }
    else
    {
        std::cout << "Matrix multiplication: FAIL" << '\n';
    }

    // Test transpose
    result = m1.toTranspose();
    float transposeData[3][3] = {
        {1, 4, 7},
        {2, 5, 8},
        {3, 6, 9}};
    expected = Matrix(transposeData);
    if (result == expected)
    {
        std::cout << "Matrix transpose: PASS" << '\n';
    }
    else
    {
        std::cout << "Matrix transpose: FAIL" << '\n';
    }

    // Test transpose property: (A^T)^T = A
    Matrix doubleTranspose = result.toTranspose();
    if (doubleTranspose == m1)
    {
        std::cout << "Matrix double transpose: PASS" << '\n';
    }
    else
    {
        std::cout << "Matrix double transpose: FAIL" << '\n';
    }

    // Test symmetric matrix transpose
    float symData[3][3] = {
        {1, 2, 3},
        {2, 4, 5},
        {3, 5, 6}};
    Matrix symmetric(symData);
    Matrix symTranspose = symmetric.toTranspose();
    if (symTranspose == symmetric)
    {
        std::cout << "Matrix symmetric transpose: PASS" << '\n';
    }
    else
    {
        std::cout << "Matrix symmetric transpose: FAIL" << '\n';
    }

    std::cout << '\n';
}

void linearTestSuite()
{
    std::cout << "=============================================" << '\n';
    std::cout << "  Linear Algebra Library - Unit Tests" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';

    testVec2();
    testIvec2();
    testVec3();
    testIvec3();
    testMatrix();
    std::cout << "=============================================" << '\n';
    std::cout << "  Linear Algebra Library - Unit Tests COMPLETED" << '\n';
    std::cout << "=============================================" << '\n';
    std::cout << '\n';
}