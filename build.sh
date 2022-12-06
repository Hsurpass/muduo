#!/bin/bash

# ./build.sh -j2
# ./build.sh -j2 install
# BUILD_TYPE=debug ./build.sh -j2
# BUILD_TYPE=debug ./build.sh -j2 install
# BUILD_DIR=./build ./build.sh
# BUILD_DIR=./build ./build.sh install

# set是把命令打印到屏幕， set -x:开启, set +x:关闭, set -o是查看 (xtrace)，set去追中一段代码的显示情况。
# 执行set -x后，对整个脚本有效。
set -x
echo $*

SOURCE_DIR=`pwd`
# BUILD_DIR=${BUILD_DIR:-../build}
BUILD_DIR=${BUILD_DIR:-./build}
BUILD_TYPE=${BUILD_TYPE:-release}
INSTALL_DIR=${INSTALL_DIR:-../${BUILD_TYPE}-install-cpp11}
CXX=${CXX:-g++}

ln -sf $BUILD_DIR/$BUILD_TYPE-cpp11/compile_commands.json

# mkdir -p $BUILD_DIR/$BUILD_TYPE-cpp11
# cd $BUILD_DIR/$BUILD_TYPE-cpp11
# cmake $SOURCE_DIR -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
mkdir -p $BUILD_DIR/$BUILD_TYPE-cpp11 \
  && cd $BUILD_DIR/$BUILD_TYPE-cpp11 \
  && cmake $SOURCE_DIR \
           -DCMAKE_BUILD_TYPE=$BUILD_TYPE \
           -DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
           -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  && make $*

# Use the following command to run all the unit tests
# at the dir $BUILD_DIR/$BUILD_TYPE :
# CTEST_OUTPUT_ON_FAILURE=TRUE make test

# cd $SOURCE_DIR && doxygen

