#!/bin/bash
output_dir="${1:-"build"}"
pp=$(pwd)
tar_ppls=${2}
tar_models=${3}
aes_tools=${pp}/tool/aes_encrypt_tools
tar_path=${pp}/config/${tar_ppls}
tar_out=${output_dir}/handtracking_pipeline_v2.0.0.tar
model_src_path=${pp}/thirdparty/ai_model_zoo/${tar_models}
# 复制模型文件
cp -r ${model_src_path} ${tar_path}/models
# 需要进入目录后打包
cd ${tar_path}
tar -cvf ${tar_out} *.json *.txt models
${aes_tools} ${tar_out} ${tar_out}.enc enc > ${tar_out}.key
# 删除中间文件
rm ${tar_out}
rm -r ${tar_path}/models