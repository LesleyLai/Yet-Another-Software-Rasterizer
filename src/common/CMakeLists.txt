add_library(common
        file_util.cpp file_util.hpp app.cpp app.hpp image.hpp color.cpp color.hpp model.cpp model.hpp stb_image_impl.cpp yasr.cpp yasr.hpp yasr_raii.hpp)
target_link_libraries(common
        PUBLIC
        CONAN_PKG::stb
        CONAN_PKG::tinyobjloader
        CONAN_PKG::spdlog
        beyond::core
        PRIVATE
        compiler_options
        )

if (NOT EMSCRIPTEN)
    target_link_libraries(common
            PUBLIC
            CONAN_PKG::sdl2
            CONAN_PKG::sdl2_image)
endif ()

target_include_directories(common PUBLIC "${CMAKE_SOURCE_DIR}/include")

if (${YASR_BUILD_TESTS_COVERAGE})
    target_compile_options(common PUBLIC -fprofile-arcs -ftest-coverage)
    target_link_libraries(common PUBLIC gcov)
endif ()