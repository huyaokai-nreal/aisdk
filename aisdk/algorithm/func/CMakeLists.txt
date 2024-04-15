target_sources(${ALGORITHM_LIBRARY_NAME}
PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}/permute.cpp
${CMAKE_CURRENT_SOURCE_DIR}/pose_solver.cpp
${CMAKE_CURRENT_SOURCE_DIR}/softmax.cpp
${CMAKE_CURRENT_SOURCE_DIR}/reducesum.cpp
${CMAKE_CURRENT_SOURCE_DIR}/elementwise_mul.cpp
${CMAKE_CURRENT_SOURCE_DIR}/netalgo_utils.cpp
${CMAKE_CURRENT_SOURCE_DIR}/warpaffine.cpp
${CMAKE_CURRENT_SOURCE_DIR}/generate_bbox.cpp
${CMAKE_CURRENT_SOURCE_DIR}/hand_filters.cpp
${CMAKE_CURRENT_SOURCE_DIR}/hand_rotation.cpp
${CMAKE_CURRENT_SOURCE_DIR}/gesture_recognition_v2.cpp

)
