# ================================================================
# 项目元信息配置层
# ================================================================
cmake_minimum_required(VERSION 3.15)
project(aisdk            # 项目名
    VERSION 2.0.0        # 版本号
    LANGUAGES CXX C      # 语言（C++和C）
)


# ================================================================
# 全局构建系统配置层
# ================================================================
## 编译器标准配置
set(CMAKE_CXX_STANDARD 17)               # 强制C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)      # 编译器不支持C++17，cmake会终止并报错

## 基础构建选项
set(CMAKE_EXPORT_COMPILE_COMMANDS True)  # 生成compile_command.json，供代码分析工具使用
set(CMAKE_POSITION_INDEPENDENT_CODE ON)  # 启用位置无关代码（PLC），用于生成共享库so/dll
set(BUILD_SHARED_LIBS ON)                # 默认构建共享库so/dll，而不是静态库
set(BUILD_TESTS ON)                      # 启用测试构建

## 全局包含目录
include_directories(${PROJECT_SOURCE_DIR})

## 全局路径配置
set(INCLUDES_DESTINATION include)                                 # 构建时，头文件输出目录
set(AISDK_INCLUDE_INSTALL_DIR ${INCLUDES_DESTINATION}/aisdk)      # 安装时，头文件目标目录
set(PROJECT_SRC_DIR "aisdk")                                      # 源代码目录路径
set(EXTERNAL_PROJECT_DIR ${PROJECT_SOURCE_DIR}/external/project)  # 外部代码项目目录路径
list(APPEND CMAKE_MODULE_PATH                                     # 扩展cmake模块搜索路径，find_package()和include()会优先从这个路径查找
    "${EXTERNAL_PROJECT_DIR}/cmake" 
    "${PROJECT_SOURCE_DIR}/cmake"
)


# ================================================================
# 平台适配层
# ================================================================
## Android平台特定配置
if(CMAKE_SYSTEM_NAME MATCHES "^Android")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++11-narrowing")  # 抑制c++11 narrowing警告
    if(${ANDROID_ABI} MATCHES "arm64-v8a")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8.2-a+dotprod")  # arm64架构下启用ARMv8.2的dotprod指令集
    endif()
endif()

## macOS平台特定配置
if(CMAKE_SYSTEM_NAME MATCHES "Darwin|macOS")
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm64")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8.2-a+dotprod")  # macOS平台下，Apple Silicon芯片启用相同指令集
    endif()
endif()


# ================================================================
# 构建模式配置层
# ================================================================
## 默认构建类型设置
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release")
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

## Release模式优化配置
if (CMAKE_BUILD_TYPE MATCHES "Release")
    # 增加debug符号或者信息
    set(ENABLE_REL_WITH_DEB_INFO OFF)

    # 启动编译生成的so执行strip清理
    # -Wl,--gc-sections 移除未使用的代码段
    # -Wl,-s 去除符号表
    # -Wl,--exclude-libs,ALL 不导出依赖库符号
    set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS}    -Wl,--gc-sections -Wl,-s -Wl,--exclude-libs,ALL")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--gc-sections -Wl,-s -Wl,--exclude-libs,ALL")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections -Wl,-s -Wl,--exclude-libs,ALL")
endif()


# ================================================================
# 项目功能选项层
# ================================================================
## SDK构建模式选项
# 构建标准业务的SDK
option(BUILD_HANDTRACKING_STD_SDK "build lib for std sdk" ON)
# 开启"Gina眼镜+手机的分段联合模式"，默认模式是全部业务都在手机上，注意这个和BUILD_HANDTRACKING_STD_SDK都要开启才有效
option(ENABLE_SEGMENT_JOINT_INFERENCE_MODE "enable prior inference 'detect and key2d model' in glass" OFF)
# 构建fptool工具业务的SDK，暂时未实现
option(BUILD_HANDTRACKING_FPTOOL_SDK "build lib for detect fp_tool sdk" OFF)
# 开启以OpenXR定义手势关键点的实现
option(ENABLE_OPENXR_HANDJOINT_FORMAT "enable hand key_points in OpenXR format" ON)

## 辅助功能选项
option(BUILD_EXAMPLE "Build examples." ON)
option(BUILD_PYTHON "Build python wrapper." OFF)
option(BUILD_TOOL "Build tools."  OFF)

## 算法调试设置
option(ENABLE_XGRAPH_PROFILER "enable mediapipe graph framework profiler."  OFF)
# 开启单帧在图的推理下耗时统计
option(ENABLE_ALGORITHM_GRAPH_STREAM_EVAL_TIME "enable algorithm mediapipe graph stream eval time."  OFF)
# 开启单帧在单模型的推理下耗时统计
option(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME "enable algorithm mediapipe calculator process eval time."  OFF)
# 开启数据录制功能
option(ENABLE_ALGORITHM_DATA_RECORD "enable algorithm data record/debug"  OFF)
# 导出中间数据
option(ENABLE_EXPORT_DATA_RECORD "enable export algorithm data record as json file"  OFF)
# 开启M2P延时测试
option(ENBALE_M2P_DELAYED_TIME_PROFILER "enable gesture M2P delayed time profiler" OFF)

## 算法调试选项
if(ENABLE_ALGORITHM_DATA_RECORD)
    # 启动数据录制的方式
    # add_definitions(-DDATA_RECORD_METHOD="Gesture")
    # add_definitions(-DDATA_RECORD_METHOD="UnityButton")
    add_definitions(-DDATA_RECORD_METHOD="DebugConfig")
endif()
if(ENABLE_EXPORT_DATA_RECORD)
    add_definitions(-DENABLE_EXPORT_DATA_RECORD)
endif()
if(ENBALE_M2P_DELAYED_TIME_PROFILER)
    add_definitions(-DENBALE_M2P_DELAYED_TIME_PROFILER)
endif()
if(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
    add_definitions(-DENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
endif()
if(ENABLE_XGRAPH_PROFILER)
    add_definitions(-DENABLE_XGRAPH_PROFILER)
endif()

## 推理引擎配置选项
option(ENABLE_XENGINE_SHARED_LIB "enable xengine shared lib."  OFF)
option(ENABLE_XENGINE_MNN_NN "enable mnn nn."  ON)  # 启动mnn推理引擎

## 平台专用推理引擎配置选项
if(NOT Xrlinux AND (CMAKE_SYSTEM_NAME STREQUAL "Android" OR CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    # android和linux平台选项；启动snpe推理引擎
    option(ENABLE_XENGINE_SNPE_NN "enable snpe nn."  ON)
    set(SNPE_VERSION "snpe-2.17.0" CACHE STRING "set snpe version.")
    message(STATUS "SNPE_VERSION=" ${SNPE_VERSION})
    # 启动snpe的Debug
    option(ENABLE_XENGINE_TRACE_SNPE_LOG "enable trace snpe log."  OFF)
    option(ENABLE_XENGINE_TRACE_SNPE_PROFILER_DIAGLOG "enable trace snpe profiler diaglog."  OFF)
    # 启动晓龙adsp/cdsp加速库
    option(ENABLE_HEXAGON_DSP_OP "enable hexagon dsp op."  OFF)
elseif(Xrlinux)
    # xrlinux平台选项；启动artosyn npu推理引擎
    option(ENABLE_XENGINE_ARTOSYN_NN "enable artosyn nn."  ON)
    set(ARTOSYN_VERSION "ar9481_0.17.02-00" CACHE STRING "set artosyn sdk version.")
    message(STATUS "ARTOSYN_VERSION=" ${ARTOSYN_VERSION})
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # window平台选项；启动tensorrt推理引擎
    option(ENABLE_XENGINE_TENSORRT_NN "enable tensorrt nn."  ON)
endif()

if(ENABLE_XENGINE_SHARED_LIB)
    option(FORCE_USE_PUSH_XENGINE_SHARED_LIB "force use pushing xengine lib" OFF)
endif()

if(BUILD_HANDTRACKING_FPTOOL_SDK)
    # 目前互斥
    set(BUILD_HANDTRACKING_STD_SDK OFF CACHE INTERNAL "")
endif()


# ================================================================
# 依赖管理层
# ================================================================
## 1. 基础运行时依赖（这些依赖是跨平台通用的，提供基础功能支持）
find_package(fmt REQUIRED)                     # 格式化库（日志、序列化）
find_package(jsoncpp REQUIRED)                 # JSON 解析库
find_package(spdlog REQUIRED)                  # 高性能日志库
add_compile_definitions(SPDLOG_FMT_EXTERNAL)   # 启用外部 fmt 集成
find_package(Eigen3 REQUIRED)                  # 线性代数库（矩阵运算）
find_package(openssl REQUIRED)                 # 加密通信库 
find_package(OpenCV REQUIRED)                  # 计算机视觉库（图像处理）
find_package(protobuf REQUIRED)                # Protocol Buffers（序列化）
find_package(glog REQUIRED)                    # Google 日志库
find_package(Ceres REQUIRED)                   # 非线性优化库（SLAM 等场景）
find_package(gflags REQUIRED)                  # 命令行参数解析库

## 2. 第三方库依赖（项目功能模块的直接依赖项）
find_package(camera_model REQUIRED)            # 相机模型抽象层
find_package(nreal_mnn REQUIRED)               # Nreal 的 MNN 推理引擎
find_package(xgraph REQUIRED)                  # 项目自定义图计算框架
find_package(abseil REQUIRED)                  # Abseil 基础库（C++ 工具集）

## 3. 平台相关依赖（条件编译。根据不同平台选择不同的推理引擎或硬件加速库）
if(NOT Xrlinux AND (CMAKE_SYSTEM_NAME STREQUAL "Android" OR CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    # Android/Linux平台使用 SNPE 推理引擎（高通 DSP 加速）
    find_package(snpe REQUIRED)
elseif(Xrlinux)
    # Xrlinux平台使用 Artosyn NPU 加速
    find_package(artosyn REQUIRED)
endif()

## 4. 项目框架核心依赖
find_package(framework CONFIG REQUIRED)
message(STATUS "Framework version: ${framework_VERSION}")

## 5. 依赖后处理（编译选项调整）
# 移除-frtti或者-fno-rtti等对我们的影响
set_property(TARGET framework::framework
        PROPERTY INTERFACE_COMPILE_OPTIONS
        $<$<CONFIG:Release>:$<$<COMPILE_LANGUAGE:CXX>:-frtti>> APPEND)
message(STATUS "INTERFACE_COMPILE_OPTIONS: ${INTERFACE_COMPILE_OPTIONS}")
message(STATUS "framework_COMPILE_OPTIONS_RELEASE: ${framework_COMPILE_OPTIONS_RELEASE}")
message(STATUS "framework_COMPILE_OPTIONS_CXX_RELEASE: ${framework_COMPILE_OPTIONS_CXX_RELEASE}")


# ================================================================
# 资源生成层
# ================================================================
## 版本文件生成
configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/version.h.in
  include/${PROJECT_NAME_LOWERCASE}/version.h
  @ONLY
)

## 安装生成的版本文件到${AISDK_INCLUDE_INSTALL_DIR}
install(
  FILES
  ${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME_LOWERCASE}/version.h
  DESTINATION
  ${AISDK_INCLUDE_INSTALL_DIR}
)

## 模型打包配置
if(Xrlinux)
    set(tar_ppls "xrlinux_ppls")
    set(tar_model "hand_tracking_xrlinux")
else()
    set(tar_ppls "android_ppls")
    set(tar_model "hand_tracking_bp")
endif()

execute_process(
    COMMAND bash tool/tar_pipeline.sh ${CMAKE_CURRENT_BINARY_DIR} ${tar_ppls} ${tar_model} 
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}   # 可选：指定工作目录
    RESULT_VARIABLE result_code                     # 可选：捕获退出状态码
    OUTPUT_VARIABLE output                          # 可选：捕获标准输出
    ERROR_VARIABLE error                            # 可选：捕获标准错误
)
message(STATUS "tool/tar_pipeline.sh result_code: ${result_code}")
message(STATUS "tool/tar_pipeline.sh output: ${output}")
message(STATUS "tool/tar_pipeline.sh error: ${error}")

# build xengine model package
set(MODEL_TAR_NAME "handtracking_pipeline_v2.0.0.tar")                                  # 模型包基础名称
set(DEFAULT_PIPELINE_TAR_PATH "${CMAKE_CURRENT_BINARY_DIR}/${MODEL_TAR_NAME}.enc")      # 加密包路径
set(DEFAULT_PIPELINE_TAR_KEY_PATH "${CMAKE_CURRENT_BINARY_DIR}/${MODEL_TAR_NAME}.key")  # 密钥路径
if(NOT EXISTS ${DEFAULT_PIPELINE_TAR_PATH})
    message(FATAL_ERROR "${DEFAULT_PIPELINE_TAR_PATH} does not exist!!!!!!")
endif()

file(STRINGS ${DEFAULT_PIPELINE_TAR_KEY_PATH} file_content)   # 读取密钥文件
list(GET file_content 2 DEFAULT_PIPELINE_TAR_KEY)             # 提取第三行作为密钥
message(STATUS "MODEL_KEY: ${DEFAULT_PIPELINE_TAR_KEY}")

# 模型包include到头文件中
configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/aisdk/xengine/util/models.h.in"  # 输入模板文件
    "${CMAKE_CURRENT_SOURCE_DIR}/aisdk/xengine/util/models.h"     # 输出目标文件
)


# ================================================================
# 代码质量层
# ================================================================
# 代码格式检查（集成clang-format和clang-tidy进行代码格式化/静态检查）
option(ENABLE_CLANG_TIDY_CHECK "Analyze code with clang-tidy." OFF)
include(clang_format)
if(ENABLE_CLANG_TIDY_CHECK)
    message(STATUS "clang-tidy enabled")
    set(CMAKE_CXX_CLANG_TIDY clang-tidy -format-style=file)
endif()


# ================================================================
# 子项目构建层
# ================================================================
# build aisdk
include(Project)

add_subdirectory(interface)
add_subdirectory(thirdparty)

if(BUILD_EXAMPLE)
    message(STATUS "enable build examples")
    add_subdirectory(example)
endif()

if(BUILD_PYTHON AND  CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "enable build python wrapper")
    add_subdirectory(python/aisdk)
endif()

if(BUILD_TOOL)
    message(STATUS "enable build tools")
    add_subdirectory(tool)
endif()


# ================================================================
# 后处理层
# ================================================================
# git钩子配置
execute_process(COMMAND git config core.hooksPath external/project/githook)
