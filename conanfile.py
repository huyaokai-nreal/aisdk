from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps
import os

# 定义项目配置类，继承自ConanFile
class AISDK(ConanFile):
    # 使用项目基础配置（来自共享的python_requires）
    python_requires = "project_base/1.0"
    python_requires_extend = "project_base.ProjectBase"
        
    def init(self):
        """初始化方法，继承基础配置"""
        # 获取基础配置类
        base = self.python_requires["project_base"].module.ProjectBase

        # 继承基础配置的设置、选项和版本模式
        self.settings = base.settings
        self.options.update(base.options, base.default_options)
        self.revision_mode = base.revision_mode


    def build_requirements(self):
        """定义构建依赖"""
        super().build_requirements()

        # 根据环境变量添加工具链依赖
        if os.getenv('BUILD_TOOL'):
            self.tool_requires("grpc/1.54.3")
            self.tool_requires("protobuf/3.21.9")
        
        # Android平台需要NDK
        if self.settings.os == "Android": 
            self.tool_requires("android-ndk/r25c")
        
        # 添加测试框架依赖
        self.test_requires("doctest/2.4.11")
    

    def build(self):
        """自定义构建步骤"""
        # 更新git子模块（用于获取模型文件等子模块内容）
        update_model_cmd = "git submodule init && git submodule update"
        os.system(update_model_cmd)
        super().build()
        

    def generate(self):
        """生成构建系统文件(CMake配置)"""
        # 创建CMake工具链实例
        tc = CMakeToolchain(self)

        # 根据环境变量设置CMake变量
        if os.getenv('ENABLE_XGRAPH_PROFILER') == 'ON':
            tc.variables["ENABLE_XGRAPH_PROFILER"] = True  # 启用性能分析
        if os.getenv('BUILD_TOOL') == 'ON':
            tc.variables["BUILD_TOOL"] = True  # 启用工具构建
        
        # 生成toolchain文件
        tc.generate()

        # 生成CMake依赖文件
        deps = CMakeDeps(self)
        deps.generate()


    def requirements(self):
        """定义项目依赖"""
        # 条件依赖：当BUILD_TOOL存在时添加gRPC运行时依赖
        if os.getenv('BUILD_TOOL'):
            self.requires("grpc/1.54.3")
        
        # 基础依赖库
        self.requires("fmt/9.1.0", transitive_headers=True, transitive_libs=True)
        self.requires("opencv/4.5.5", transitive_headers=True, transitive_libs=True)
        self.requires("eigen/3.3.7", transitive_headers=True, transitive_libs=True)
        self.requires("jsoncpp/1.9.5", transitive_headers=True, transitive_libs=True)
        self.requires("openssl/3.2.2", transitive_headers=True, transitive_libs=True)
        self.requires("libyuv/stable", transitive_libs=True)
        self.requires("nreal_mnn/2.0.0", transitive_headers=True, transitive_libs=True)

        # 平台特定依赖
        if self.settings.os in ["Linux", "Android"] and self.conf.get("user.os:distro") != "Xrlinux":
            self.requires("snpe/2.17.0", transitive_headers=False, transitive_libs=False)

        self.requires("camera_model/develop", transitive_libs=True)

        # 框架依赖（根据平台选择不同版本）
        if self.settings.os == "Linux" and self.conf.get("user.os:distro") != "Xrlinux":
            self.requires("framework/jenkins#835223d03ea5fdca60b7c57a1a759936897707f0")
        else:
            #self.requires(super().override_require("framework/jenkins"), run=True)
            self.requires(super().override_require("framework/jenkins#f86342896a5a5513b153e90ae1991e704dbd3070"), run=True)
        
        # XGraph配置（根据性能分析开关选择版本）
        xgraph_version = "xgraph/main"
        enable_xgraph_profiler = False
        if os.getenv('ENABLE_XGRAPH_PROFILER') == 'ON':
            print("Enable xgraph profiler !!!!!!!!")
            enable_xgraph_profiler = True
            xgraph_version = "xgraph/0.10.0.profiler"
        
        # 其他依赖
        self.requires(xgraph_version, transitive_libs=True, options={"enable_profiler": enable_xgraph_profiler})
        self.requires("abseil/20230125.3", transitive_libs=True)
        self.requires("protobuf/3.21.9", transitive_libs=True)
        self.requires("glog/0.6.0", transitive_libs=True)
        self.requires("suitesparse/5.7.1")  #ceres-slover依赖suitesparse/5.7.1
        self.requires("ceres-solver/2.0.0.1")

        # 线性代数库选择
        if self.settings.os in ["Linux", "Android", "Windows"] and self.conf.get("user.os:distro") != "Xrlinux":
            self.requires("openblas/0.3.27")

        # # Xrlinux特定依赖
        # if self.conf.get("user.os:distro") == "Xrlinux":
        #     self.requires("artosyn/ar9481_0.17.02-00")
                    

    def package_info(self):
        """定义包信息（供依赖者使用）"""
        # 指定生成的库名称（会被链接到最终的可执行文件中）
        self.cpp_info.libs = [
            "nr_hand_tracking",   # 手部跟踪库
            "handtracking",       # 手部跟踪算法库
            "xengine",            # 推理引擎
            "xr_base_graph"       # 基础图处理库
        ]