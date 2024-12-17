project(matrix_service)

set(MATRIX_SERVICE_SRC_FILES
    src/main.cpp
    src/st_blocking_server.cpp
)

SET(MATRIX_SERVICE_NAME ${PROJECT_NAME})
add_executable(${MATRIX_SERVICE_NAME} ${MATRIX_SERVICE_SRC_FILES})

target_include_directories(${MATRIX_SERVICE_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/third-party/cxxopts/include)
target_link_libraries(${MATRIX_SERVICE_NAME} PRIVATE executor_lib cxxopts)
