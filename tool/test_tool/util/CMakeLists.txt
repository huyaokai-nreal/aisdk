add_library(tool_util STATIC
lmdb/mdb.c
lmdb/midl.c
)
target_compile_options(tool_util PRIVATE -Wno-error)
add_clang_format_target(tool_util)