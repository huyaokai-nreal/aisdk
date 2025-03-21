
add_library(tool_proto STATIC
nreal.ai.tool.pb.cc
nreal.ai.tool.grpc.pb.cc
)
target_link_libraries(tool_proto grpc::grpc protobuf::protobuf)
target_compile_options(tool_proto PRIVATE -Wno-error)
add_clang_format_target(tool_proto)