cmake_minimum_required(VERSION 3.15)
project(aisdk VERSION 2.0.0 LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_SHARED_LIBS ON)
set(BUILD_TESTS ON)

if(CMAKE_SYSTEM_NAME MATCHES "^Android")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++11-narrowing")
    if(${ANDROID_ABI} MATCHES "arm64-v8a")
        # 开启特定汇编指令
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8.2-a+dotprod")
    endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm64")
        # 开启特定汇编指令
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8.2-a+dotprod")
    endif()
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release")
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

if (CMAKE_BUILD_TYPE MATCHES "Release")
    # 增加debug符号或者信息
    set(ENABLE_REL_WITH_DEB_INFO OFF)

    # 启动编译生成的so执行strip清理
    set(CMAKE_EXE_LINKER_FLAGS    "${CMAKE_EXE_LINKER_FLAGS}    -Wl,--gc-sections -Wl,-s -Wl,--exclude-libs,ALL")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -Wl,--gc-sections -Wl,-s -Wl,--exclude-libs,ALL")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--gc-sections -Wl,-s -Wl,--exclude-libs,ALL")
endif()

set(PROJECT_SRC_DIR "aisdk")
set(EXTERNAL_PROJECT_DIR ${PROJECT_SOURCE_DIR}/external/project)
list(APPEND CMAKE_MODULE_PATH "${EXTERNAL_PROJECT_DIR}/cmake" )
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
# build aisdk
# 构建标准业务的SDK
option(BUILD_HANDTRACKING_STD_SDK "build lib for std sdk" ON)
# 开启"Gina眼镜+手机的分段联合模式"，默认模式是全部业务都在手机上，注意这个和BUILD_HANDTRACKING_STD_SDK都要开启才有效
option(ENABLE_SEGMENT_JOINT_INFERENCE_MODE "enable prior inference 'detect and key2d model' in glass" OFF)
# 构建fptool工具业务的SDK，暂时未实现
option(BUILD_HANDTRACKING_FPTOOL_SDK "build lib for detect fp_tool sdk" OFF)
# 开启以OpenXR定义手势关键点的实现
option(ENABLE_OPENXR_HANDJOINT_FORMAT "enable hand key_points in OpenXR format" ON)

if(Xrlinux AND ENABLE_SEGMENT_JOINT_INFERENCE_MODE)
    # Xrlinux是Flora/Gina眼镜所对应的操作系统的标签，等同于Android/Darwin/Linux等
    # 分段联合模式下，眼镜业务的模型包
    set(tar_ppls "xrlinux_ppls")
    set(tar_model "hand_tracking_gina")
elseif(Xrlinux AND NOT ENABLE_SEGMENT_JOINT_INFERENCE_MODE)
    # 暂不支持
    message(FATAL_ERROR "Xrlinux && !ENABLE_SEGMENT_JOINT_INFERENCE_MODE")
else()
    # 默认模式下的模型包
    set(tar_ppls "android_ppls")
    set(tar_model "hand_tracking")
endif()

option(BUILD_EXAMPLE "Build examples." ON)
option(BUILD_PYTHON "Build python wrapper." OFF)
option(BUILD_TOOL "Build tools."  OFF)
option(ENABLE_CLANG_TIDY_CHECK       "Analyze code with clang-tidy."                OFF)

# build algorithm
option(ENABLE_XGRAPH_PROFILER "enable mediapipe graph framework profiler."  OFF)
# 开启单帧在图的推理下耗时统计
option(ENABLE_ALGORITHM_GRAPH_STREAM_EVAL_TIME "enable algorithm mediapipe graph stream eval time."  OFF)
# 开启单帧在单模型的推理下耗时统计
option(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME "enable algorithm mediapipe calculator process eval time."  OFF)
# 开启数据录制功能
option(ENABLE_ALGORITHM_DATA_RECORD "enable algorithm data record/debug"  OFF)
if(ENABLE_ALGORITHM_DATA_RECORD)
    # 启动数据录制的方式
    # add_definitions(-DDATA_RECORD_METHOD="Gesture")
    # add_definitions(-DDATA_RECORD_METHOD="UnityButton")
    add_definitions(-DDATA_RECORD_METHOD="DebugConfig")
endif()
# 导出中间数据
option(ENABLE_EXPORT_DATA_RECORD "enable export algorithm data record as json file"  OFF)
if(ENABLE_EXPORT_DATA_RECORD)
    add_definitions(-DENABLE_EXPORT_DATA_RECORD)
endif()
# 开启M2P延时测试
option(ENBALE_M2P_DELAYED_TIME_PROFILER "enable gesture M2P delayed time profiler" OFF)
if(ENBALE_M2P_DELAYED_TIME_PROFILER)
    add_definitions(-DENBALE_M2P_DELAYED_TIME_PROFILER)
endif()
if(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
add_definitions(-DENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
endif()
if(ENABLE_XGRAPH_PROFILER)
add_definitions(-DENABLE_XGRAPH_PROFILER)
endif()

# build xengine nn
option(ENABLE_XENGINE_SHARED_LIB "enable xengine shared lib."  OFF)
if(NOT Xrlinux AND (CMAKE_SYSTEM_NAME STREQUAL "Android" OR CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    # 启动snpe推理引擎
    option(ENABLE_XENGINE_SNPE_NN "enable snpe nn."  ON)
    set(SNPE_VERSION "snpe-2.17.0" CACHE STRING "set snpe version.")
    message(STATUS "SNPE_VERSION=" ${SNPE_VERSION})
    # 启动snpe的Debug
    option(ENABLE_XENGINE_TRACE_SNPE_LOG "enable trace snpe log."  OFF)
    option(ENABLE_XENGINE_TRACE_SNPE_PROFILER_DIAGLOG "enable trace snpe profiler diaglog."  OFF)
    # 启动晓龙adsp/cdsp加速库
    option(ENABLE_HEXAGON_DSP_OP "enable hexagon dsp op."  OFF)
endif()
# 启动mnn推理引擎
option(ENABLE_XENGINE_MNN_NN "enable mnn nn."  ON)
if(Xrlinux)
    # 启动artosyn npu推理引擎
    option(ENABLE_XENGINE_ARTOSYN_NN "enable artosyn nn."  ON)
    set(ARTOSYN_VERSION "ar9481_0.17.02-00" CACHE STRING "set artosyn sdk version.")
    message(STATUS "ARTOSYN_VERSION=" ${ARTOSYN_VERSION})
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    # 启动tensorrt推理引擎
    option(ENABLE_XENGINE_TENSORRT_NN "enable tensorrt nn."  ON)
endif()

if(BUILD_HANDTRACKING_FPTOOL_SDK)
    # 目前互斥
    set(BUILD_HANDTRACKING_STD_SDK OFF CACHE INTERNAL "")
endif()

find_package(fmt REQUIRED)
find_package(camera_model REQUIRED)
find_package(jsoncpp REQUIRED)
find_package(spdlog REQUIRED)
add_compile_definitions(SPDLOG_FMT_EXTERNAL)
find_package(Eigen3 REQUIRED)
find_package(openssl REQUIRED)
find_package(OpenCV REQUIRED)
find_package(nreal_mnn REQUIRED)
if(NOT Xrlinux AND (CMAKE_SYSTEM_NAME STREQUAL "Android" OR CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    find_package(snpe REQUIRED)
elseif(Xrlinux)
    find_package(artosyn REQUIRED)
endif()
find_package(framework CONFIG REQUIRED)
find_package(xgraph REQUIRED)
find_package(absl REQUIRED)
find_package(protobuf REQUIRED)
find_package(glog REQUIRED)
find_package(Ceres REQUIRED)
find_package(gflags REQUIRED)
find_package(framework CONFIG REQUIRED)
# 移除-frtti或者-fno-rtti等对我们的影响
set_property(TARGET framework::framework
        PROPERTY INTERFACE_COMPILE_OPTIONS
        $<$<CONFIG:Release>:$<$<COMPILE_LANGUAGE:CXX>:-frtti>> APPEND)
message(STATUS "INTERFACE_COMPILE_OPTIONS: ${INTERFACE_COMPILE_OPTIONS}")
message(STATUS "framework_COMPILE_OPTIONS_RELEASE: ${framework_COMPILE_OPTIONS_RELEASE}")
message(STATUS "framework_COMPILE_OPTIONS_CXX_RELEASE: ${framework_COMPILE_OPTIONS_CXX_RELEASE}")
set(INCLUDES_DESTINATION include)
set(AISDK_INCLUDE_INSTALL_DIR ${INCLUDES_DESTINATION}/aisdk)
# generate version.h
configure_file(
  ${PROJECT_SOURCE_DIR}/cmake/version.h.in
  include/${PROJECT_NAME_LOWERCASE}/version.h
  @ONLY
)
install(
  FILES
  ${CMAKE_CURRENT_BINARY_DIR}/include/${PROJECT_NAME_LOWERCASE}/version.h
  DESTINATION
  ${AISDK_INCLUDE_INSTALL_DIR}
)
# add global include root
include_directories(${PROJECT_SOURCE_DIR})
# import clang-format tool
include(clang_format)
# set clang-tidy check
if(ENABLE_CLANG_TIDY_CHECK)
  message(STATUS "clang-tidy enabled")
  set(CMAKE_CXX_CLANG_TIDY clang-tidy -format-style=file)
endif()

# build model package
# 实时打包模型和配置
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
set(MODEL_TAR_NAME "handtracking_pipeline_v2.0.0.tar")
set(DEFAULT_PIPELINE_TAR_PATH "${CMAKE_CURRENT_BINARY_DIR}/${MODEL_TAR_NAME}.enc")
set(DEFAULT_PIPELINE_TAR_KEY_PATH "${CMAKE_CURRENT_BINARY_DIR}/${MODEL_TAR_NAME}.key")
if(NOT EXISTS ${DEFAULT_PIPELINE_TAR_PATH})
    message(FATAL_ERROR "${DEFAULT_PIPELINE_TAR_PATH} does not exist!!!!!!")
endif()
file(STRINGS ${DEFAULT_PIPELINE_TAR_KEY_PATH} file_content)
list(GET file_content 2 DEFAULT_PIPELINE_TAR_KEY)
message(STATUS "MODEL_KEY: ${DEFAULT_PIPELINE_TAR_KEY}")
# 模型包include到头文件中
configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/aisdk/xengine/util/models.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/aisdk/xengine/util/models.h"
)


# build aisdk
include(Project)

# build aisdk interface
if(ENABLE_XENGINE_SHARED_LIB)
    option(FORCE_USE_PUSH_XENGINE_SHARED_LIB "force use pushing xengine lib" OFF)
endif()
add_subdirectory(interface)

# build thirdparty
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

# Set git hook
execute_process(COMMAND git config core.hooksPath external/project/githook)

