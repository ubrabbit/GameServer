#!/bin/bash

source ./common.sh

INSTALL_DIR=${CURREND_PWD}/thirdlib/jemalloc
SOURCE_DIR=${CURREND_PWD}/src/jemalloc
SOURCE_VERSION="5.3.0"
FILENAME="${SOURCE_VERSION}.tar.gz"

rm -rf ${SOURCE_DIR} 2>&1 > /dev/null
mkdir -p ${SOURCE_DIR} && cd ${SOURCE_DIR}
wget "https://github.com/jemalloc/jemalloc/archive/refs/tags/${SOURCE_VERSION}.tar.gz"
ErrorExit "下载失败！"

tar xvf ${FILENAME} && cd jemalloc-${SOURCE_VERSION}
ErrorExit "进入源码目录失败！"

rm -rf ${INSTALL_DIR} 2>&1 > /dev/null
midir -p `dirname ${INSTALL_DIR}`
./autogen.sh --prefix=${INSTALL_DIR}
ErrorExit "执行 configure 失败！"

make clean && make -j "$(nproc)"
ErrorExit "执行 make 失败！"

make install
ErrorExit "执行 make install 失败！"

rm -rf ${SOURCE_DIR} 2>&1 > /dev/null
