cmake_minimum_required(VERSION 3.15)
project(aisdk VERSION 2.0.0 LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(BUILD_SHARED_LIBS ON)
set(BUILD_TESTS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++11-narrowing")

if(CMAKE_SYSTEM_NAME MATCHES "^Android")
    if(${ANDROID_ABI} MATCHES "arm64-v8a")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8.2-a+dotprod")
    endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "arm64")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8.2-a+dotprod")
    endif()
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release")
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()
if (CMAKE_BUILD_TYPE MATCHES "Release")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -Os")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os")
endif()

set(PROJECT_SRC_DIR "aisdk")
set(EXTERNAL_PROJECT_DIR ${PROJECT_SOURCE_DIR}/external/project)
list( APPEND CMAKE_MODULE_PATH "${EXTERNAL_PROJECT_DIR}/cmake" )
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

option(FORCE_USE_PUSH "build lib for pushing" OFF)
option(BUILD_HANDTRACKING_STD_SDK "build lib for std sdk" ON)
option(BUILD_HANDTRACKING_FPTOOL_SDK "build lib for detect fp_tool sdk" OFF)
option(BUILD_EXAMPLE "Build examples." ON)
option(BUILD_TOOL "Build tools."  OFF)
option(ENABLE_CLANG_TIDY_CHECK       "Analyze code with clang-tidy."                OFF)

if(BUILD_HANDTRACKING_FPTOOL_SDK)
    # 目前互斥
    set(BUILD_HANDTRACKING_STD_SDK OFF CACHE INTERNAL "")
endif()

if(FORCE_USE_PUSH)
add_definitions(-DFORCE_USE_PUSH=ON)
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
find_package(snpe REQUIRED)
find_package(xgraph REQUIRED)
find_package(abseil REQUIRED)
find_package(protobuf REQUIRED)
find_package(glog REQUIRED)
find_package(gflags REQUIRED)
find_package(framework CONFIG REQUIRED)
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

# build xengine model package
set(MODEL_TAR_NAME "handtracking_pipeline_v1.0.0.tar")
set(DEFAULT_PIPELINE_TAR_PATH "${CMAKE_CURRENT_SOURCE_DIR}/pipeline_tar/${MODEL_TAR_NAME}.enc")
set(DEFAULT_PIPELINE_TAR_KEY "${CMAKE_CURRENT_SOURCE_DIR}/pipeline_tar/${MODEL_TAR_NAME}.key")
file(STRINGS ${DEFAULT_PIPELINE_TAR_KEY} file_content)
list(GET file_content 2 DEFAULT_PIPELINE_TAR_KEY)
message(STATUS "MODEL_KEY: ${DEFAULT_PIPELINE_TAR_KEY}")
configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/aisdk/xengine/model/models.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/aisdk/xengine/model/models.h"
)

# build xengine nn
option(ENABLE_XENGINE_SNPE_NN "enable snpe nn."  ON)
set(SNPE_VERSION "snpe-2.17.0" CACHE STRING "set snpe version.")
message(STATUS "SNPE_VERSION=" ${SNPE_VERSION})
option(ENABLE_HEXAGON_DSP_OP "enable hexagon dsp op."  OFF)
option(ENABLE_XENGINE_MNN_NN "enable mnn nn."  ON)
option(ENABLE_XENGINE_ARTOSYN_NN "enable artosyn nn."  OFF)
option(ENABLE_XENGINE_TENSORRT_NN "enable tensorrt nn."  OFF)

# build aisdk
include(Project)

# build aisdk interface
add_subdirectory(interface)

# build thirdparty
add_subdirectory(thirdparty)

if(BUILD_EXAMPLE)
message(STATUS "enable build examples")
add_subdirectory(example)
endif()

if(BUILD_TOOL)
message(STATUS "enable build tools")
add_subdirectory(tool)
endif()

# Set git hook
execute_process(COMMAND git config core.hooksPath external/project/githook)
