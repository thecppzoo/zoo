import nlo

class Zoo(nlo.NloBasicCmakeConanFile):
    name = "zoo"
    generators = ("nlo_cmake_link", "cmake", "json")
    exports_sources = ["CMakeLists.txt", "inc/*"]
    options = {"production": [False, True]}
    default_options = {"production": False}

    def build(self):
        self.simple_cmake_build()
