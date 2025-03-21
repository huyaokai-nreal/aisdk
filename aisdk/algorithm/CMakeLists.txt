# Registries should be involved. conan can not deal it with -Wl,--whole-archive
# mediapipe::stream_handler -Wl,--no-whole-archive The generated link.txt drop
# mediapipe::stream_handler out of the block, why?
add_library(xgraph_stream_handler_whole_archive INTERFACE)

target_link_libraries(
    xgraph_stream_handler_whole_archive
    INTERFACE -Wl,--whole-archive xgraph::stream_handler -Wl,--no-whole-archive)

add_library(absl_group INTERFACE)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(absl_group INTERFACE -Wl,--start-group abseil::abseil
                                                -Wl,--end-group pthread)
else()
    target_link_libraries(absl_group INTERFACE abseil::abseil)
endif()
## generate protobufs
set(ALGORITHM_LIBRARY_NAME algorithm)
add_library(${ALGORITHM_LIBRARY_NAME} STATIC)
if(NOT Xrlinux AND ENABLE_OPENXR_HANDJOINT_FORMAT) 
    target_compile_options(${ALGORITHM_LIBRARY_NAME} PRIVATE -DENABLE_OPENXR_HANDJOINT_FORMAT)
endif() 
target_include_directories(${ALGORITHM_LIBRARY_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/interface/handtracking_sdk/include)
target_include_directories(${ALGORITHM_LIBRARY_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/interface/handtracking_sdk/include/channel)
target_include_directories(${ALGORITHM_LIBRARY_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/interface/handtracking_sdk/include/common)
target_include_directories(${ALGORITHM_LIBRARY_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/interface/handtracking_sdk/include/plugin)
target_include_directories(${ALGORITHM_LIBRARY_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/interface/handtracking_sdk/include/public)

add_subdirectory(common)
add_subdirectory(func)
add_subdirectory(model)
if(Xrlinux)
    add_subdirectory(model_artosyn)
endif()
add_subdirectory(calculator)

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
if(NOT Xrlinux AND NOT APPLE AND NOT IOS)
 find_package(openblas)
 target_link_libraries(${ALGORITHM_LIBRARY_NAME} PUBLIC
 openblas::openblas)
endif()

target_compile_options(${ALGORITHM_LIBRARY_NAME}
    PRIVATE -Wno-error
    PRIVATE -DNRPLUGIN
)


add_clang_format_target(${ALGORITHM_LIBRARY_NAME})
