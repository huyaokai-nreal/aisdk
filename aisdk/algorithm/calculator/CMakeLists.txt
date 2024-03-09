
target_sources(${ALGORITHM_LIBRARY_NAME} PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}/hand_detection_calculator.cc
${CMAKE_CURRENT_SOURCE_DIR}/hand_landmark_calculator.cc
${CMAKE_CURRENT_SOURCE_DIR}/hand_state_calculator.cc
${CMAKE_CURRENT_SOURCE_DIR}/block_hard_rules_calculator.cc
${CMAKE_CURRENT_SOURCE_DIR}/check_headpose_calculator.cc
${CMAKE_CURRENT_SOURCE_DIR}/convert_to_world_calculator.cc
${CMAKE_CURRENT_SOURCE_DIR}/hand_detection_track_calculator.cc
${CMAKE_CURRENT_SOURCE_DIR}/compute_3dscore_calculator.cc
${CMAKE_CURRENT_SOURCE_DIR}/detect_box_smoothing_calculator.cc
${CMAKE_CURRENT_SOURCE_DIR}/kalman_filter_correction_calculator.cc
${CMAKE_CURRENT_SOURCE_DIR}/landmark_filter_calculator.cc
${CMAKE_CURRENT_SOURCE_DIR}/standardize_keypoints_calculator.cc
${CMAKE_CURRENT_SOURCE_DIR}/lift_calculator.cc
${CMAKE_CURRENT_SOURCE_DIR}/gesture_recognition_calculator.cc
${CMAKE_CURRENT_SOURCE_DIR}/pre_constraint_calculator.cc
${CMAKE_CURRENT_SOURCE_DIR}/post_constraint_calculator.cc
)
