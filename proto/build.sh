#!/bin/bash

CURRENT_PWD=`pwd`
SRC_DIR="${CURRENT_PWD}/src"
DST_DIR_CPP="${CURRENT_PWD}/distcpp"
DST_DIR_PY="${CURRENT_PWD}/distpy"

# ln -s ./../dependency/thirdlib/protobuf/bin/protoc protoc
${CURRENT_PWD}/protoc -I=$SRC_DIR $SRC_DIR/*.proto --cpp_out=$DST_DIR_CPP --python_out=$DST_DIR_PY
