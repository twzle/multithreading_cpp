#include "executor/executor.hpp"
#include "procedures.hpp"

#include "matrix_service.pb.h"

#include <cassert>
#include <tuple>
#include <utility>

namespace matrix_service {

namespace {

using ProvidedProcedures = std::tuple<
    /* Known procesures, аналог services в grpc */
    std::nullptr_t, // Все варианты в proto3 начинаются с 0, поэтому номера процедур начинаем с 1
    std::pair<
        matrix_service::MatrixOpRequest, // RequestT
        matrix_service::MatrixOpResponse // ResponseT
    >
>;

// Валидация консистентности ProvidedProcedures и констант в протобуфах
template<std::size_t Idx, typename ProcedureT>
struct ValidateProcedure
{
    static_assert(std::is_same_v<ProcedureT, std::pair<typename ProcedureT::first_type, typename ProcedureT::second_type>>);
    static constexpr bool valid = ProcedureT::first_type::ID == Idx && ProcedureT::second_type::ID == Idx;
};

template<typename ProceduresT, std::size_t... Ids>
constexpr bool ValidateProcedures(std::integer_sequence<std::size_t, Ids...>)
{
    // +1, т.к. первый - nullptr
    return ( true && ... && ValidateProcedure<Ids + 1, std::tuple_element_t<Ids + 1, ProceduresT>>::valid );
}

static_assert(std::is_same_v<std::nullptr_t, std::tuple_element_t<0, ProvidedProcedures>>);
static_assert(ValidateProcedures<ProvidedProcedures>(
                    std::make_integer_sequence<std::size_t, std::tuple_size_v<ProvidedProcedures> - 1>()
              ));


// Исполнение процедур
template<std::size_t Idx>
bool TryRunProcedure(const ProcedureData& request, std::string& response)
{
    if (request.proc_id() != Idx)
        return false;

    using RequestT = typename std::tuple_element_t<Idx, ProvidedProcedures>::first_type;
    RequestT request_proto;
    if (!request_proto.ParseFromArray(request.payload().data(), request.payload().size())) [[unlikely]]
        throw ProcedureError(std::format("Corrupted protobuf for procedure request with id {}!", Idx));

    static_assert(std::is_same_v<decltype(RunProcedure(request_proto)),
                                 typename std::tuple_element_t<Idx, ProvidedProcedures>::second_type>);
    auto response_proto = RunProcedure(request_proto);
    response = response_proto.SerializeAsString();

    return true;
}

} // namespace


std::pair<std::string, bool> ExecuteProcedure(std::string_view request)
{
    ProcedureData response_proto;

    try
    {
        ProcedureData request_proto;
        if (!request_proto.ParseFromArray(request.data(), request.size())) [[unlikely]]
            throw ProcedureError("Corrupted matrix_service::Procedure protobuf!");

        std::string response;
        auto try_run_procedures =
            []<std::size_t... Ids>(const ProcedureData& request_proto, std::string& response, std::integer_sequence<std::size_t, Ids...>)
            {
                bool was_executed = ( false || ... || TryRunProcedure<Ids + 1>(request_proto, response) );
                if (!was_executed) [[unlikely]]
                    throw ProcedureError(std::format("Unknown ProcedureId: {}", (int) request_proto.proc_id()));
            };

        try_run_procedures(request_proto, response,
                           std::make_integer_sequence<std::size_t, std::tuple_size_v<ProvidedProcedures> - 1>());

        response_proto.set_proc_id(request_proto.proc_id());
        *response_proto.mutable_payload() = response;
        return { response_proto.SerializeAsString(), true };
    }
    catch (const ProcedureError& e)
    {
        response_proto.set_proc_id(ProcedureData::ProcedureId::ProcedureData_ProcedureId_INVALID);
        *response_proto.mutable_payload() = e.what();
        return { response_proto.SerializeAsString(), false };
    }
}

} // namespace matrix_service
