#include "utility.hpp"

#include <cerrno>     // Для errno
#include <cstring>    // Для strerror
#include <stdexcept>  // Для std::runtime_error
#include <format>     // Для std::format

namespace matrix_service {

void RaiseLinuxCallError(std::size_t line, const char* file, const char* call_str, const char* comment)
{
    throw std::runtime_error(std::format(
            "System call '{}' on line {} in file {} failed (== -1). Errno = {} ({}). Comment: {}",
            call_str, line, file,
            errno, strerror(errno),
            comment
    ));
}

void RaiseOnLinuxCallError(std::size_t line, const char* file, int call_result, const char* call_str, const char* comment)
{
    if (call_result == -1) [[unlikely]]
        RaiseLinuxCallError(line, file, call_str, comment);
}

} // namespace matrix_service
