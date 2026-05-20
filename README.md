# gl-endless-runner
Proyecto Final de Programacion Grafica: Juego tipo endless runner. Desarrollado en C++/OpenGL, enfocado en el pipeline gráfico, shaders y gestión de estados de juego.

## Compilación

Este proyecto utiliza CMake y [vcpkg](https://vcpkg.io/) para la gestión de dependencias.

### Linux

1. Asegúrate de tener `vcpkg` instalado.
2. Ejecuta el siguiente comando en la raíz del proyecto para configurar y compilar:

```bash
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[RUTA_A_VCPKG]/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

3. El ejecutable se encontrará en `build/GL_Test_Multiplatform`.

### Windows

1. Asegúrate de tener instalado Visual Studio (con C++) y `vcpkg`.
2. En una terminal de Visual Studio ("Developer PowerShell"), ejecuta:

```powershell
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[RUTA_A_VCPKG]\scripts\buildsystems\vcpkg.cmake
cmake --build build --config Release
```

3. El ejecutable se encontrará en `build\Release\GL_Test_Multiplatform.exe`.

> **Nota:** Reemplaza `[RUTA_A_VCPKG]` por la ruta real donde tengas clonado el repositorio de vcpkg en tu equipo.
