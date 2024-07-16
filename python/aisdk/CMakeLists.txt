set(PYTHON_VERSION 3.11)
set(Python_EXECUTABLE "/usr/bin/python${PYTHON_VERSION}")
find_package(Python ${PYTHON_VERSION} COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 REQUIRED)
include_directories(${camera_model_INCLUDE_DIRS})
set(target_name _pyaisdk)
pybind11_add_module(${target_name} aisdk.cpp)
target_link_libraries(${target_name} PRIVATE base algorithm Eigen3::Eigen opencv::opencv
)
target_compile_options(${target_name}
    PRIVATE -Wno-error
)
add_clang_format_target(${target_name})
set_target_properties(${target_name} PROPERTIES  
LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})