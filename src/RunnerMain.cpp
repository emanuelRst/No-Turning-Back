#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include "Player.h"

// Variables Globales para callback
Player player;

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_A || key == GLFW_KEY_LEFT) player.MoveLeft();
        if (key == GLFW_KEY_D || key == GLFW_KEY_RIGHT) player.MoveRight();
        if (key == GLFW_KEY_SPACE || key == GLFW_KEY_W) player.Jump();
        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);
    }
}

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform vec3 u_offset;
void main() {
    gl_Position = vec4(aPos + u_offset, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(0.2f, 0.6f, 1.0f, 1.0f);
}
)";

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Endless Runner Prototipo", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSetKeyCallback(window, key_callback);

    // Shaders
    unsigned int vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vShader);
    unsigned int fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fShader);
    unsigned int program = glCreateProgram();
    glAttachShader(program, vShader);
    glAttachShader(program, fShader);
    glLinkProgram(program);

    // Cuadrado (Jugador)
    float vertices[] = { -0.1f, -0.1f, 0.0f,  0.1f, -0.1f, 0.0f,  0.1f, 0.1f, 0.0f, -0.1f, 0.1f, 0.0f };
    unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    float lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        player.Update(deltaTime);

        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(program);
        
        // Convertir posición del player (mundo) a coordenadas NDC (-1 a 1)
        // Jugador está en X: -2,0,2 -> NDC: -0.5, 0, 0.5 (aprox)
        // Jugador está en Y: 0+ -> NDC: -0.8 + Y*0.5
        glm::vec3 pos = player.GetPosition();
        glUniform3f(glGetUniformLocation(program, "u_offset"), pos.x * 0.25f, (pos.y * 0.5f) - 0.7f, 0.0f);
        
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}
