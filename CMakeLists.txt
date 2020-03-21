cmake_minimum_required(VERSION 3.14)

project(rasterizer VERSION 0.0.2)

include("cmake/StandardProjectSettings.cmake")
include("CMakeDependentOption")

include("cmake/compiler.cmake")
include("cmake/clangformat.cmake")
include("cmake/StaticAnalyzers.cmake")

include("cmake/conan.cmake")
run_conan()


if (EMSCRIPTEN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_SDL=2 -s ALLOW_MEMORY_GROWTH=1 -s ASSERTIONS=1 -s SAFE_HEAP=1 -s STACK_OVERFLOW_CHECK=2 -s DEMANGLE_SUPPORT=1 -std=c++17 -g4 --preload-file assets")
endif ()

add_subdirectory(third-party)
add_subdirectory(src)

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
    include(CTest)
endif ()

if (CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING AND NOT EMSCRIPTEN)
    add_subdirectory(test)
endif ()