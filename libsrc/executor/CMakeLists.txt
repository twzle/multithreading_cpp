project(executor)

SET(EXECUTOR_LIBNAME ${PROJECT_NAME}_lib)
set(EXECUTOR_SRC_FILES
    src/executor.cpp
    src/procedures.cpp
)

add_library(${EXECUTOR_LIBNAME} STATIC ${EXECUTOR_SRC_FILES})

target_include_directories(${EXECUTOR_LIBNAME} PRIVATE ${PROTO_CPP_HEADER_DIR})
target_link_libraries(${EXECUTOR_LIBNAME} PRIVATE matrix_op_lib protogen)
