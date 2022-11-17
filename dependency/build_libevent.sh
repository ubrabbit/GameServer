#!/bin/bash

source ./common.sh

SOURCE_VERSION="2.1.12"
INSTALL_DIR=${CURREND_PWD}/thirdlib/libevent
SOURCE_DIR=${CURREND_PWD}/src/libevent

rm -rf ${SOURCE_DIR} 2>&1 > /dev/null
mkdir -p ${SOURCE_DIR} && cd ${SOURCE_DIR}
wget "https://github.com/libevent/libevent/releases/download/release-${SOURCE_VERSION}-stable/libevent-${SOURCE_VERSION}-stable.tar.gz"
ErrorExit "下载失败！"

tar xvf libevent-${SOURCE_VERSION}-stable.tar.gz
ErrorExit "解压源码目录失败！"
cd libevent-${SOURCE_VERSION}-stable

rm -rf ${INSTALL_DIR} 2>&1 > /dev/null
mkdir -p `dirname ${INSTALL_DIR}`
cmake -DCMAKE_INSTALL_PREFIX="${INSTALL_DIR}/" .
ErrorExit "执行 cmake 失败！"

make
ErrorExit "执行 make 构建失败！"

make install
ErrorExit "执行 make install 失败！"

make verify
ErrorExit "执行 make verify 验证失败！"

cd ${CURREND_PWD} && rm -rf ${SOURCE_DIR}
