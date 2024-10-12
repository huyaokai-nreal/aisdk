set(XENGINE_LIBRARY_NAME xengine)
add_library(${XENGINE_LIBRARY_NAME} STATIC
${CMAKE_CURRENT_SOURCE_DIR}/nrhal_capi_symbol.cpp
${CMAKE_CURRENT_SOURCE_DIR}/nrhal_backend.cpp
${CMAKE_CURRENT_SOURCE_DIR}/nrhal_common.cpp
${CMAKE_CURRENT_SOURCE_DIR}/nrhal_net.cpp
${CMAKE_CURRENT_SOURCE_DIR}/nrnn_model.cpp
${CMAKE_CURRENT_SOURCE_DIR}/nrnn_session.cpp
)
target_link_libraries(${XENGINE_LIBRARY_NAME} PUBLIC base jsoncpp openssl::openssl opencv::opencv abseil::abseil)
target_compile_options(${XENGINE_LIBRARY_NAME}
    PRIVATE -Wno-error
)
add_subdirectory(cv)
add_subdirectory(util)
add_subdirectory(backend)
add_subdirectory(nn)

install(TARGETS ${XENGINE_LIBRARY_NAME}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
add_clang_format_target(${XENGINE_LIBRARY_NAME})