project(protogen)

set(PROTO_NAMES "matrix_service" "matrix")
set(PROTO_FILES "")
set(PROTO_HEADER "")
set(PROTO_HEADER_ORIG "")
set(PROTO_CPP "")
set(PROTO_PY "")

set(PROTO_ETC_DIR ${PROJECT_SOURCE_DIR}/etc)
set(PROTO_CPP_HEADER_DIR ${PROJECT_SOURCE_DIR}/cpp/include)
set(PROTO_CPP_SRC_DIR ${PROJECT_SOURCE_DIR}/cpp/src)
set(PROTO_PY_DIR ${PROJECT_SOURCE_DIR}/py)
foreach(proto_name ${PROTO_NAMES})
    LIST(APPEND PROTO_FILES "${PROTO_ETC_DIR}/${proto_name}.proto")
    LIST(APPEND PROTO_HEADER "${PROTO_CPP_HEADER_DIR}/${proto_name}.pb.h")
    LIST(APPEND PROTO_HEADER_ORIG "${PROTO_CPP_SRC_DIR}/${proto_name}.pb.h")
    LIST(APPEND PROTO_CPP "${PROTO_CPP_SRC_DIR}/${proto_name}.pb.cc")
    LIST(APPEND PROTO_PY "${PROTO_PY_DIR}/${proto_name}_pb2.py")
endforeach()


add_custom_command(OUTPUT
    ${PROTO_CPP} ${PROTO_HEADER} ${PROTO_PY}
    COMMAND mkdir -p ${PROTO_CPP_HEADER_DIR} ${PROTO_CPP_SRC_DIR} ${PROTO_PY_DIR}
    COMMAND protoc --cpp_out ${PROTO_CPP_SRC_DIR} --python_out=${PROTO_PY_DIR} -I ${PROTO_ETC_DIR} ${PROTO_FILES}
    COMMAND mv ${PROTO_HEADER_ORIG} ${PROTO_CPP_HEADER_DIR}
    DEPENDS ${PROTO_FILES}
    VERBATIM
)

add_library(protogen STATIC ${PROTO_CPP})
target_include_directories(protogen PUBLIC
    ${PROTO_CPP_HEADER_DIR}
    ${CMAKE_SOURCE_DIR}/third-party/protobuf/src
)
target_link_libraries(protogen PUBLIC protobuf::libprotobuf)
