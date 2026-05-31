cmake_minimum_required(VERSION 3.15)
project(GL_Test_Multiplatform)

# Estándar de C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Buscar dependencias mediante vcpkg
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# Crear el ejecutable del Runner
add_executable(EndlessRunner 
    src/Player.cpp 
    src/Game.cpp 
    src/main.cpp 
    vendor/glad/glad.c
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
    ${OPENGL_LIBRARIES}
)

# Copiar o configurar que reconozca Linux vs Windows para hilos si es necesario
if(LINUX)
    target_link_libraries(EndlessRunner PRIVATE pthread dl)
endif()

