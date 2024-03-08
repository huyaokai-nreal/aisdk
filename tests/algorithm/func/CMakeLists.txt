list(APPEND TEST_DEP_LIBS base xengine algorithm opencv::opencv doctest::doctest)
ADD_TEST_CASE(${CMAKE_CURRENT_SOURCE_DIR}/test_get_roi_image.cpp "" TEST_DEP_LIBS)
ADD_TEST_CASE(${CMAKE_CURRENT_SOURCE_DIR}/test_pose_solver.cpp "" TEST_DEP_LIBS)