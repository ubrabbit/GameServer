cmake_minimum_required(VERSION 3.20)

message("${CMAKE_CURRENT_SOURCE_DIR}")
set(SOURCE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../)
set(THIRDLIB_ROOT_DIR ${SOURCE_ROOT_DIR}/dependency/thirdlib)
message("THIRDLIB_ROOT_DIR: ${THIRDLIB_ROOT_DIR}")

set(PROJECT_NAME socket_client)
project(${PROJECT_NAME} LANGUAGES CXX)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()
set(CMAKE_CXX_FLAGS "-Wall -fpic -pipe")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include_directories(${SOURCE_ROOT_DIR})

# ================================ protobuf start ==========================================
include_directories(${THIRDLIB_ROOT_DIR}/protobuf/include/)
link_directories(${THIRDLIB_ROOT_DIR}/protobuf/lib/)

add_custom_target(COMPILE_PROTO ALL
    COMMAND mkdir -p ${SOURCE_ROOT_DIR}/proto/distcpp
    COMMAND mkdir -p ${SOURCE_ROOT_DIR}/proto/distpy
    COMMAND cd ${SOURCE_ROOT_DIR}/proto && bash build.sh
)

aux_source_directory(. SOURCES)
aux_source_directory(${SOURCE_ROOT_DIR}/proto SOURCES_PROTO)
aux_source_directory(${SOURCE_ROOT_DIR}/proto/distcpp SOURCES_PROTO_DISTCPP)
# ================================ protobuf finished ========================================

# add the executable
add_executable(${PROJECT_NAME}
    ${SOURCES}
    ${SOURCES_PROTO} ${SOURCES_PROTO_DISTCPP})

# ================================ link libraries start ==========================================
find_library(PROTOBUF_LIB NAMES protobuf PATHS ${THIRDLIB_ROOT_DIR}/protobuf/lib NO_DEFAULT_PATH)
message("PROTOBUF_LIB: ${PROTOBUF_LIB}")

target_link_libraries(${PROJECT_NAME} -ldl -lutil -lm -pthread)
target_link_libraries(${PROJECT_NAME} ${PROTOBUF_LIB})
# ================================ link libraries end ===========================================
