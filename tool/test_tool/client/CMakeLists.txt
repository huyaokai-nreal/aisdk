add_executable(grpc_client_x86 
main.cpp
grpc_client.cpp
load_dataset.cpp 
picture_processer.cpp 
report_result.cpp 
test_config.cpp
#video_processer.cpp
)
target_link_libraries(grpc_client_x86 grpc::grpc JsonCpp::JsonCpp opencv::opencv protobuf::protobuf tool_util base fmt::fmt tool_proto)
target_compile_options(grpc_client_x86 PRIVATE -Wno-error)
add_clang_format_target(grpc_client_x86)
set_target_properties(grpc_client_x86 PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)