#!/bin/bash

rm -rf build 2>&1 >/dev/null
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug .. && make

mkdir -p ../bin
cp -f ./gamesvr ../bin
cd - && rm -rf build
exit $?
