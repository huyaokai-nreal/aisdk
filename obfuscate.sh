#!/bin/bash
set -e
BIN=${OBFUSCATE_BIN}
# BIN=/home/zhengyi/projects/zjs/code/obfuscate
# BIN=./obfuscate

find aisdk -type f -iname "*.c" -or -iname "*.cpp" -or -iname "*.cc" | \
    egrep -v "xr_cv.cpp" | \
    egrep -v "warpaffine.cpp" | \
    egrep -v "log.h" | \
    egrep -v "profiling.h" | \
    egrep -v "nrhal_define.h" | \
    egrep -v "coord_transform_service.h" | \
    egrep -v "handtracking_xgraph.cpp" | \
    egrep -v "handtracking_prior_glass_xgraph.cpp" | \
    egrep -v "handtracking_next_host_xgraph.cpp" | \
    egrep -v "snpe_wrapper.cpp" | \
    xargs $BIN >> obfuscate.log


# 混淆编译失败的以下很多情况：
# xr_cv.cpp、warpaffine.cpp：   汇编 cpp
# coord_transform_service.h:    有 static_assert 语法
# log.h：      有 constexpr 语法
# profiling.h、nrhal_define.h:      有 __attribute__((visibility("default"))) 属性
# handtracking_xgraph.cpp、snpe_wrapper.cpp：   原本连接在一起多行，但是被格式化成多行的字符串： 如： "11112222" 变成  "1111""2222"