#include "Game.h"
#include <algorithm>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Inicialización de la instancia estática
Game* Game::instance = nullptr;

// Shader para Phong
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 objectColor;

void main() {
    // Ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
}
)";

namespace {
constexpr float kWorldScale = 0.25f;
constexpr float kHeightScale = 0.2f;
constexpr float kRenderGroundY = -0.5f;
constexpr float kTrainWidth = 1.55f;
constexpr float kTrainHeight = 1.15f;
constexpr float kTrainDepth = 5.0f;
constexpr float kTrainSpeed = 20.0f;
constexpr float kTrainSpawnDistance = 50.0f;
constexpr float kTrainSpacing = 16.0f;
constexpr float kTrainDespawnDistance = 8.0f;
}

Game::Game(int w, int h)
    : window(nullptr), width(w), height(h), shaderProgram(0), VAO(0),
      trains(), nextTrainLane(0) {
    instance = this;
    ResetRun();
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
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);

    // Pantalla completa y ocultar cursor
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Habilitar el test de profundidad
    glEnable(GL_DEPTH_TEST);

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
    float vertices[] = {
        // positions          // normals
        -0.1f, -0.1f,  0.1f,  0.0f,  0.0f,  1.0f,
         0.1f, -0.1f,  0.1f,  0.0f,  0.0f,  1.0f,
         0.1f,  0.1f,  0.1f,  0.0f,  0.0f,  1.0f,
        -0.1f,  0.1f,  0.1f,  0.0f,  0.0f,  1.0f,
        -0.1f, -0.1f, -0.1f,  0.0f,  0.0f, -1.0f,
        -0.1f,  0.1f, -0.1f,  0.0f,  0.0f, -1.0f,
         0.1f,  0.1f, -0.1f,  0.0f,  0.0f, -1.0f,
         0.1f, -0.1f, -0.1f,  0.0f,  0.0f, -1.0f,
        -0.1f,  0.1f, -0.1f,  0.0f,  1.0f,  0.0f,
        -0.1f,  0.1f,  0.1f,  0.0f,  1.0f,  0.0f,
         0.1f,  0.1f,  0.1f,  0.0f,  1.0f,  0.0f,
         0.1f,  0.1f, -0.1f,  0.0f,  1.0f,  0.0f,
        -0.1f, -0.1f, -0.1f,  0.0f, -1.0f,  0.0f,
         0.1f, -0.1f, -0.1f,  0.0f, -1.0f,  0.0f,
         0.1f, -0.1f,  0.1f,  0.0f, -1.0f,  0.0f,
        -0.1f, -0.1f,  0.1f,  0.0f, -1.0f,  0.0f,
        -0.1f, -0.1f, -0.1f, -1.0f,  0.0f,  0.0f,
        -0.1f, -0.1f,  0.1f, -1.0f,  0.0f,  0.0f,
        -0.1f,  0.1f,  0.1f, -1.0f,  0.0f,  0.0f,
        -0.1f,  0.1f, -0.1f, -1.0f,  0.0f,  0.0f,
         0.1f, -0.1f, -0.1f,  1.0f,  0.0f,  0.0f,
         0.1f,  0.1f, -0.1f,  1.0f,  0.0f,  0.0f,
         0.1f,  0.1f,  0.1f,  1.0f,  0.0f,  0.0f,
         0.1f, -0.1f,  0.1f,  1.0f,  0.0f,  0.0f
    };
    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        8, 9, 10, 10, 11, 8,
        12, 13, 14, 14, 15, 12,
        16, 17, 18, 18, 19, 16,
        20, 21, 22, 22, 23, 20
    };
    unsigned int VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    return true;
}

void Game::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (instance && action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);
        if (key == GLFW_KEY_R && instance->player.HasCrashed()) {
            instance->ResetRun();
            return;
        }

        if (key == GLFW_KEY_A || key == GLFW_KEY_LEFT) instance->player.MoveLeft();
        if (key == GLFW_KEY_D || key == GLFW_KEY_RIGHT) instance->player.MoveRight();
        if (key == GLFW_KEY_SPACE || key == GLFW_KEY_W) instance->player.Jump();
    }
}

void Game::FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
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
    if (player.HasCrashed()) {
        return;
    }

    UpdateTrains(deltaTime);

    std::vector<GameObject*> collisionObjects;
    collisionObjects.reserve(trains.size());
    for (Train& train : trains) {
        collisionObjects.push_back(&train);
    }

    player.Update(deltaTime, collisionObjects);
}

void Game::UpdateTrains(float deltaTime) {
    const float playerZ = player.GetPosition().z;

    for (Train& train : trains) {
        train.Update(deltaTime);

        if (train.BackEdgeZ() > playerZ + kTrainDespawnDistance) {
            ResetTrain(train);
        }
    }
}

void Game::ResetTrain(Train& train) {
    float farthestZ = player.GetPosition().z - kTrainSpawnDistance;
    for (const Train& other : trains) {
        if (&other == &train) {
            continue;
        }

        farthestZ = std::min(farthestZ, other.GetPosition().z);
    }

    train.Reset(nextTrainLane, farthestZ - kTrainSpacing,
                glm::vec3(kTrainWidth, kTrainHeight, kTrainDepth), kTrainSpeed);
    nextTrainLane = (nextTrainLane + 2) % 3;
}

void Game::ResetRun() {
    player.Reset();
    nextTrainLane = 0;

    const glm::vec3 trainSize(kTrainWidth, kTrainHeight, kTrainDepth);
    const float playerZ = player.GetPosition().z;
    trains = {
        Train(1, playerZ - kTrainSpawnDistance, trainSize, kTrainSpeed),
        Train(0, playerZ - kTrainSpawnDistance - kTrainSpacing, trainSize, kTrainSpeed),
        Train(2, playerZ - kTrainSpawnDistance - kTrainSpacing * 2.0f, trainSize, kTrainSpeed)
    };
}

void Game::Render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);
    
    glm::vec3 pos = player.GetPosition();
    
    // Cámara inclinada hacia abajo
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 1.0f, 2.5f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Ajustar aspecto para pantalla completa
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    float aspect = (float)fbWidth / (float)fbHeight;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    
    // Escalar si está saltando
    float scaleFactor = player.IsJumping() ? 1.2f : 1.0f;
    glm::vec3 playerSize = player.GetCollisionSize();
    
    glm::mat4 model = glm::mat4(1.0f);
    // Centrado en X (pos.x * 0.25f), y movido hacia abajo en Y
    model = glm::translate(model, glm::vec3(pos.x * kWorldScale, kRenderGroundY + (pos.y * kHeightScale), 0.0f));
    model = glm::scale(model, glm::vec3(playerSize.x * 1.25f,
                                        playerSize.y,
                                        playerSize.z * 1.25f) * scaleFactor);
    
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 1.0f, 1.0f, 2.0f);
    glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), 0.0f, 1.0f, 2.5f); // Actualizado con la posición de la cámara
    glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
    if (player.HasCrashed()) {
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.9f, 0.15f, 0.1f);
    } else {
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.2f, 0.6f, 1.0f);
    }
    
    // Dibuja el jugador con la posicion calculada por Player.
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    // Dibuja los trenes usando el mismo cubo base del jugador.
    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.85f, 0.2f, 0.12f);
    for (const Train& train : trains) {
        const glm::vec3 objectPosition = train.GetPosition();
        const glm::vec3 objectSize = train.GetHitboxSize();
        glm::mat4 objectModel = glm::mat4(1.0f);
        // Se aplica la misma escala visual de Y que usa el jugador para mantener coherencia.
        objectModel = glm::translate(objectModel, glm::vec3(objectPosition.x * kWorldScale,
                                                           kRenderGroundY + objectPosition.y * kHeightScale,
                                                           (objectPosition.z - pos.z) * kWorldScale));
        // Ajusta el cubo unitario al tamano logico del objeto pisable.
        objectModel = glm::scale(objectModel, glm::vec3(objectSize.x * 1.25f,
                                                        objectSize.y,
                                                        objectSize.z * 1.25f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(objectModel));
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }
}
