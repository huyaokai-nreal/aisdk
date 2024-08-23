# Registries should be involved. conan can not deal it with -Wl,--whole-archive
# mediapipe::stream_handler -Wl,--no-whole-archive The generated link.txt drop
# mediapipe::stream_handler out of the block, why?
# add_library(xgraph_stream_handler_whole_archive INTERFACE)
# target_link_libraries(
#     xgraph_stream_handler_whole_archive
#     INTERFACE -Wl,--whole-archive xgraph::stream_handler -Wl,--no-whole-archive)

# add_library(absl_group INTERFACE)
# if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
#     target_link_libraries(absl_group INTERFACE -Wl,--start_group abseil::abseil
#                                                 -Wl, --end-group pthread)
# else()
#     target_link_libraries(absl_group INTERFACE abseil::abseil)
# endif()

set(XGRAPH_LIBRARY_NAME xr_base_graph)
add_library(${XGRAPH_LIBRARY_NAME} STATIC
${CMAKE_CURRENT_SOURCE_DIR}/xr_base_graph.cpp
)

target_link_libraries(${XGRAPH_LIBRARY_NAME} PUBLIC 
base jsoncpp xgraph::xgraph
abseil::abseil glog::glog protobuf::protobuf)

install(TARGETS ${XGRAPH_LIBRARY_NAME}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
add_clang_format_target(${XGRAPH_LIBRARY_NAME})

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
add_subdirectory(reporter)
endif()