project(mano)

set(CNPY_DIR ${CMAKE_CURRENT_LIST_DIR}/numpyLib)

SET(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -O3 -Wall -fPIC")

if(CMAKE_SYSTEM_NAME MATCHES "^Linux")
    add_compile_definitions(LINUX)
endif()

# include_directories(${CMAKE_CURRENT_LIST_DIR})
# include_directories("numpyLib")
# include_directories("zlib")


configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/models.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/models.h"
)

add_subdirectory(zlib)

# find_package(Eigen3 3.3 REQUIRED)
# include_directories(${EIGEN3_INCLUDE_DIRS})
# message(STATUS "eigen3 directories:" ${EIGEN3_INCLUDE_DIRS})

aux_source_directory(. DIR_MANO_SRCS)
aux_source_directory(${CNPY_DIR} DIR_CNPY_SRCS)
message(STATUS ${DIR_MANO_SRCS})

# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../../include/utils)

if(CMAKE_SYSTEM_NAME MATCHES "^Android")
    # include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../install/tools/android/${ANDROID_ABI}/include)
    # link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../install/tools/android/${ANDROID_ABI}/lib)
else()
    if(NOT DEFINED COMPILE_TARGET)
        set(COMPILE_TARGET linux)
    endif()

    # include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../install/tools/${COMPILE_TARGET}/include)
    # link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../install/tools/${COMPILE_TARGET}/lib)

    # include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../install/depends/${COMPILE_TARGET}/openssl/include)
    # link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../install/depends/${COMPILE_TARGET}/openssl/lib)
endif()

add_library(${PROJECT_NAME} STATIC ${DIR_MANO_SRCS} ${DIR_CNPY_SRCS})

target_include_directories(
    ${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/numpyLib ${CMAKE_CURRENT_SOURCE_DIR}/zlib
)

target_link_libraries(${PROJECT_NAME}
    zlib
    Eigen3::Eigen
)
