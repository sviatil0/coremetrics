#ifndef __MATRIX_HPP__
#define __MATRIX_HPP__
class Matrix
{
private:
    float components[3][3]{};

public:
    Matrix() = default;
    Matrix(float _data[3][3]);
    Matrix(const Matrix &copy);
    Matrix &operator=(const Matrix &copy);
    bool operator==(const Matrix &rhs) const;
    Matrix operator*(const Matrix &rhs) const;
    Matrix toTranspose() const;
};
#endif