from conans import ConanFile, CMake


class JumpyThievesWasm(ConanFile):
    settings = {"os": ["Emscripten"]}
    requires = ["entt/3.1.1@skypjack/stable",
                "glm/0.9.8.5@bincrafters/stable",
                "gsl_microsoft/2.0.0@bincrafters/stable",
                "spdlog/1.5.0",
                "imgui/1.75"]
    generators = ["cmake"]

    def _configure_cmake(self):
        cmake = CMake(self)
        cmake.configure()
        return cmake

    def build(self):
        cmake = self._configure_cmake()
        cmake.build()
