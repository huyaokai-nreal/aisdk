#!/bin/bash

# ========================================================
# 脚本名称: local_compile.sh
# 描述: 该脚本用于aisdk代码本地自动化编译过程（jenkins不执行这个脚本），解决以下问题：
#       1. aisdk代码执行需要依赖framework中各种相关库配置，conanfile里的framework不会自动更新，
#       但编译过程中，一些其他库会自动拉取最新版，这会导致基础库版本冲突，所以需要在执行compile.sh之前
#       设置环境变量CONAN_USER_REQUIRES，保证各种库版本一致，都统一为最新版。
#       2. 编译命令较长，手动设置环境变量（如 CONAN_USER_REQUIRES）容易出错或遗漏。
#       3. 提供统一的编译入口，方便团队协作。
# 使用方法:
#       1. 使用 source 命令运行脚本，以确保环境变量生效：
#          source local_compile.sh（注意：直接运行./local_compile.sh会导致环境变量设置失效，因此不被允许）
# TODO:
#       1. 目前版本，如果framewrok有更新，需要手动去jenkins上最新的android构建（示例地址：
#       https://jenkins-nrsdk.xreal.work/blue/organizations/jenkins/android/detail/android/722/pipeline/305）
#       获取环境变量CONAN_USER_REQUIRES的值，并更新脚本中的CONAN_USER_REQUIRES，较为繁琐，期望后续改成自动获取。
#
# 作者: shuaiwang@xreal.com
# 版本: 1.0.0
# 创建日期: 2024-01-22
# ========================================================

# 检查脚本是否是通过 source 命令执行的
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
  echo "错误：请使用 'source local_compile.sh' 来执行此脚本android环境下的编译生成。"
  echo "错误：请使用 'source local_compile.sh linux' 来执行此脚本linux环境下的编译生成。"
  echo "错误：请使用 'source local_compile.sh Xrlinux' 来执行此脚本Xrlinux环境下的编译生成。"
  echo "直接运行 './local_compile.sh' 会导致环境变量设置失败。"
  exit 1
fi

# 设置环境变量
export CONAN_USER_REQUIRES="['leopard/jenkins#48f02138175896e58f826b83cfb341666948c753','xr_codec/jenkins#b06b81bb2a5bd7f404cb183614a857dbca7c8435','nr_controller/jenkins#29822a327e1287a5ee9da3ee68bb19708228e88c','nr_display/jenkins#671c1bb0d4327f786f31005b387e99b3015d8c88','dispinterrupt/jenkins#0174f8b0679a67eaa3250cf9ea8d53d9b71df288','ov580_driver/jenkins#dcc9de17eeb72ec5c2e88a62616dc86138df69af','nralg-trackingnoninertial/jenkins#b5da6ad7bb3a50e20041783975d35260babf534a','plugin_external_sensor/jenkins#d56093d8c4fb1e329ed291a36b453433fd48e1fc','framework/jenkins#f4b20f7e2afa58e366fd421604e5d44f7e2fb7f4','aisdk/jenkins#d916015c29f52f72144db6b62e4829347a4e7faf','cocoaengine/jenkins#271311c4bdd1ae7354197c22d2024c18f3486e51','nrecon3d-online/jenkins#5f438b52fda04d7a14b2418a3227241e5800922d','perception_sdk_sequoia/jenkins#11e56e3cda9a5449c32038bd5695669cfb5cbb14','genthreedof/jenkins#78d03fe8c5f687f4343b2cc6232346e4c5eb94ff','nrslam-old/jenkins#69492ebbb6fac87ac5e6d08e7e9e35b198f65f55','chameleon/jenkins#ba1c40ffc2ceacac764ca5b469b48fbaea1d761f','warpcore/jenkins#d1c4b0fbf40bb9e5cadf7f1dae421a10bdeedc9a','sparrow/jenkins#9e7e135cc7d8a106f946e7adec6d793fd1c456b0','xrspatialanchor/jenkins#7f2c5e6395274beda72b2841f2aafcd2f79028a4','xr_timesync/jenkins#4d70e474faa0772a94993b979f02dc85d3db4198','nrealutil/jenkins#44bf9e7a2c66fdce0be78b3e8a3bb2552ff5adc1']"

# 验证环境变量
echo "CONAN_USER_REQUIRES 已设置为: $CONAN_USER_REQUIRES"

# 执行编译脚本
if [[ "$1" == "linux" ]]; then
    bash compile.sh -c linux
elif [[ "$1" == "xrlinux" ]]; then
    bash compile.sh -c xrlinux
else
    bash compile.sh -c android64
fi