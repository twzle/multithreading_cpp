#pragma once

#include <cstddef> // Для std::size_t

namespace matrix_service {

// Объявления функций
void RaiseLinuxCallError(std::size_t line, const char* file, const char* call_str, const char* comment);
void RaiseOnLinuxCallError(std::size_t line, const char* file, int call_result, const char* call_str, const char* comment);

// Макросы
#define VALIDATE_LINUX_CALL(X) RaiseOnLinuxCallError(__LINE__, __FILE__, (X), #X, "<nothing>")
#define VALIDATE_LINUX_CALL_COMMENT(X, comment) RaiseOnLinuxCallError(__LINE__, __FILE__, (X), #X, comment)

} // namespace matrix_service
