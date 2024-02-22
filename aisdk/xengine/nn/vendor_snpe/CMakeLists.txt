target_sources(${XENHINE_LIBRARY_NAME} PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}/snpe_impl.cpp
${CMAKE_CURRENT_SOURCE_DIR}/CreateUserBuffer.cpp
)
target_link_libraries(${XENHINE_LIBRARY_NAME} PUBLIC SNPE)