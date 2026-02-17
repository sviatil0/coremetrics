#include "matrix.hpp"

Matrix Matrix::toTranspose() const
{
    Matrix result;
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            result.components[i][j] = components[j][i];
        }
    }
    return result;
};

Matrix::Matrix(float _data[3][3])
{
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            components[i][j] = _data[i][j];
        }
    }
}
Matrix::Matrix(const Matrix &copy)
{

    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            components[i][j] = copy.components[i][j];
        }
    }
}
Matrix &Matrix::operator=(const Matrix &copy)
{
    if (this == &copy)
    {
        return *this;
    }

    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            components[i][j] = copy.components[i][j];
        }
    }
    return *this;
}
Matrix Matrix::operator*(const Matrix &rhs) const
{

    Matrix result;

    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            float sum = 0.0f;
            for (int k = 0; k < 3; ++k)
            {
                sum += components[i][k] * rhs.components[k][j];
            }
            result.components[i][j] = sum;
        }
    }
    return result;
}
bool Matrix::operator==(const Matrix &rhs) const
{
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            if (components[i][j] != rhs.components[i][j])
                return false;
        }
    }
    return true;
}