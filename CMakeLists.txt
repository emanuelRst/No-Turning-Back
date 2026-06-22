cmake_minimum_required(VERSION 3.15)
project(GL_Test_Multiplatform)

# Estándar de C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Buscar dependencias mediante vcpkg
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenGL REQUIRED)
find_package(soil2 CONFIG REQUIRED)

# Intentar buscar OpenAL y SndFile vía configuración de CMake (vcpkg)
find_package(OpenAL CONFIG QUIET)
find_package(SndFile CONFIG QUIET)

# Fallback a PkgConfig si no se encuentran por configuración
if(NOT OpenAL_FOUND OR NOT SndFile_FOUND)
    find_package(PkgConfig REQUIRED)
    if(NOT OpenAL_FOUND)
        pkg_check_modules(OPENAL REQUIRED openal)
    endif()
    if(NOT SndFile_FOUND)
        pkg_check_modules(SNDFILE REQUIRED sndfile)
    endif()
endif()

# Definir targets para vincular de forma consistente
if(OpenAL_FOUND)
    set(OPENAL_LIBRARIES OpenAL::OpenAL)
    set(OPENAL_INCLUDE_DIRS "")
else()
    set(OPENAL_LIBRARIES ${OPENAL_LIBRARIES})
    set(OPENAL_INCLUDE_DIRS ${OPENAL_INCLUDE_DIRS})
endif()

if(SndFile_FOUND)
    set(SNDFILE_LIBRARIES SndFile::sndfile)
    set(SNDFILE_INCLUDE_DIRS "")
else()
    set(SNDFILE_LIBRARIES ${SNDFILE_LIBRARIES})
    set(SNDFILE_INCLUDE_DIRS ${SNDFILE_INCLUDE_DIRS})
endif()

# Crear el ejecutable del Runner
add_executable(EndlessRunner 
    src/LevelGenerator.cpp
    src/GameObject.cpp
    src/Train.cpp
    src/ObstacleOverhead.cpp
    src/RampTrain.cpp
    src/Player.cpp 
    src/Game.cpp 
    src/main.cpp 
    src/Model.cpp
    src/Menu.cpp
    src/AudioManager.cpp
    src/States.cpp
    src/Coin.cpp
    vendor/glad/glad.c
    assets/Icon/resource.rc
)

# Copiar activos a la carpeta de build
add_custom_command(TARGET EndlessRunner POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:EndlessRunner>/assets
)

# Incluir las carpetas de cabeceras para que el compilador las encuentre
target_include_directories(EndlessRunner PRIVATE 
    include 
    src 
    vendor
    ${OPENAL_INCLUDE_DIRS}
    ${SNDFILE_INCLUDE_DIRS}
)

# Vincular las librerías al ejecutable
target_link_libraries(EndlessRunner PRIVATE 
    glfw 
    glm::glm 
    assimp::assimp
    nlohmann_json::nlohmann_json
    ${OPENAL_LIBRARIES}
    ${SNDFILE_LIBRARIES}
    ${OPENGL_LIBRARIES}
    soil2
)

# Copiar o configurar que reconozca Linux vs Windows para hilos si es necesario
if(LINUX)
    target_link_libraries(EndlessRunner PRIVATE pthread dl)
endif()

