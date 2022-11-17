#!/bin/bash

source ./common.sh

SOURCE_VERSION="3.11.0"
FILENAME="Python-${SOURCE_VERSION}.tgz"

INSTALL_DIR=${CURREND_PWD}/thirdlib/python3
SOURCE_DIR=${CURREND_PWD}/src/python3
OPENSSL_DIR=`dirname ${INSTALL_DIR}`/openssl

rm -rf ${SOURCE_DIR} 2>&1 > /dev/null
mkdir -p ${SOURCE_DIR} && cd ${SOURCE_DIR}

wget "https://www.python.org/ftp/python/${SOURCE_VERSION}/${FILENAME}"
ErrorExit "下载失败！"

tar xvf ${FILENAME} && cd Python-${SOURCE_VERSION}
ErrorExit "进入源码目录失败！"

rm -rf ${INSTALL_DIR} 2>&1 > /dev/null
mkdir -p `dirname ${INSTALL_DIR}`
./configure --prefix=${INSTALL_DIR} --with-ensurepip=no --with-ssl-default-suites=python --with-openssl=${OPENSSL_DIR} --with-lto --with-computed-gotos --with-system-ffi LDFLAGS="-Wl,-rpath ${OPENSSL_DIR}/lib"
ErrorExit "执行 configure 失败！"

make clean && make -j "$(nproc)"
ErrorExit "执行 make 失败！"

make install
ErrorExit "执行 make install 失败！"

rm -rf ${SOURCE_DIR} 2>&1 > /dev/null
