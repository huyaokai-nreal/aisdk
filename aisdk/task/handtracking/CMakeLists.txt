add_library(handtracking STATIC
${CMAKE_CURRENT_SOURCE_DIR}/handtracking_calculators_register.cpp
${CMAKE_CURRENT_SOURCE_DIR}/handtracking_xgraph.cpp
${CMAKE_CURRENT_SOURCE_DIR}/handtracking_prior_glass_xgraph.cpp
${CMAKE_CURRENT_SOURCE_DIR}/handtracking_next_host_xgraph.cpp
${CMAKE_CURRENT_SOURCE_DIR}/base_xgraph.cpp
${CMAKE_CURRENT_SOURCE_DIR}/nrcore_pipeline.cpp

)
if(ENABLE_ALGORITHM_GRAPH_STREAM_EVAL_TIME)
    target_compile_options(handtracking PRIVATE -DENABLE_ALGORITHM_GRAPH_STREAM_EVAL_TIME)
endif()
if(ENABLE_ALGORITHM_DATA_RECORD)
    target_compile_options(handtracking PRIVATE -DENABLE_ALGORITHM_DATA_RECORD)
endif()
if(ENABLE_SEGMENT_JOINT_INFERENCE_MODE)
    target_compile_options(handtracking PRIVATE -DENABLE_SEGMENT_JOINT_INFERENCE_MODE)
endif()
if(NOT Xrlinux AND ENABLE_OPENXR_HANDJOINT_FORMAT) 
    target_compile_options(handtracking PRIVATE -DENABLE_OPENXR_HANDJOINT_FORMAT)
endif()    
target_link_libraries(handtracking PRIVATE base algorithm opencv::opencv)
target_include_directories(handtracking PRIVATE ${PROJECT_SOURCE_DIR}/interface/handtracking_sdk)
install(TARGETS handtracking
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
FILE(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(
  FILES
  ${HEADERS}
  DESTINATION
  ${AISDK_INCLUDE_INSTALL_DIR}/task/handtracking
)
target_compile_options(handtracking PRIVATE -Wno-error
    PRIVATE -Wno-error
    PRIVATE -DNRPLUGIN  # handtracking_sdk需要的宏定义
  )

add_clang_format_target(handtracking)
