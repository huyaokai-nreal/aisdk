list(APPEND TEST_DEP_LIBS base xengine opencv::opencv doctest::doctest  algorithm)
list(APPEND TEST_DEP_SRCS ${PROJECT_SOURCE_DIR}/aisdk/algorithm/calculator/block_hard_rules_calculator.cc)
ADD_TEST_CASE(${CMAKE_CURRENT_SOURCE_DIR}/test_block_hard_rules_calculator.cpp TEST_DEP_SRCS TEST_DEP_LIBS)