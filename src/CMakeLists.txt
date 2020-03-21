add_subdirectory(common)

if (EMSCRIPTEN)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

    add_custom_target(www
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/www ${CMAKE_BINARY_DIR}/bin
            )

    add_executable(app "web_main.cpp")
    target_link_libraries(app
            PRIVATE
            common
            compiler_options
            )
    add_dependencies(app www)
    add_clangformat(app)

    message("Build for emscripten")
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set(EMCC_LINKER_FLAGS "")
    set_target_properties(app
            PROPERTIES LINK_FLAGS
            "-s 'EXTRA_EXPORTED_RUNTIME_METHODS=[\"ccall\", \"cwrap\"]' -s MAX_WEBGL_VERSION=2 ")
else ()
    add_executable(app "main.cpp" main.cpp)
    target_link_libraries(app
            PRIVATE
            common
            compiler_options
            )
    add_clangformat(app)
endif ()


if (EMSCRIPTEN)
    set(ASSETS_DESTINATION ${CMAKE_BINARY_DIR}/src/assets)
else ()
    set(ASSETS_DESTINATION ${CMAKE_BINARY_DIR}/bin/assets)
endif ()
add_custom_target(assets
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets ${ASSETS_DESTINATION}
        )
add_dependencies(app assets)
