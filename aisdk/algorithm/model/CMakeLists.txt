# 将以下源文件添加到算法库目标（${ALGORITHM_LIBRARY_NAME}）的构建中
# PRIVATE 表示这些源文件仅用于构建库本身，不暴露给依赖该库的其他目标
target_sources(
    ${ALGORITHM_LIBRARY_NAME} 
    PRIVATE

    # 手部检测逻辑实现文件，android平台和npu平台
    ${CMAKE_CURRENT_SOURCE_DIR}/hand_detect.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hand_detect_artosyn.cpp

    # 2d点检测逻辑实现文件，android平台和npu平台
    ${CMAKE_CURRENT_SOURCE_DIR}/hand_rsntiny.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hand_rsnnano.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hand_rtmtiny.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hand_rtmtiny_artosyn.cpp

    # 关键点滤波处理逻辑实现文件
    ${CMAKE_CURRENT_SOURCE_DIR}/landmark_filter.cpp

    # 手部姿态提升逻辑实现文件
    ${CMAKE_CURRENT_SOURCE_DIR}/hand_lift.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hand_lift_nimble.cpp
)