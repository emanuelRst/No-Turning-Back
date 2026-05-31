#include "Game.h"
#include <iostream>
#include <glm/glm.hpp>

// Inicialización de la instancia estática
Game* Game::instance = nullptr;

// Shader simple para mover el jugador
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

Game::Game(int w, int h) : width(w), height(h), window(nullptr), shaderProgram(0), VAO(0) {
    instance = this;
}

Game::~Game() {
    glfwTerminate();
}

bool Game::Init() {
    if (!glfwInit()) return false;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Endless Runner Prototipo", NULL, NULL);
    if (!window) return false;
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    
    // Configurar callback
    glfwSetKeyCallback(window, KeyCallback);

    // Compilar Shaders
    unsigned int vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vShader);
    unsigned int fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fShader);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vShader);
    glAttachShader(shaderProgram, fShader);
    glLinkProgram(shaderProgram);

    // VAO del Jugador
    float vertices[] = { -0.1f, -0.1f, 0.0f,  0.1f, -0.1f, 0.0f,  0.1f, 0.1f, 0.0f, -0.1f, 0.1f, 0.0f };
    unsigned int indices[] = { 0, 1, 2, 2, 3, 0 };
    unsigned int VBO, EBO;
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

    return true;
}

void Game::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (instance && action == GLFW_PRESS) {
        if (key == GLFW_KEY_A || key == GLFW_KEY_LEFT) instance->player.MoveLeft();
        if (key == GLFW_KEY_D || key == GLFW_KEY_RIGHT) instance->player.MoveRight();
        if (key == GLFW_KEY_SPACE || key == GLFW_KEY_W) instance->player.Jump();
        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);
    }
}

void Game::Run() {
    float lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        Update(deltaTime);
        Render();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void Game::Update(float deltaTime) {
    player.Update(deltaTime);
}

void Game::Render() {
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(shaderProgram);
    glm::vec3 pos = player.GetPosition();
    glUniform3f(glGetUniformLocation(shaderProgram, "u_offset"), pos.x * 0.25f, (pos.y * 0.5f) - 0.7f, 0.0f);
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
