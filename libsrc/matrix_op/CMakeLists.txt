project(matrix_op)

set(MATRIX_OP_LIBNAME ${PROJECT_NAME}_lib)
set(MATRIX_OP_SRC_FILES
    src/matrix.cpp
)

add_library(${MATRIX_OP_LIBNAME} STATIC ${MATRIX_OP_SRC_FILES})
