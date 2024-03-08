add_library(handtracking
${CMAKE_CURRENT_SOURCE_DIR}/test.cpp
)

target_link_libraries(handtracking PRIVATE base)
install(TARGETS handtracking
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
FILE(GLOB HEADERS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
install(
  FILES 
  ${HEADERS}
  DESTINATION
  ${AISDK_INCLUDE_INSTALL_DIR}/task/handtracking
)
target_compile_options(handtracking PRIVATE -Wno-error)