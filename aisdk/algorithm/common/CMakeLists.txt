# 将以下源文件添加到算法库目标（${ALGORITHM_LIBRARY_NAME}）的构建中
# PRIVATE 表示这些源文件仅用于构建库本身，不暴露给依赖该库的其他目标
target_sources(
    ${ALGORITHM_LIBRARY_NAME} 
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/ofxOneEuroFilter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NR_Transfer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NR_Predictor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NR_Predictor2d.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/metrics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/math.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/NR_Seq_Manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/data_debug_record.cpp
)