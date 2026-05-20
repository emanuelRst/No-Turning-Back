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

# Crear el ejecutable
add_executable(${PROJECT_NAME} ${SOURCES})

# Incluir las carpetas de cabeceras para que el compilador las encuentre
target_include_directories(${PROJECT_NAME} PRIVATE src vendor .)

# Vincular las librerías al ejecutable
target_link_libraries(${PROJECT_NAME} PRIVATE 
    glfw 
    glm::glm 
    ${OPENGL_LIBRARIES}
)

# Copiar o configurar que reconozca Linux vs Windows para hilos si es necesario
if(LINUX)
    target_link_libraries(${PROJECT_NAME} PRIVATE pthread dl)
endif()