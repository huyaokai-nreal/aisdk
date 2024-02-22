message ("ENABLE_XENGINE_SNPE_NN: ${ENABLE_XENGINE_SNPE_NN}")
if(ENABLE_XENGINE_SNPE_NN)
    set(HAVE_HAL_SNPE 1)
    add_compile_definitions(HAVE_HAL_SNPE)
    if(SNPE_VERSION STREQUAL "snpe-1.55.0")
        add_compile_definitions(SNPE_VERSION=1550)
    endif()
    if(SNPE_VERSION STREQUAL "snpe-1.61.0")
        add_compile_definitions(SNPE_VERSION=1550)
    endif()
    if(SNPE_VERSION STREQUAL "snpe-1.66.0")
        add_compile_definitions(SNPE_VERSION=1660)
    endif()
    if(SNPE_VERSION STREQUAL "snpe-2.17.0")
        add_compile_definitions(SNPE_VERSION=2170)
    endif()
    set(pattern_snpe2 "snpe-2\.[0-9]+\.[0-9]+")
    string(REGEX MATCH "${pattern_snpe2}" match_snpe2 "${SNPE_VERSION}")
    message("match_snpe2=" ${match_snpe2})
endif()

message ("ENABLE_XENGINE_MNN_NN: ${ENABLE_XENGINE_MNN_NN}")
if(ENABLE_XENGINE_MNN_NN)
    add_compile_definitions(HAVE_HAL_MNN)
endif()

file(GLOB XENGINE_SOURCES 
"${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/model/*.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/model/*.c"
"${CMAKE_CURRENT_SOURCE_DIR}/nn/vendor_xrnn/*.cpp"
"${CMAKE_CURRENT_SOURCE_DIR}/backend/cpu/*.cpp"
)

# 选择nn的支持情况
if(ENABLE_XENGINE_SNPE_NN)
    if(match_snpe2)
        file(GLOB SNPE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/nn/vendor_snpe2/*.cpp")
        set(XENGINE_SOURCES ${XENGINE_SOURCES} ${SNPE_SRCS})

    else()
        file(GLOB SNPE_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/nn/vendor_snpe/*.cpp")
        set(XENGINE_SOURCES ${XENGINE_SOURCES} ${SNPE_SRCS})
    endif()
endif()

if(ENABLE_XENGINE_MNN_NN)
    file(GLOB MNN_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/nn/vendor_mnn/*.cpp")
    set(XENGINE_SOURCES ${XENGINE_SOURCES} ${MNN_SRCS})
endif()

set(LIBRARY_NAME nr_hand_algo)
add_library(${LIBRARY_NAME} SHARED ${XENGINE_SOURCES})
target_link_libraries(${LIBRARY_NAME} PUBLIC base jsoncpp openssl::openssl)

if(ENABLE_XENGINE_SNPE_NN)
    if(NOT match_snpe2)
        target_link_libraries(${LIBRARY_NAME} PUBLIC SNPE)
    else()
        target_include_directories(${LIBRARY_NAME} PRIVATE ${snpe_INCLUDE_DIR})
    endif()
endif()

if(ENABLE_XENGINE_MNN_NN)
    target_link_libraries(${LIBRARY_NAME} PUBLIC MNN)
endif()

install(TARGETS ${LIBRARY_NAME}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
add_clang_format_target(${LIBRARY_NAME})