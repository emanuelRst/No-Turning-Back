# gl-endless-runner
Final project for Computer Graphics: Endless runner game. Developed in C++/OpenGL, focusing on the graphics pipeline, shaders, and game state management.

## Compilation

This project uses CMake and [vcpkg](https://vcpkg.io/) for dependency management.

### Linux

1. Ensure you have `vcpkg` installed.
2. Run the following command in the project root to configure and build:

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[PATH_TO_VCPKG]/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

3. The executable will be found in `build/GL_Test_Multiplatform`.

### Windows

1. Ensure you have Visual Studio (with C++) and `vcpkg` installed.
2. In a Visual Studio terminal ("Developer PowerShell"), run:

```powershell
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[PATH_TO_VCPKG]\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release
```

3. The executable will be found in `build\Release\GL_Test_Multiplatform.exe`.

> **Note:** Replace `[PATH_TO_VCPKG]` with the actual path where you have the vcpkg repository cloned on your machine.
