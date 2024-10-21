from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps
import os
class AISDK(ConanFile):
    python_requires = "project_base/1.0"
    python_requires_extend = "project_base.ProjectBase"
        
    def init(self):
        base = self.python_requires["project_base"].module.ProjectBase
        self.settings = base.settings
        self.options.update(base.options, base.default_options)
        self.revision_mode = base.revision_mode
    def build_requirements(self):
        super().build_requirements()
        if os.getenv('BUILD_TOOL'):
            self.tool_requires("grpc/1.54.3")
            self.tool_requires("protobuf/3.21.9")
        if self.settings.os == "Android": 
            self.tool_requires("android-ndk/r25c")
        self.test_requires("doctest/2.4.11")
    
    def build(self):
        update_model_cmd = "git submodule init && git submodule update"
        os.system(update_model_cmd)
        super().build()
        

    def generate(self):
        tc = CMakeToolchain(self)
        if os.getenv('ENABLE_XGRAPH_PROFILER') == 'ON':
            tc.variables["ENABLE_XGRAPH_PROFILER"] = True
        if os.getenv('BUILD_TOOL') == 'ON':
            tc.variables["BUILD_TOOL"] = True
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def requirements(self):
        if os.getenv('BUILD_TOOL'):
            self.requires("grpc/1.54.3")
        self.requires("fmt/9.1.0", transitive_headers=True, transitive_libs=True)
        self.requires("opencv/4.5.5", transitive_headers=True, transitive_libs=True)
        self.requires("eigen/3.3.7", transitive_headers=True, transitive_libs=True)
        self.requires("jsoncpp/1.9.5", transitive_headers=True, transitive_libs=True)
        self.requires("openssl/3.2.2", transitive_headers=True, transitive_libs=True)
        self.requires("nreal_mnn/2.0.0", transitive_headers=True, transitive_libs=True)
        if self.settings.os in ["Linux", "Android"] and self.conf.get("user.os:distro") != "Xrlinux":
            self.requires("snpe/2.17.0", transitive_headers=False, transitive_libs=False)
        self.requires("camera_model/develop", transitive_libs=True)
        if self.settings.os == "Linux" and self.conf.get("user.os:distro") != "Xrlinux":
            self.requires("framework/jenkins#835223d03ea5fdca60b7c57a1a759936897707f0")
        else:
            #self.requires(super().override_require("framework/jenkins"), run=True)
            self.requires(super().override_require("framework/jenkins#f86342896a5a5513b153e90ae1991e704dbd3070"), run=True)
        xgraph_version = "xgraph/main"
        enable_xgraph_profiler = False
        if os.getenv('ENABLE_XGRAPH_PROFILER') == 'ON':
            print("Enable xgraph profiler !!!!!!!!")
            enable_xgraph_profiler = True
            xgraph_version = "xgraph/0.10.0.profiler"
        self.requires(xgraph_version, transitive_libs=True, options={"enable_profiler": enable_xgraph_profiler})
        self.requires("abseil/20230125.3", transitive_libs=True)
        self.requires("protobuf/3.21.9", transitive_libs=True)
        self.requires("glog/0.6.0", transitive_libs=True)
        self.requires("suitesparse/5.7.1")  #ceres-slover依赖suitesparse/5.7.1
        self.requires("ceres-solver/2.0.0.1")
        if self.settings.os in ["Linux", "Android", "Windows"] and self.conf.get("user.os:distro") != "Xrlinux":
            self.requires("openblas/0.3.27")
        if self.conf.get("user.os:distro") == "Xrlinux":
            self.requires("artosyn/ar9481_0.17.02-00")
                    
    def package_info(self):
        self.cpp_info.libs = ["nr_hand_tracking","handtracking","xengine","xr_base_graph"]