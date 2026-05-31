cmake_minimum_required(VERSION 3.15)
project(GL_Test_Multiplatform)

# Estándar de C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Buscar dependencias mediante vcpkg
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# Buscar todos los archivos fuentes (.cpp y .c)
file(GLOB SOURCES 
    "src/*.cpp"
    "src/*.h"
    "vendor/glad/glad.c"
)

# Crear el ejecutable original
add_executable(${PROJECT_NAME} ${SOURCES})

# Crear el ejecutable del Runner
add_executable(EndlessRunner src/Player.cpp src/RunnerMain.cpp vendor/glad/glad.c)

# Incluir las carpetas de cabeceras para que el compilador las encuentre
target_include_directories(${PROJECT_NAME} PRIVATE src vendor .)
target_include_directories(EndlessRunner PRIVATE src vendor .)

# Vincular las librerías al ejecutable
target_link_libraries(EndlessRunner PRIVATE 
    glfw 
    glm::glm 
    ${OPENGL_LIBRARIES}
)

target_link_libraries(EndlessRunner PRIVATE 
    glm::glm
)

# Copiar o configurar que reconozca Linux vs Windows para hilos si es necesario
if(LINUX)
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread dl)
    target_link_libraries(EndlessRunner PRIVATE pthread)
endif()
