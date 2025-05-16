# Registries should be involved. conan can not deal it with -Wl,--whole-archive
# mediapipe::stream_handler -Wl,--no-whole-archive The generated link.txt drop
# mediapipe::stream_handler out of the block, why?

# ==================== 特殊链接选项库定义 ==================== 
# 定义全链接模式接口库（解决符号未包含问题）
add_library(xgraph_stream_handler_whole_archive INTERFACE)

# 使用链接器参数强制包含整个库
target_link_libraries(
    xgraph_stream_handler_whole_archive
    INTERFACE 
    -Wl,--whole-archive     # 强制包含所有符号
    xgraph::stream_handler 
    -Wl,--no-whole-archive  # 恢复默认链接行为
)

# ==================== 特殊依赖分组库定义 ====================
# 创建 Abseil 分组接口库（处理循环依赖）
add_library(absl_group INTERFACE)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # 针对 Linux 系统使用分组链接
    target_link_libraries(absl_group INTERFACE 
        -Wl,--start-group   # 开始依赖分组
        abseil::abseil      # Abseil 核心库
        -Wl,--end-group     # 结束依赖分组
        pthread             # 附加 POSIX 线程库
    )
else()
    # 非 Linux 系统直接链接
    target_link_libraries(absl_group INTERFACE abseil::abseil)
endif()


# ==================== 主算法库配置 ==================== 
# 定义算法库名称
set(ALGORITHM_LIBRARY_NAME algorithm)

# 创建静态算法库
add_library(${ALGORITHM_LIBRARY_NAME} STATIC)

# ==================== 条件编译设置 ==================== 
if(NOT Xrlinux AND ENABLE_OPENXR_HANDJOINT_FORMAT) 
    target_compile_options(${ALGORITHM_LIBRARY_NAME} PRIVATE -DENABLE_OPENXR_HANDJOINT_FORMAT)
endif() 

# ==================== 包含目录配置 ====================
# 添加 SDK 头文件目录
set(HANDTRACKING_INCLUDES
    ${PROJECT_SOURCE_DIR}/interface/handtracking_sdk/include
    ${PROJECT_SOURCE_DIR}/interface/handtracking_sdk/include/channel
    ${PROJECT_SOURCE_DIR}/interface/handtracking_sdk/include/common
    ${PROJECT_SOURCE_DIR}/interface/handtracking_sdk/include/plugin
    ${PROJECT_SOURCE_DIR}/interface/handtracking_sdk/include/public
)
target_include_directories(${ALGORITHM_LIBRARY_NAME} PRIVATE ${HANDTRACKING_INCLUDES})

# ==================== 子目录模块集成 ==================== 
# 添加子模块构建
foreach(SUB_DIR common func model calculator)
    add_subdirectory(${SUB_DIR})
endforeach()

# ==================== 依赖链接配置 ==================== 
target_link_libraries(${ALGORITHM_LIBRARY_NAME}
    PUBLIC
    base
    xengine
    mano
    Eigen3::Eigen
    opencv::opencv
    xgraph_stream_handler_whole_archive
    xgraph::graph
    absl_group
    glog::glog
    protobuf::protobuf
    Ceres::ceres
)

# ==================== 平台特殊依赖 ==================== 
# 非 Xrlinux/Apple/iOS 平台链接 OpenBLAS
if(NOT Xrlinux AND NOT APPLE AND NOT IOS)
    find_package(openblas)  # 查找数学加速库
    target_link_libraries(${ALGORITHM_LIBRARY_NAME} PUBLIC
    openblas::openblas)     # 链接优化后的 BLAS 实现
endif()

# ==================== 编译选项配置 ====================
target_compile_options(${ALGORITHM_LIBRARY_NAME}
    PRIVATE -Wno-error
    PRIVATE -DNRPLUGIN
)

# ==================== 代码格式化配置 ==================== 
# 添加代码格式化目标（需确保已定义 add_clang_format_target 宏）
add_clang_format_target(${ALGORITHM_LIBRARY_NAME})
