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

set(ALGORITHM_LIBRARY_NAME algorithm)
add_library(${ALGORITHM_LIBRARY_NAME} STATIC
${CMAKE_CURRENT_SOURCE_DIR}/model/hand_detect.cc
${CMAKE_CURRENT_SOURCE_DIR}/model/hand_rsntiny.cc
${CMAKE_CURRENT_SOURCE_DIR}/model/landmark_filter.cc
${CMAKE_CURRENT_SOURCE_DIR}/model/hand_lift.cc

${CMAKE_CURRENT_SOURCE_DIR}/pipeline/nrcore_pipeline.cpp
${CMAKE_CURRENT_SOURCE_DIR}/core/nrcore_pipeline_mediapipe_service.cpp
${CMAKE_CURRENT_SOURCE_DIR}/calculator/hand_detection_calculator.cc

${CMAKE_CURRENT_SOURCE_DIR}/calculator/hand_landmark_calculator.cc
${CMAKE_CURRENT_SOURCE_DIR}/func/rsntiny_preprocess/warpaffine.cc

${CMAKE_CURRENT_SOURCE_DIR}/calculator/hand_state_calculator.cc

${CMAKE_CURRENT_SOURCE_DIR}/calculator/block_hard_rules_calculator.cc

${CMAKE_CURRENT_SOURCE_DIR}/common/ofxOneEuroFilter.cpp
${CMAKE_CURRENT_SOURCE_DIR}/common/NR_Seq_Manager.cpp

${CMAKE_CURRENT_SOURCE_DIR}/func/hand_filters.cpp

${CMAKE_CURRENT_SOURCE_DIR}/func/hand_rotation.cpp

${CMAKE_CURRENT_SOURCE_DIR}/pipeline/nrcore_pipeline_mediapipe_graph_impl.cpp

${CMAKE_CURRENT_SOURCE_DIR}/calculator/check_headpose_calculator.cc
)

add_subdirectory(func)

target_link_libraries(${ALGORITHM_LIBRARY_NAME}
    PUBLIC
    base
    xengine
    Eigen3::Eigen
    opencv::opencv
    xgraph_stream_handler_whole_archive
    xgraph::graph
    absl_group
    glog::glog
    protobuf::protobuf
)
target_compile_options(${ALGORITHM_LIBRARY_NAME} PRIVATE -Wno-error)

add_clang_format_target(${ALGORITHM_LIBRARY_NAME})
