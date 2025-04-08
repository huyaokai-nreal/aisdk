# 添加可执行文件
add_executable(handtracking_test test_interface.cpp)

# 链接主库和依赖
target_link_libraries(handtracking_test PRIVATE
    ${HANDTRACKING_INTERFACE_LIBRARY_NAME}
    # 其他测试专用依赖（如gtest）
)

# 设置测试可执行文件输出目录
set_target_properties(handtracking_test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# 如需添加测试框架（示例）
# enable_testing()
# add_test(NAME handtracking_test COMMAND handtracking_test)