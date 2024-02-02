message(STATUS "EANABLE BUILD TESTS")
find_package(doctest REQUIRED)
include(${PROJECT_SOURCE_DIR}/cmake/test_macro.cmake)
add_subdirectory(base)