#pragma once

#include <stdexcept>

namespace matrix_op {

class MatrixCalcError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

} // namespace matrix_op
