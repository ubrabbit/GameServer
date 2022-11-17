#!/bin/bash

# https://blog.csdn.net/weixin_29146599/article/details/116691635

wget https://github.com/Kitware/CMake/releases/download/v3.23.4/cmake-3.23.4.tar.gz

tar xvf cmake-3.23.4.tar.gz
cd cmake-3.23.4
./bootstrap
gmake
gmake install
