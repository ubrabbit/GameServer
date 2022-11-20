cmake_minimum_required(VERSION 3.20)

message("${CMAKE_CURRENT_SOURCE_DIR}")
set(SOURCE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(THIRDLIB_ROOT_DIR ${SOURCE_ROOT_DIR}/dependency/thirdlib)

set(PROJECT_NAME gamesvr)
project(${PROJECT_NAME} LANGUAGES CXX C)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()
set(CMAKE_CXX_FLAGS "-Wall -fpic -pipe")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

include_directories(${SOURCE_ROOT_DIR})
include_directories(${THIRDLIB_ROOT_DIR}/jemalloc/include/)
include_directories(${THIRDLIB_ROOT_DIR}/python3/include/python3.11/)
include_directories(${THIRDLIB_ROOT_DIR}/libevent/include/)
include_directories(${THIRDLIB_ROOT_DIR}/spdlog/include/)

link_directories(${THIRDLIB_ROOT_DIR}/jemalloc/lib/)
link_directories(${THIRDLIB_ROOT_DIR}/libevent/lib/)

# ================================ protobuf start ==========================================
include_directories(${THIRDLIB_ROOT_DIR}/protobuf/include/)
link_directories(${THIRDLIB_ROOT_DIR}/protobuf/lib/)

add_custom_target(COMPILE_PROTO ALL
    COMMAND mkdir -p ${SOURCE_ROOT_DIR}/proto/distcpp
    COMMAND mkdir -p ${SOURCE_ROOT_DIR}/proto/distpy
    COMMAND cd ${SOURCE_ROOT_DIR}/proto && bash build.sh
)
# ================================ protobuf finished ========================================

aux_source_directory(. SOURCES)
aux_source_directory(${SOURCE_ROOT_DIR}/common SOURCES_COMMON)
aux_source_directory(${SOURCE_ROOT_DIR}/common/conf SOURCES_COMMON_CONF)
aux_source_directory(${SOURCE_ROOT_DIR}/common/utils SOURCES_COMMON_UTILS)

aux_source_directory(${SOURCE_ROOT_DIR}/network SOURCES_NETWORK)
aux_source_directory(${SOURCE_ROOT_DIR}/network/libevent SOURCES_NETWORK_LIBEVENT)
aux_source_directory(${SOURCE_ROOT_DIR}/network/socket SOURCES_NETWORK_SOCKETSVR)

aux_source_directory(${SOURCE_ROOT_DIR}/proto SOURCES_PROTO)
aux_source_directory(${SOURCE_ROOT_DIR}/proto/distcpp SOURCES_PROTO_DISTCPP)

# add the executable
add_executable(${PROJECT_NAME}
    ${SOURCES} ${SOURCES_COMMON} ${SOURCES_COMMON_CONF} ${SOURCES_COMMON_UTILS}
    ${SOURCES_NETWORK} ${SOURCES_NETWORK_LIBEVENT} ${SOURCES_NETWORK_SOCKETSVR}
    ${SOURCES_PROTO} ${SOURCES_PROTO_DISTCPP})

# ================================ link static libraries start ==========================================
if (WIN32 OR MSVC)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
elseif (UNIX)
    # 如果只是优先查找静态库，保证 .a 后缀在前面即可，把默认的后缀加上
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
endif()

find_library(JEMALLOC_LIB NAMES jemalloc PATHS ${THIRDLIB_ROOT_DIR}/jemalloc/lib/ NO_DEFAULT_PATH)
message("JEMALLOC_LIB: ${JEMALLOC_LIB}")

find_library(LIBEVENT_LIB NAMES event PATHS ${THIRDLIB_ROOT_DIR}/libevent/lib/ NO_DEFAULT_PATH)
message("LIBEVENT_LIB: ${LIBEVENT_LIB}")

find_library(LIBEVENT_LIB_CORE NAMES event_core PATHS ${THIRDLIB_ROOT_DIR}/libevent/lib/ NO_DEFAULT_PATH)
message("LIBEVENT_LIB_CORE: ${LIBEVENT_LIB_CORE}")

find_library(LIBEVENT_LIB_PTHREADS NAMES event_pthreads PATHS ${THIRDLIB_ROOT_DIR}/libevent/lib/ NO_DEFAULT_PATH)
message("LIBEVENT_LIB_PTHREADS: ${LIBEVENT_LIB_PTHREADS}")

find_library(PROTOBUF_LIB NAMES protobuf PATHS ${THIRDLIB_ROOT_DIR}/protobuf/lib NO_DEFAULT_PATH)
message("PROTOBUF_LIB: ${PROTOBUF_LIB}")

target_link_libraries(${PROJECT_NAME} -ldl -lutil -lm -pthread)
target_link_libraries(${PROJECT_NAME} ${JEMALLOC_LIB} ${LIBEVENT_LIB} ${LIBEVENT_LIB_CORE} ${LIBEVENT_LIB_PTHREADS} ${PROTOBUF_LIB})
# ================================ link static libraries end ===========================================
