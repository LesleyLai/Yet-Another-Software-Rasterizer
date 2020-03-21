# Yet another Software Rasterizer
A pure software rasterizer.

## Build
### Emscripten
First you need to bring emscripten in path.
```shell
source <path to emsdk>/emsdk_env.sh
```

Then you can `conan install` with the wasm recipe file.
``` shell
conan install ./wasm.recipe.py --build missing --install-folder cmake-build-emscripten-release -pr ./wasm.profile
```

You can continue with the normal CMake routine after the conan setup.