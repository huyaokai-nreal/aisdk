# 打开xgraph的profiler 必须设置export ENABLE_XGRAPH_PROFILER=ON
export ENABLE_XGRAPH_PROFILER=ON
# export CMAKE_COMMAND_COMMON="-DENABLE_XGRAPH_PROFILER=ON"
export CMAKE_COMMAND_COMMON="-DENABLE_XGRAPH_PROFILER=ON -DENABLE_LOG_LEVEL_RELEASE=OFF -DENABLE_LOG_LEVEL_ALL=ON -DENABLE_ALGORITHM_DATA_RECORD=ON -DENABLE_ALGORITHM_GRAPH_STREAM_EVAL_TIME=ON"
./compile.sh -c android64

adb_device='10.2.18.72:5555'
# adb -s ${adb_device} push build_android/Release/example/handtracking_sdk/handtracking_demo /data/local/tmp/aisdk_android+linux_demotest
# adb -s ${adb_device} push build_android/Release/lib/libnr_hand_tracking.so /data/local/tmp/aisdk_android+linux_demotest/handTracking

# adb -s ${adb_device} push build_android/Release/lib/libnr_hand_tracking.so /system/lib64

# adb -s ${adb_device} push config/sdk.json /sdcard/Android/data/com.xreal.evapro.nebula/files
# adb -s ${adb_device} push build_android/Release/lib/libnr_hand_tracking.so /sdcard/Android/data/com.xreal.evapro.nebula/files

# adb -s ${adb_device} push config/sdk.json /sdcard/Android/data/com.xreal.HandInteractionExamples_NRSDK/files
# adb -s ${adb_device} push build_android/Release/lib/libnr_hand_tracking.so /sdcard/Android/data/com.xreal.HandInteractionExamples_NRSDK/files


