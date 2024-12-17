cmake_minimum_required(VERSION 3.20)

project("matrixcalc_root")

set(
    CXX_STANDARD 20
)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# -std=c++20 - некоторые CMake не понимают CXX_STANDART
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

# 1. third-party
set(protobuf_INSTALL OFF)
set(protobuf_BUILD_TESTS OFF)
set(protobuf_BUILD_CONFORMANCE OFF)
set(protobuf_BUILD_EXAMPLES OFF)
set(protobuf_BUILD_PROTOC_BINARIES ON)
set(protobuf_BUILD_SHARED_LIBS OFF)
set(ABSL_PROPAGATE_CXX_STD ON)
add_subdirectory(third-party/protobuf)

add_subdirectory(third-party/Catch2)
add_subdirectory(third-party/cxxopts)


# Далее наши проекты - собираем со строгими опциями
# FIXME: missing-requires -- иначе протобуф не собирается
add_compile_options(-Wall -Wextra -Werror -Wno-error=missing-requires)
include_directories(${CMAKE_SOURCE_DIR}/include)

# 2. Библиотеки
add_subdirectory(libsrc/matrix_op)
add_subdirectory(libsrc/executor)

# 3. Проекты
add_subdirectory(projects/protogen)
add_subdirectory(projects/matrix_service)

# 4. Тесты
add_subdirectory(tests/unit_tests)
