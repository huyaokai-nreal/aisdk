add_library(
    base 
    STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/camera_model_distortion.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/camera_model_projection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/camera_model.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/file.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/time.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/profiling.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/set_cpu_affinity.cpp
)

target_link_libraries(base PUBLIC Eigen3::Eigen camera_model::camera_model framework::framework absl::time)
target_compile_options(base PRIVATE -Wno-error)
add_clang_format_target(base)