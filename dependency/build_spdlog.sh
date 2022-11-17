#!/bin/bash

source ./common.sh

SOURCE_VERSION="1.9.2"
INSTALL_DIR=${CURREND_PWD}/thirdlib/spdlog
SOURCE_DIR=${CURREND_PWD}/src/spdlog

rm -rf ${SOURCE_DIR} 2>&1 > /dev/null
mkdir -p ${SOURCE_DIR} && cd ${SOURCE_DIR}

FILENAME="v${SOURCE_VERSION}.tar.gz"
wget "https://github.com/gabime/spdlog/archive/refs/tags/${FILENAME}"
ErrorExit "下载失败！"

tar xvf ${FILENAME} && cd spdlog-${SOURCE_VERSION}
ErrorExit "解压源码目录失败！"

rm -rf ${INSTALL_DIR} 2>&1 > /dev/null
mkdir -p `dirname ${INSTALL_DIR}`
cmake -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}/" .
ErrorExit "执行 cmake 失败！"

make clean && make -j "$(nproc)"
ErrorExit "执行 make 构建失败！"

make install
ErrorExit "执行 make install 失败！"

cd ${CURREND_PWD}
rm -rf ${SOURCE_DIR}
