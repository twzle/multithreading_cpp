#pragma once

#include "matrix_exception.hpp"

#include <cassert>
#include <cstdint>
#include <format>
#include <span>
#include <vector>

namespace matrix_op {

class Matrix
{
public:
    Matrix(std::uint32_t rows, std::uint32_t columns, const float* begin, const float* end)
        : rows_(rows),
          columns_(columns),
          matrix_(begin, end)
    {
        if (matrix_.size() != rows_ * columns_ || matrix_.empty()) [[unlikely]]
            throw MatrixCalcError(std::format("Data size {} != {} (r) x {} (c) <or> empty matrix", matrix_.size(), rows_, columns_));
    }

    Matrix(const Matrix&) = default;
    Matrix& operator=(const Matrix&) = default;
    Matrix(Matrix&&) = default;
    Matrix& operator=(Matrix&&) = default;

    std::span<const float> operator[](std::uint32_t row) const
    {
        assert(row < rows_);
        return std::span<const float>(matrix_.data() + columns_*row, columns_);
    }

    std::uint32_t Rows() const { return rows_; }
    std::uint32_t Columns() const { return columns_; }
    std::span<const float> Content() const { return matrix_; }

    friend Matrix operator*(const Matrix&, const Matrix&);

private:
    Matrix(std::uint32_t rows, std::uint32_t columns)
        : rows_(rows),
          columns_(columns),
          matrix_(rows_ * columns_)
    {}

    float& operator[](std::pair<std::uint32_t, std::uint32_t> rc)
    {
        assert(rc.first < rows_ && rc.second < columns_);
        return *(matrix_.data() + columns_*rc.first + rc.second);
    }

private:
    std::uint32_t rows_;
    std::uint32_t columns_;

    std::vector<float> matrix_;
};

} // namespace matrix_op
