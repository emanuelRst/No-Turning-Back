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

# Crear el ejecutable del Runner
add_executable(EndlessRunner 
    src/GameObject.cpp
    src/Train.cpp
    src/Player.cpp 
    src/Game.cpp 
    src/main.cpp 
    src/Model.cpp
    src/Menu.cpp
    vendor/glad/glad.c
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
)

# Vincular las librerías al ejecutable
target_link_libraries(EndlessRunner PRIVATE 
    glfw 
    glm::glm 
    assimp::assimp
    nlohmann_json::nlohmann_json
    ${OPENGL_LIBRARIES}
)

# Copiar o configurar que reconozca Linux vs Windows para hilos si es necesario
if(LINUX)
    target_link_libraries(EndlessRunner PRIVATE pthread dl)
endif()

