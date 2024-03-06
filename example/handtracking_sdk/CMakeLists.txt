set(HANDTRACKING_SDK_DEMO_NAME handtracking_demo)

add_executable(${HANDTRACKING_SDK_DEMO_NAME}
${CMAKE_CURRENT_SOURCE_DIR}/demo_main.cpp
${CMAKE_CURRENT_SOURCE_DIR}/demo_deploy_handTracking.cpp
)

# add target include. not fix exteral xxx.h 
target_include_directories(${HANDTRACKING_SDK_DEMO_NAME} PRIVATE "../../interface/handtracking_sdk")

target_compile_options(${HANDTRACKING_SDK_DEMO_NAME} PRIVATE -DNRPLUGIN)

target_link_libraries(${HANDTRACKING_SDK_DEMO_NAME} PUBLIC base jsoncpp Eigen3::Eigen opencv::opencv)

install(TARGETS ${HANDTRACKING_SDK_DEMO_NAME}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
add_clang_format_target(${HANDTRACKING_SDK_DEMO_NAME})