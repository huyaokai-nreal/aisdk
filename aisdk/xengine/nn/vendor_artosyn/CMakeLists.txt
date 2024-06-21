target_sources(${XENGINE_LIBRARY_NAME} PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}/artosyn_impl.cpp
)

# conan ARTOSYN 组件中缺少binder_ipc依赖
target_link_libraries(${XENGINE_LIBRARY_NAME} PUBLIC ARTOSYN_NPU)
target_include_directories(${XENGINE_LIBRARY_NAME} PRIVATE 
    ${artosyn_INCLUDE_DIR}/apps
    ${artosyn_INCLUDE_DIR}/hal
    ${artosyn_INCLUDE_DIR}/kmodule
    ${artosyn_INCLUDE_DIR}/middlewares
    ${artosyn_INCLUDE_DIR}/mpp
)