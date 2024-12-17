#include "executor/executor.hpp"
#include "catch2/catch_test_macros.hpp"

#include "matrix_service.pb.h"

using namespace matrix_service;

namespace {

ProcedureData ParseResponse(std::size_t line, std::string_view response)
{
    CAPTURE(line);
    ProcedureData resp_proto;
    REQUIRE(resp_proto.ParseFromArray(response.data(), response.size()));
    return resp_proto;
}

std::string PackMatrixRequest(const MatrixOpRequest& payload_proto)
{
    ProcedureData request;
    request.set_proc_id(ProcedureData::ProcedureId::ProcedureData_ProcedureId_MATRIX_OP);
    *request.mutable_payload() = payload_proto.SerializeAsString();
    return request.SerializeAsString();
}


void CheckError(std::size_t line, std::string_view request)
{
    CAPTURE(line);
    auto result = ExecuteProcedure(request);
    CHECK(!result.second);

    ProcedureData resp_proto = ParseResponse(__LINE__, result.first);
    CHECK(resp_proto.proc_id() == ProcedureData::ProcedureId::ProcedureData_ProcedureId_INVALID);
}

MatrixOpResponse RunValidMatrixRequest(std::size_t line, const MatrixOpRequest& payload_proto)
{
    CAPTURE(line);

    auto res = ExecuteProcedure(PackMatrixRequest(payload_proto));
    CHECK(res.second);
    ProcedureData res_proto = ParseResponse(__LINE__, res.first);
    CHECK(res_proto.proc_id() == ProcedureData::ProcedureId::ProcedureData_ProcedureId_MATRIX_OP);

    MatrixOpResponse typed_res_proto;
    REQUIRE(typed_res_proto.ParseFromArray(res_proto.payload().data(), res_proto.payload().size()));
    return typed_res_proto;
}

} // namespace


TEST_CASE("Test executor", "[matrix_service]")
{
    CheckError(__LINE__, "qqq");

    ProcedureData request;
    CheckError(__LINE__, request.SerializeAsString());

    request.set_proc_id((ProcedureData::ProcedureId) (ProcedureData::ProcedureId_MAX + 1)); // Some invalid value
    CheckError(__LINE__, request.SerializeAsString());

    request.set_proc_id(ProcedureData::ProcedureId::ProcedureData_ProcedureId_MATRIX_OP);
    *request.mutable_payload() = "qqq"; // Bad payload
    CheckError(__LINE__, request.SerializeAsString());
}

TEST_CASE("Test matrix op", "[matrix_service]")
{
    MatrixOpRequest payload_proto;
    payload_proto.set_op((MatrixOpRequest::Operator) (MatrixOpRequest::Operator_MAX + 1)); // Unknown op
    CheckError(__LINE__, PackMatrixRequest(payload_proto));

    payload_proto.set_op(MatrixOpRequest::Operator::MatrixOpRequest_Operator_MUL);
    CheckError(__LINE__, PackMatrixRequest(payload_proto)); // Not enough args

    auto* m1 = payload_proto.add_args();
    auto* m2 = payload_proto.add_args();
    m1->set_rows(1);
    m1->set_columns(1);
    m1->mutable_content()->Add(1.f);

    m2->set_rows(1);
    m2->set_columns(2); // Wrong size: no enough values
    m2->mutable_content()->Add(2.f);
    CheckError(__LINE__, PackMatrixRequest(payload_proto));


    m2->set_columns(1);
    {
        MatrixOpResponse typed_res_proto = RunValidMatrixRequest(__LINE__, payload_proto);
        CHECK(typed_res_proto.has_result());
        CHECK(typed_res_proto.result().rows() == 1);
        CHECK(typed_res_proto.result().columns() == 1);
        REQUIRE(typed_res_proto.result().content_size() == 1);
        CHECK(typed_res_proto.result().content()[0] == 2.f);
    }


    m2->set_rows(2);
    m2->mutable_content()->Add(2); // (1x1) x (2x1)
    {
        // Not procedure error, but n typed response
        MatrixOpResponse typed_res_proto = RunValidMatrixRequest(__LINE__, payload_proto);
        CHECK(typed_res_proto.has_error());
        CHECK(!typed_res_proto.has_result());
    }
}
