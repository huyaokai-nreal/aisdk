list(APPEND TEST_DEP_LIBS base doctest::doctest framework::framework)
ADD_TEST_CASE(${CMAKE_CURRENT_SOURCE_DIR}/test_camera_model.cpp "" TEST_DEP_LIBS)
ADD_TEST_CASE(${CMAKE_CURRENT_SOURCE_DIR}/test_log.cpp "" TEST_DEP_LIBS)
ADD_TEST_CASE(${CMAKE_CURRENT_SOURCE_DIR}/test_file.cpp "" TEST_DEP_LIBS)