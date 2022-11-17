#!/bin/bash

source ./common.sh

SOURCE_VERSION="21.9"
FILENAME="v${SOURCE_VERSION}.tar.gz"

INSTALL_DIR=${CURREND_PWD}/thirdlib/protobuf
SOURCE_DIR=${CURREND_PWD}/src/protobuf

rm -rf ${SOURCE_DIR} 2>&1 > /dev/null
mkdir -p ${SOURCE_DIR} && cd ${SOURCE_DIR}

wget "https://github.com/protocolbuffers/protobuf/archive/refs/tags/${FILENAME}"
ErrorExit "下载失败！"

tar xvf ${FILENAME} && cd protobuf-${SOURCE_VERSION}
ErrorExit "进入源码目录失败！"

rm -rf ${INSTALL_DIR} 2>&1 > /dev/null
mkdir -p `dirname ${INSTALL_DIR}`
./autogen.sh && ./configure --prefix=${INSTALL_DIR} --with-zlib CXX='g++ -std=c++11'
ErrorExit "执行 configure 失败！"

make -j "$(nproc)"
ErrorExit "执行 make 失败！"

make install
ErrorExit "执行 make install 失败！"

rm -rf ${SOURCE_DIR} 2>&1 > /dev/null
