#ifndef __MATRIX_HPP__
#define __MATRIX_HPP__

class Matrix
{
private:
    static constexpr int SIZE = 3;
    float components[SIZE][SIZE]{};

public:
    Matrix() = default;
    Matrix(float data[SIZE][SIZE]);
    Matrix(const Matrix &copy);

    Matrix &operator=(const Matrix &copy);
    Matrix operator*(const Matrix &rhs) const;
    bool operator==(const Matrix &rhs) const;

    Matrix toTranspose() const;
};

#endif
