add_library(
    base 
    STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/camera_model_distortion.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/camera_model_projection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/camera_model.cpp
)

target_link_libraries(base PUBLIC Eigen3::Eigen camera_model::camera_model)
add_clang_format_target(base)