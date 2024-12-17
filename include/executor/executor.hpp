#pragma once

#include <string>

namespace matrix_service {

// Исполняет процедуру, content должен содержать сериализованный протобуф Procedure,
// ответом также будет сериализованный протобуф Procedure.
// Второе поле - является ли исполнение успешным. Если нет, то proc_id==INVALID, а content содержит ошибку
std::pair<std::string, bool> ExecuteProcedure(std::string_view content);

} // namespace matrix_service
