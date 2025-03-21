include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/interface/handtracking_sdk)
find_package(gRPC REQUIRED)
add_subdirectory(util)
add_subdirectory(proto)
add_subdirectory(client)
add_subdirectory(server)

