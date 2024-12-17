project(unit_tests)

set(UNIT_TESTS_SRC_FILES
    src/matrix_op.cpp
    src/executor.cpp
)

SET(UNIT_TESTS_NAME ${PROJECT_NAME})
add_executable(${UNIT_TESTS_NAME} ${UNIT_TESTS_SRC_FILES})

target_include_directories(${UNIT_TESTS_NAME} PRIVATE
    ${CMAKE_ROOT_DIR}/third-party/Catch2/src
)
target_link_libraries(${UNIT_TESTS_NAME} PRIVATE
    matrix_op_lib
    executor_lib
    protogen Catch2::Catch2WithMain
)
