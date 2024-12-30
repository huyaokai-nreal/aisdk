
if(CMAKE_SYSTEM_NAME STREQUAL "Android")
    target_sources(${XENGINE_LIBRARY_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/xr_cv.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/warpaffine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/perspective_crop_image.cpp
    )

    target_compile_options(${XENGINE_LIBRARY_NAME} PRIVATE -march=armv8.2-a+fp16+dotprod)

    if(ENABLE_HEXAGON_DSP_OP)
        add_subdirectory(hexagon_dsp)
    endif()
endif()