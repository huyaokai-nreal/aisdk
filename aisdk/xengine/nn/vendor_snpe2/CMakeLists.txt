target_sources(${XENHINE_LIBRARY_NAME} PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}/snpe_impl.cpp
${CMAKE_CURRENT_SOURCE_DIR}/snpe_lib_wrapper.cpp
${CMAKE_CURRENT_SOURCE_DIR}/snpe_wrapper.cpp
)
target_include_directories(${XENHINE_LIBRARY_NAME} PRIVATE ${snpe_INCLUDE_DIR})