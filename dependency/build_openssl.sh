#!/bin/bash

source ./common.sh

SOURCE_VERSION="1.1.1q"
FILENAME="openssl-${SOURCE_VERSION}.tar.gz"

INSTALL_DIR=${CURREND_PWD}/thirdlib/openssl
SOURCE_DIR=${CURREND_PWD}/src/openssl

rm -rf ${SOURCE_DIR} 2>&1 > /dev/null
mkdir -p ${SOURCE_DIR} && cd ${SOURCE_DIR}

wget "https://www.openssl.org/source/${FILENAME}" --no-check-certificate
ErrorExit "下载失败！"

tar xvf ${FILENAME} && cd openssl-${SOURCE_VERSION}
ErrorExit "进入源码目录失败！"

rm -rf ${INSTALL_DIR} 2>&1 > /dev/null
mkdir -p `dirname ${INSTALL_DIR}`
./config --prefix=${INSTALL_DIR} --openssldir=${INSTALL_DIR} shared zlib
ErrorExit "执行 configure 失败！"

make clean && make -j "$(nproc)"
ErrorExit "执行 make 失败！"

make test
ErrorExit "执行 make test 失败！"

make install
ErrorExit "执行 make install 失败！"

rm -rf ${SOURCE_DIR} 2>&1 > /dev/null
