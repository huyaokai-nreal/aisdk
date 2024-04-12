#!/bin/bash

pp=$(pwd)
# linux x86_64
aes_tools=${pp}/tool/aes_encrypt_tools
tar_path=${pp}/config/ppls
tar_out=${pp}/build/handtracking_pipeline_v2.0.0.tar
# 需要进入目录后打包
cd ${tar_path}
tar -cvf ${tar_out} *_pipeline_config.json *.txt -h models
${aes_tools} ${tar_out} ${tar_out}.enc enc > ${tar_out}.key
rm ${tar_out}