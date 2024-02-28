set(XENHINE_LIBRARY_NAME xengine)
add_library(${XENHINE_LIBRARY_NAME} STATIC
${CMAKE_CURRENT_SOURCE_DIR}/nrhal_capi_symbol.cpp
${CMAKE_CURRENT_SOURCE_DIR}/nrhal_backend.cpp
${CMAKE_CURRENT_SOURCE_DIR}/nrhal_common.cpp
${CMAKE_CURRENT_SOURCE_DIR}/nrhal_net.cpp
${CMAKE_CURRENT_SOURCE_DIR}/nrnn_model.cpp
${CMAKE_CURRENT_SOURCE_DIR}/nrnn_session.cpp
)
target_link_libraries(${XENHINE_LIBRARY_NAME} PUBLIC base jsoncpp openssl::openssl)

add_subdirectory(model)
add_subdirectory(backend)
add_subdirectory(nn)

install(TARGETS ${XENHINE_LIBRARY_NAME}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
add_clang_format_target(${XENHINE_LIBRARY_NAME})