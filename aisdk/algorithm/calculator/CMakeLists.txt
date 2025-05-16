# 设置 Protobuf 文件编译开关（可通过外部设置覆盖）
set(BUILD_PROTO_FILES ON)
message(STATUS "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")

# Protobuf 文件处理模块
if(BUILD_PROTO_FILES)
    # 获取当前目录下所有 .proto 文件
    file(GLOB protobuf_files ./*.proto)

    # 设置生成的 .pb 文件输出目录为当前构建目录
    SET(PROTO_META_BASE_DIR ${CMAKE_CURRENT_BINARY_DIR})

    # 添加 Protobuf 编译参数：指定源文件搜索路径
    LIST(APPEND PROTO_FLAGS -I${CMAKE_CURRENT_SOURCE_DIR})

    # 遍历所有找到的 .proto 文件
    FOREACH(FIL ${protobuf_files})
        message(STATUS "正在处理Protobuf文件: " ${FIL})

        # 提取文件名（不带扩展名）用于生成输出文件
        GET_FILENAME_COMPONENT(FIL_WE ${FIL} NAME_WE)

        # 通过正则表达式提取基础文件名和路径
        string(REGEX REPLACE ".+/(.+)\\..*" "\\1" FILE_NAME ${FIL})
        string(REGEX REPLACE "(.+)\\${FILE_NAME}.*" "\\1" FILE_PATH ${FIL})

        # 执行protoc编译器生成 C++ 代码
        EXECUTE_PROCESS(
            COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} 
            ${PROTO_FLAGS} 
            --cpp_out=${CMAKE_CURRENT_SOURCE_DIR}  # 指定生成文件输出到源码目录
            ${FIL}
        )

        # 标记生成的 .pb.cc/.pb.h 为生成文件（避免编译系统找不到文件）
        set_source_files_properties(${FILE_PATH}${FIL_WE}.pb.cc PROPERTIES GENERATED True)
        set_source_files_properties(${FILE_PATH}${FIL_WE}.pb.h PROPERTIES GENERATED True)
    ENDFOREACH()
endif()

# 将以下源文件添加到算法库目标（${ALGORITHM_LIBRARY_NAME}）的构建中
# PRIVATE 表示这些源文件仅用于构建库本身，不暴露给依赖该库的其他目标
target_sources(
    ${ALGORITHM_LIBRARY_NAME} 
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/xgraph_service_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hand_detection_track_calculator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hand_detection_track_calculator.pb.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/hand_landmark_calculator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hand_landmark_batch_calculator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hand_landmark_calculator.pb.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/block_hard_rules_calculator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/block_hard_rules_calculator.pb.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/convert_to_world_calculator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/detect_box_smoothing_calculator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hand_filter_calculator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hand_filter_calculator.pb.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/landmark_filter_calculator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/standardize_keypoints_calculator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mono_bino_switch_calculator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mono_bino_switch_calculator.pb.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/hand_lift_calculator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hand_lift_calculator.pb.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/gesture_recognition_calculator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gesture_recognition_calculator.pb.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/hand_data_record_calculator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mono_hand_kpt3d_calculator.cpp
)
