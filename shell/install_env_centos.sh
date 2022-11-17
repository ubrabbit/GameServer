#!/bin/bash

function install_openssl()
{
    wget https://www.openssl.org/source/openssl-1.1.1h.tar.gz --no-check-certificate
    tar xvf openssl-1.1.1h.tar.gz
    cd openssl-1.1.1h
    ./config --prefix=/data/home/user00/rigellv/openssl/openssl-1.1.1h --openssldir=/data/home/user00/rigellv/openssl/openssl-1.1.1h shared zlib
}

sudo yum install -y libffi-devel openssl-devel
sudo yum install -y epel-release jemalloc jemalloc-devel
