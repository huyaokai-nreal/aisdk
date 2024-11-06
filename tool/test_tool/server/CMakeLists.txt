add_executable(grpc_server_android
grpc_server.cpp
hand_tracking.cpp
)
target_link_libraries(grpc_server_android grpc::grpc JsonCpp::JsonCpp opencv::opencv Eigen3::Eigen base tool_util tool_proto base)
target_compile_options(grpc_server_android PRIVATE -Wno-error -DNRPLUGIN)
add_clang_format_target(grpc_server_android)
set_target_properties(grpc_server_android PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
target_include_directories(grpc_server_android PRIVATE ${PROJECT_SOURCE_DIR}/interface/handtracking_sdk/channel)
target_include_directories(grpc_server_android PRIVATE ${PROJECT_SOURCE_DIR}/interface/handtracking_sdk/common)
target_include_directories(grpc_server_android PRIVATE ${PROJECT_SOURCE_DIR}/interface/handtracking_sdk/perception)
target_include_directories(grpc_server_android PRIVATE ${PROJECT_SOURCE_DIR}/interface/handtracking_sdk/public)