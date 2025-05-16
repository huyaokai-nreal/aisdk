#!/bin/bash
set -e  # 任何命令失败，立即终止脚本

output_dir="${1:-"build"}"
tar_ppls=${2}
tar_models=${3}

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)  # 获取脚本所在目录
model_src_path="${script_dir}/thirdparty/ai_model_zoo/${tar_models}"
tar_path="${script_dir}/config/${tar_ppls}"
tar_out="${output_dir}/handtracking_pipeline_v2.0.0.tar"
aes_tools="${script_dir}/tool/aes_encrypt_tools"

# 检查必要目录存在
[ ! -d "${model_src_path}" ] && echo "模型目录不存在: ${model_src_path}" && exit 1
[ ! -d "${tar_path}" ] && echo "配置目录不存在: ${tar_path}" && exit 1

# 复制模型文件（保留原目录结构）
mkdir -p "${tar_path}/models"
cp -r "${model_src_path}/"* "${tar_path}/models/" || exit 1

# 打包
tar -cvf "${tar_out}" -C "${tar_path}" *.json *.txt models || exit 1

# 加密（检查工具是否存在）
[ ! -x "${aes_tools}" ] && echo "加密工具不可执行: ${aes_tools}" && exit 1
"${aes_tools}" "${tar_out}" "${tar_out}.enc" enc > "${tar_out}.key" || exit 1

# 清理中间文件
rm -f "${tar_out}"
rm -rf "${tar_path}/models"

echo "成功生成加密包: ${tar_out}.enc"