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
add_library(${ALGORITHM_LIBRARY_NAME} STATIC
${CMAKE_CURRENT_SOURCE_DIR}/pipeline/nrcore_pipeline.cpp
${CMAKE_CURRENT_SOURCE_DIR}/core/nrcore_pipeline_mediapipe_service.cpp
${CMAKE_CURRENT_SOURCE_DIR}/pipeline/nrcore_pipeline_mediapipe_graph_impl.cpp
${CMAKE_CURRENT_SOURCE_DIR}/common/NR_Transfer.cpp
${CMAKE_CURRENT_SOURCE_DIR}/common/NR_Predictor.cpp
${CMAKE_CURRENT_SOURCE_DIR}/common/metrics.cpp
${CMAKE_CURRENT_SOURCE_DIR}/common/NR_Seq_Manager.cpp
${CMAKE_CURRENT_SOURCE_DIR}/pipeline/handtracking_mediapipe_calculators_register.cpp
${CMAKE_CURRENT_SOURCE_DIR}/pipeline/handtracking_mediapipe_graph.cpp
)

target_include_directories(${ALGORITHM_LIBRARY_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/interface/handtracking_sdk)
add_subdirectory(common)
add_subdirectory(func)
add_subdirectory(model)
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
)

target_compile_options(${ALGORITHM_LIBRARY_NAME}
    PRIVATE -Wno-error
    PRIVATE -DNRPLUGIN  # handtracking_sdk需要的宏定义
)

if(ENABLE_ALGORITHM_GRAPH_STREAM_EVAL_TIME)
    target_compile_options(${ALGORITHM_LIBRARY_NAME} PRIVATE -DENABLE_ALGORITHM_GRAPH_STREAM_EVAL_TIME)
endif()

if(ENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
    target_compile_options(${ALGORITHM_LIBRARY_NAME} PRIVATE -DENABLE_ALGORITHM_CALCULATOR_PROCESS_EVAL_TIME)
endif()

add_clang_format_target(${ALGORITHM_LIBRARY_NAME})
