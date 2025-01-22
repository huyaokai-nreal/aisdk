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
  echo "错误：请使用 'source local_compile.sh' 来执行此脚本。"
  echo "直接运行 './local_compile.sh' 会导致环境变量设置失败。"
  exit 1
fi

# 设置环境变量
export CONAN_USER_REQUIRES="['leopard/jenkins#8c2d2805c596ec2c398625c95cf020bf6b3d83a1','xr_codec/jenkins#5971075b8477b6dc1ce248d441e450bc9d81a961','nr_controller/jenkins#52bf5247abce9e9f01bb4b01b74c710bca90df70','nr_display/jenkins#ef65ad275ab413e4d2c43fdfc1f0580f16bd169b','dispinterrupt/jenkins#0dcdda86db6f9b8bea61c9a75da43f466e4bfb97','ov580_driver/jenkins#0fcf8414ee4e75866a13a3fd2158c9c7f4c60826','nralg-trackingnoninertial/jenkins#77eefcbbb936a4e6d4a9f6f5aed4c269358a30f6','plugin_external_sensor/jenkins#92ba10ed4b22e1953ed3d5137032cd73e0ca2ab7','framework/jenkins#3e93285bc590a626726e2ebd136960e41c4a76b9','aisdk/jenkins#e4d650989a959ea3c403fabb07ea2ea1dbad9e9b','cocoaengine/jenkins#bf5f94efb3f5c86624196846073c8aaebaa76925','nrecon3d-online/jenkins#5f438b52fda04d7a14b2418a3227241e5800922d','perception_sdk_sequoia/jenkins#d3c7145c1d9793483350001a3c96dd4abc3a8771','genthreedof/jenkins#9ed0bd099af41a499f9db3d7bca31622b2341505','nrslam-old/jenkins#69492ebbb6fac87ac5e6d08e7e9e35b198f65f55','chameleon/jenkins#bbb875c2a68587af877bf98b9a6a0d636e12e819','warpcore/jenkins#d1c4b0fbf40bb9e5cadf7f1dae421a10bdeedc9a','sparrow/jenkins#eefb187247158a0570e47071025544b3f752f499','xrspatialanchor/jenkins#d4b242e87ace2458cc5a78ef565883d44487e445','xr_timesync/jenkins#6308ffaffb4a2a4ae012357d4b8ea049262e455b','nrealutil/jenkins#f67ef105712cb57544682e53fec25a163918bdf0']"

# 验证环境变量
echo "CONAN_USER_REQUIRES 已设置为: $CONAN_USER_REQUIRES"

# 执行编译脚本
bash compile.sh -c android64