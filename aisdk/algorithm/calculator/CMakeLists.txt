
set(BUILD_PROTO_FILES ON)
message(STATUS "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx")
if(BUILD_PROTO_FILES)
    file(GLOB protobuf_files ./*.proto)
    SET(PROTO_META_BASE_DIR ${CMAKE_CURRENT_BINARY_DIR})
    LIST(APPEND PROTO_FLAGS -I${CMAKE_CURRENT_SOURCE_DIR})
    FOREACH(FIL ${protobuf_files})
    message(STATUS ${FIL})

    GET_FILENAME_COMPONENT(FIL_WE ${FIL} NAME_WE)

        string(REGEX REPLACE ".+/(.+)\\..*" "\\1" FILE_NAME ${FIL})
        string(REGEX REPLACE "(.+)\\${FILE_NAME}.*" "\\1" FILE_PATH ${FIL})
        EXECUTE_PROCESS(
                COMMAND ${PROTOBUF_PROTOC_EXECUTABLE} ${PROTO_FLAGS} --cpp_out=${CMAKE_CURRENT_SOURCE_DIR} ${FIL}
        )
        set_source_files_properties(${FILE_PATH}${FIL_WE}.pb.cc PROPERTIES GENERATED True)
        set_source_files_properties(${FILE_PATH}${FIL_WE}.pb.h PROPERTIES GENERATED True)

    ENDFOREACH()
endif()
target_sources(${ALGORITHM_LIBRARY_NAME} PRIVATE
${CMAKE_CURRENT_SOURCE_DIR}/xgraph_service_utils.cpp
${CMAKE_CURRENT_SOURCE_DIR}/hand_detection_track_calculator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/hand_detection_track_calculator.pb.cc
${CMAKE_CURRENT_SOURCE_DIR}/hand_landmark_calculator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/hand_landmark_calculator.pb.cc
${CMAKE_CURRENT_SOURCE_DIR}/block_hard_rules_calculator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/block_hard_rules_calculator.pb.cc
${CMAKE_CURRENT_SOURCE_DIR}/convert_to_world_calculator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/detect_box_smoothing_calculator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/hand_filter_calculator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/hand_filter_calculator.pb.cc
${CMAKE_CURRENT_SOURCE_DIR}/landmark_filter_calculator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/standardize_keypoints_calculator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/mono_bino_switch_calculator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/mono_bino_switch_calculator.pb.cc
${CMAKE_CURRENT_SOURCE_DIR}/hand_lift_calculator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/hand_lift_calculator.pb.cc
${CMAKE_CURRENT_SOURCE_DIR}/gesture_recognition_calculator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/hand_data_record_calculator.cpp
${CMAKE_CURRENT_SOURCE_DIR}/mono_hand_kpt3d_calculator.cpp

)
