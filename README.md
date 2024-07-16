# AISDK
AISDK提供为 AI组对外提供算法支持的统一算法 SDK， 包括 hand tracking, keyboard tracking和 marker tracking等功能

## 架构
https://xreal.feishu.cn/wiki/HSBwwBvW7i160FktSWKc463znxf

##  工程结构

```
conanfile.py # conan依赖与工程构建配置文件
config # 配置文件目录
cmake # cmake 函数目录
aisdk  # sdk主要源码目录
  |-- base # aisdk内部基础概念实现，如 timer, logger, tensor等
  |-- algorithm # 算法实现目录，包括 calculator及函数
  |-- task # 各个算法 pipeline 实现
  |-- xengine # 对模型推理的统一实现接口， 自定义加速实现
        |-- hal #不同后端实现
example # 示例
interface # 对外接口
tool # 各种工具，如 profiler  
tests # 单元测试
```

## python

### 编译
```bash
# 1.install pybind11
pip install "pybind11[global]"
# 2. enable python build
# set PYTHON_VERSION in python/aisdk/CMakeLists.txt
# set BUILD_PYTHON in CMakeLists.txt
# 3. build dynamic lib
./compile.sh  -c linux install
# 4. install  python package 
pip install -v .
# 5. run example
python python/example.py
```

### 从 github安装
目前支持python3.11

```bash
pip install git+ssh://git@github.com/nreal-alg-ai/aisdk.git@python
```

## 开发测试流程
https://xreal.feishu.cn/wiki/S2dSwvleziXIRYkYDIkc4as8nEf