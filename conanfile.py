from conans import CMake, python_requires
nlo = python_requires("nlo-cmake-pythonlib/1.0@snap/stable")

class Zoo(nlo.NloBasicCmakeConanFile):
    name = "zoo"
    generators = ("nlo_cmake_link", "cmake", "json")
    exports_sources = ["CMakeLists.txt", "inc/*"]
    options = {"production": [False, True]}
    default_options = {"production": False}

    def build(self):
        self.simple_cmake_build()
