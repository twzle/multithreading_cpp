#pragma once

#include "matrix_service.pb.h"
#include <stdexcept>

namespace matrix_service {

// Не фатальные ошибки, которые передаются пользователям в ответе
class ProcedureError : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};


// Частные случаи процедур
MatrixOpResponse RunProcedure(const MatrixOpRequest&);

} // namespace matrix_service
