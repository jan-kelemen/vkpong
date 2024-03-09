# vkpong

Pong clone using Vulkan rendering engine.

## Building
Necessary build tools are:
* CMake 3.27 or higher
* Conan 2.0 or higher
  * See [installation instructions](https://docs.conan.io/2/installation.html)
* One of supported compilers:
  * Clang-17
  * GCC-13
  * Visual Studio 2022 (MSVC v193)

```
conan install . --profile=conan/clang-17 --build=missing --settings build_type=Release
cmake --preset release
cmake --build --preset=release
```

