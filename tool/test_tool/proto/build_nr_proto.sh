#!/bin/bash

# x86编译安装grpc
# git clone  https://github.com/grpc/grpc.git
# git checkout v1.48.0 v1.48.0
# cd grpc/
# mkdir -p cmake/build
# cd cmake/build
# cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=/usr/local/ -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON ../..
# make
# make install

export LD_LIBRARY_PATH=/usr/local/lib/:$LD_LIBRARY_PATH
protoc --grpc_out=./ --cpp_out=./ -I . --plugin=protoc-gen-grpc=/aisdk/build/build_with_android/conan_data/p/grpc3d07c7a9cd49a/p/bin/grpc_cpp_plugin nreal.ai.tool.proto