#pragma once

#include <errno.h>
#include <string.h>

#include <format>
#include <stdexcept>

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

#define VALIDATE_LINUX_CALL(X) RaiseOnLinuxCallError(__LINE__, __FILE__, (X), #X, "<nothing>")
#define VALIDATE_LINUX_CALL_COMMENT(X, comment) RaiseOnLinuxCallError(__LINE__, __FILE__, (X), #X, comment)

} // namespace matrix_service

