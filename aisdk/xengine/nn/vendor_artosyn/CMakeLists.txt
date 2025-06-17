# 添加源文件
target_sources(${XENGINE_LIBRARY_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/artosyn_impl.cpp
)

# 添加头文件目录
target_include_directories(${XENGINE_LIBRARY_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}/thirdparty/artosyn/inc
)

# 添加链接库目录
link_directories(${CMAKE_SOURCE_DIR}/thirdparty/artosyn/lib)

# 链接所有必需库
set(ARTOSYN_LIBS 
    cjson 
    hal_sys 
    osal 
    utils 
    hal_npu
    hal_dsp
    mpi_npu
    mpi_dsp
    binder_ipc
    fuse3
    rpc_fs
    rpc_proxy
    tools
    cnn_sr
    hal_proc
    hal_efuse
    hal_sysctl
    mpi_sysctl
)

# 链接目标库
foreach(lib ${ARTOSYN_LIBS})
    set(LIB_FILE "${CMAKE_SOURCE_DIR}/thirdparty/artosyn/lib/lib${lib}.so")

    # 检查库文件是否存在
    if(NOT EXISTS ${LIB_FILE})
        message(SEND_ERROR "fatal mistake: Artosyn lib file does not exist - ${LIB_FILE}")
    endif()

    # 使用完整的路径链接库
    target_link_libraries(${XENGINE_LIBRARY_NAME} PRIVATE "${LIB_FILE}")
endforeach()

# 设置运行时库搜索路径（优先级：应用目录 > /lib > /usr/lib > 系统路径）
set(CMAKE_INSTALL_RPATH "$ORIGIN:$ORIGIN/../lib:/lib:/usr/lib")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

# 添加 -Wl,--disable-new-dtags 链接器选项
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags")

# 库文件检查
add_custom_target(verify_artosyn_libs ALL
    COMMAND ${CMAKE_COMMAND} -E echo "VERIFY ARTOSYN LIBS EXIST..."
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/lib_checks"

    # 检查每个库文件
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/lib_checks/cjson" && test -e ${CMAKE_SOURCE_DIR}/thirdparty/artosyn/lib/libcjson.so || (echo "lack of lib: libcjson.so"; exit 1)
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/lib_checks/hal_sys" && test -e ${CMAKE_SOURCE_DIR}/thirdparty/artosyn/lib/libhal_sys.so || (echo "lack of lib: libhal_sys.so"; exit 1)
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/lib_checks/osal" && test -e ${CMAKE_SOURCE_DIR}/thirdparty/artosyn/lib/libosal.so || (echo "lack of lib: libosal.so"; exit 1)
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/lib_checks/utils" && test -e ${CMAKE_SOURCE_DIR}/thirdparty/artosyn/lib/libutils.so || (echo "lack of lib: libutils.so"; exit 1)
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/lib_checks/hal_npu" && test -e ${CMAKE_SOURCE_DIR}/thirdparty/artosyn/lib/libhal_npu.so || (echo "lack of lib: libhal_npu.so"; exit 1)
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/lib_checks/hal_dsp" && test -e ${CMAKE_SOURCE_DIR}/thirdparty/artosyn/lib/libhal_dsp.so || (echo "lack of lib: libhal_dsp.so"; exit 1)
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/lib_checks/mpi_npu" && test -e ${CMAKE_SOURCE_DIR}/thirdparty/artosyn/lib/libmpi_npu.so || (echo "lack of lib: libmpi_npu.so"; exit 1)
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/lib_checks/mpi_dsp" && test -e ${CMAKE_SOURCE_DIR}/thirdparty/artosyn/lib/libmpi_dsp.so || (echo "lack of lib: libmpi_dsp.so"; exit 1)
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/lib_checks/binder_ipc" && test -e ${CMAKE_SOURCE_DIR}/thirdparty/artosyn/lib/libbinder_ipc.so || (echo "lack of lib: libbinder_ipc.so"; exit 1)
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/lib_checks/rpc_fs" && test -e ${CMAKE_SOURCE_DIR}/thirdparty/artosyn/lib/librpc_fs.so || (echo "lack of lib: librpc_fs.so"; exit 1)
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/lib_checks/rpc_proxy" && test -e ${CMAKE_SOURCE_DIR}/thirdparty/artosyn/lib/librpc_proxy.so || (echo "lack of lib: librpc_proxy.so"; exit 1)
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/lib_checks/tools" && test -e ${CMAKE_SOURCE_DIR}/thirdparty/artosyn/lib/libtools.so || (echo "lack of lib: libtools.so"; exit 1)
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/lib_checks/cnn_sr" && test -e ${CMAKE_SOURCE_DIR}/thirdparty/artosyn/lib/libcnn_sr.so || (echo "lack of lib: libcnn_sr.so"; exit 1)
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/lib_checks/hal_proc" && test -e ${CMAKE_SOURCE_DIR}/thirdparty/artosyn/lib/libhal_proc.so || (echo "lack of lib: libhal_proc.so"; exit 1)
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/lib_checks/hal_efuse" && test -e ${CMAKE_SOURCE_DIR}/thirdparty/artosyn/lib/libhal_efuse.so || (echo "lack of lib: libhal_efuse.so"; exit 1)
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/lib_checks/hal_sysctl" && test -e ${CMAKE_SOURCE_DIR}/thirdparty/artosyn/lib/libhal_sysctl.so || (echo "lack of lib: libhal_sysctl.so"; exit 1)
    COMMAND ${CMAKE_COMMAND} -E touch "${CMAKE_CURRENT_BINARY_DIR}/lib_checks/mpi_sysctl" && test -e ${CMAKE_SOURCE_DIR}/thirdparty/artosyn/lib/libmpi_sysctl.so || (echo "lack of lib: libmpi_sysctl.so"; exit 1)

    COMMENT "CHECK ARTOSYN LIB FILES"
    BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/lib_checks
)

# 确保在构建目标前验证库存在
add_dependencies(${XENGINE_LIBRARY_NAME} verify_artosyn_libs)
