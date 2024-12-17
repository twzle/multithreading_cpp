#include "procedures.hpp"

#include "matrix_op/matrix.hpp"
#include "matrix_op/matrix_exception.hpp"

#include <format>

namespace matrix_service {

namespace {

matrix_op::Matrix FromProto(const Matrix& m)
{
    if (m.content().size() != (int) (m.rows() * m.columns())) [[unlikely]]
        throw ProcedureError(std::format("Invalid matrix content size: {} != {} x {}", m.content_size(), m.rows(), m.columns()));

    return matrix_op::Matrix(m.rows(), m.columns(), m.content().data(), m.content().data() + m.content().size());
}

} // namespace


MatrixOpResponse RunProcedure(const MatrixOpRequest& request)
{
    if (request.op() != MatrixOpRequest::MUL) [[unlikely]]
        throw ProcedureError(std::format("Unsupported operation in MatrixOpRequest: {}", (int) request.op()));
    if (request.args_size() != 2) [[unlikely]]
        throw ProcedureError(std::format("Invalid count of args in MatrixOpRequest: {}", request.args_size()));

    MatrixOpResponse resp;
    try
    {
        matrix_op::Matrix m1 = FromProto(request.args()[0]);
        matrix_op::Matrix m2 = FromProto(request.args()[1]);

        matrix_op::Matrix multiplication = m1 * m2;

        auto* result_proto_matrix = resp.mutable_result();
        result_proto_matrix->set_rows(multiplication.Rows());
        result_proto_matrix->set_columns(multiplication.Columns());
        result_proto_matrix->mutable_content()->Assign(multiplication.Content().begin(), multiplication.Content().end());
    }
    catch(const matrix_op::MatrixCalcError& e)
    {
        *resp.mutable_error() = e.what();
    }

    return resp;
}

} // namespace matrix_service
