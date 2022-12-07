#!/bin/bash

BINARY_NAME="gamesvr"
GAMESVR_DIR=`pwd`
SOURCE_CODE_ROOT_DIR="${GAMESVR_DIR}/../.."
BUILD_DIR="${SOURCE_CODE_ROOT_DIR}/build"
DEPLOY_BIN_DIR="${SOURCE_CODE_ROOT_DIR}/bin"

mkdir -p ${DEPLOY_BIN_DIR}/${BINARY_NAME}

rm -rf ${BUILD_DIR} 2>&1 >/dev/null
mkdir ${BUILD_DIR} && cd ${BUILD_DIR}
cp CMakeLists.txt ${BUILD_DIR}
cmake -DCMAKE_BUILD_TYPE=Debug ${GAMESVR_DIR} && make
cp -f ./${BINARY_NAME} ${DEPLOY_BIN_DIR}/${BINARY_NAME}

#cd - && rm -rf ${BUILD_DIR}
exit $?
