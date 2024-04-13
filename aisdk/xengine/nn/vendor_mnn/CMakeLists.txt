target_sources(${XENGINE_LIBRARY_NAME} PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}/mnn_impl.cpp
)
target_link_libraries(${XENGINE_LIBRARY_NAME} PUBLIC MNN)