from conan import ConanFile
class AISDK(ConanFile):
    python_requires = "project_base/1.0"
    python_requires_extend = "project_base.ProjectBase"
    def init(self):
        base = self.python_requires["project_base"].module.ProjectBase
        self.settings = base.settings
        self.options.update(base.options, base.default_options)
        self.revision_mode = base.revision_mode
    def build_requirements(self):
        if self.settings.os == "Android": 
            self.tool_requires("android-ndk/r25c")
        self.test_requires("doctest/2.4.11")
    def requirements(self):
        self.requires("fmt/9.1.0", transitive_headers=True, transitive_libs=True)
        self.requires("opencv/4.5.5", transitive_headers=True, transitive_libs=True)
        self.requires("eigen/3.3.7", transitive_headers=True, transitive_libs=True)
        self.requires("jsoncpp/1.9.5", transitive_headers=True, transitive_libs=True)
        self.requires("openssl/1.1.1m", transitive_headers=True, transitive_libs=True)
        self.requires("nreal_mnn/2.0.0", transitive_headers=True, transitive_libs=True)
        self.requires("snpe/2.17.0", transitive_headers=False, transitive_libs=False)
        self.requires(super().override_require("framework/jenkins"), run=True)

    def package_info(self):
        self.cpp_info.libs = ["test"]