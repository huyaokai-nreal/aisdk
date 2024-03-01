# 查询当前分支 和 commit ID
execute_process(COMMAND "git" rev-parse --abbrev-ref HEAD OUTPUT_VARIABLE GIT_CURRENT_BRANCH ERROR_QUIET)
execute_process(COMMAND "git" log -1 --format=%H OUTPUT_VARIABLE GIT_LAST_COMMIT ERROR_QUIET)
string(STRIP ${GIT_CURRENT_BRANCH} GIT_CURRENT_BRANCH)
string(STRIP ${GIT_LAST_COMMIT} GIT_LAST_COMMIT)
message("Current Branch: ${GIT_CURRENT_BRANCH}")
message("Last Commit: ${GIT_LAST_COMMIT}")

if(BUILD_HANDTRACKING_STD_SDK)
    add_subdirectory(handtracking_sdk)
endif()

if(BUILD_HANDTRACKING_FPTOOL_SDK)
    add_subdirectory(handtracking_fptool)
endif()