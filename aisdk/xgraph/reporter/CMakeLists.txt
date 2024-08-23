add_executable(print_profiler ${CMAKE_CURRENT_SOURCE_DIR}/print_profile.cpp
${CMAKE_CURRENT_SOURCE_DIR}/reporter.cpp
${CMAKE_CURRENT_SOURCE_DIR}/statistic.cpp
)
target_link_libraries(print_profiler PRIVATE xgraph::xgraph abseil::abseil glog::glog protobuf::protobuf)
target_compile_options(print_profiler
    PRIVATE -Wno-error
)
set_target_properties(print_profiler PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)