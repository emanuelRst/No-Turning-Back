# gl-endless-runner
Final project for Computer Graphics: Endless runner game. Developed in C++/OpenGL, focusing on the graphics pipeline, shaders, and game state management.

## Compilation

This project uses CMake and [vcpkg](https://vcpkg.io/) for dependency management.

> **Crucial:** You must provide the vcpkg toolchain file when configuring the project with CMake.

### Linux

#### Install system dependencies

```bash
sudo apt install build-essential cmake \
     libgl1-mesa-dev libglu1-mesa-dev \
     libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev \
     libopenal-dev libsndfile1-dev
```

#### Setup vcpkg (if not installed)

```bash
git clone https://github.com/Microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT="$(pwd)/vcpkg"
```

#### Build

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

The executable will be found in `build/EndlessRunner`.

### Windows

1. Install Visual Studio with "Desktop development with C++" workload.
2. Install [vcpkg](https://vcpkg.io/) (clone and run `bootstrap-vcpkg.bat`).
3. In a Visual Studio terminal ("Developer PowerShell"), run:

```powershell
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[PATH_TO_VCPKG]\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release
```

4. The executable will be found in `build\Release\EndlessRunner.exe`.

> **Note:** Replace `[PATH_TO_VCPKG]` with the actual path where you have the `vcpkg` repository cloned on your machine.
> If you are on Linux and prefer system libraries, CMake will automatically fallback to them if `vcpkg` packages are not used.

# Gameplay images
<img width="1920" height="1080" alt="Manu" src="https://github.com/user-attachments/assets/2263ca78-0792-43f0-a602-5b54b5ff5259" />
<img width="1920" height="1080" alt="CharacterSelect " src="https://github.com/user-attachments/assets/9d70c703-d517-4ef3-821c-e766b2854028" />
<img width="1920" height="1080" alt="gameplay" src="https://github.com/user-attachments/assets/1fd12518-4af3-406e-8e1c-b4023154a5e1" />


