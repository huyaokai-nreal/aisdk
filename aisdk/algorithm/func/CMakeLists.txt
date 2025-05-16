# 将以下源文件添加到算法库目标（${ALGORITHM_LIBRARY_NAME}）的构建中
# PRIVATE 表示这些源文件仅用于构建库本身，不暴露给依赖该库的其他目标
target_sources(
    ${ALGORITHM_LIBRARY_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/permute.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pose_solver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/softmax.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/reducesum.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/elementwise_mul.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/netalgo_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/warpaffine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hand_filters.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hand_nimble.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hand_rotation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/hand_rotation_v2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gesture_recognition_v2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/keypoint3d_solver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/perspective_crop.cpp
)
