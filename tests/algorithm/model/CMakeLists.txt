list(APPEND TEST_DEP_LIBS base xengine algorithm opencv::opencv doctest::doctest)
ADD_TEST_CASE(${CMAKE_CURRENT_SOURCE_DIR}/test_hand_detect.cpp "" TEST_DEP_LIBS)