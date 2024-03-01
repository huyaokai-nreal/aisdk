target_sources(${ALGORITHM_LIBRARY_NAME}
PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}/permute.cc
${CMAKE_CURRENT_SOURCE_DIR}/pose_solver.cpp
${CMAKE_CURRENT_SOURCE_DIR}/rsntiny_postprocess/rsntiny_postprocess.cc
${CMAKE_CURRENT_SOURCE_DIR}/rsntiny_postprocess/softmax.cc
${CMAKE_CURRENT_SOURCE_DIR}/rsntiny_postprocess/reducesum.cc
${CMAKE_CURRENT_SOURCE_DIR}/rsntiny_postprocess/elementwise_mul.cc
${CMAKE_CURRENT_SOURCE_DIR}/NR_CV.cpp
${CMAKE_CURRENT_SOURCE_DIR}/netalgo_utils.cpp
${CMAKE_CURRENT_SOURCE_DIR}/rsntiny_preprocess/warpaffine.cc
${CMAKE_CURRENT_SOURCE_DIR}/rsntiny_preprocess/generate_bbox.cc
${CMAKE_CURRENT_SOURCE_DIR}/hand_filters.cpp
${CMAKE_CURRENT_SOURCE_DIR}/hand_rotation.cpp



)
