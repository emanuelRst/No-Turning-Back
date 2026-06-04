#include "Game.h"
#include "Menu.h"
#include <algorithm>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Inicialización de la instancia estática
Game* Game::instance = nullptr;

// Shader para Phong con soporte de texturas
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in ivec4 aBoneIDs;
layout (location = 4) in vec4 aWeights;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 boneMatrices[100];
uniform bool isAnimated;

void main() {
    vec4 totalPosition;
    if (isAnimated) {
        mat4 BoneTransform = boneMatrices[aBoneIDs[0]] * aWeights[0];
        BoneTransform += boneMatrices[aBoneIDs[1]] * aWeights[1];
        BoneTransform += boneMatrices[aBoneIDs[2]] * aWeights[2];
        BoneTransform += boneMatrices[aBoneIDs[3]] * aWeights[3];
        totalPosition = BoneTransform * vec4(aPos, 1.0);
    } else {
        totalPosition = vec4(aPos, 1.0);
    }
    
    FragPos = vec3(model * totalPosition);
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoords = aTexCoords;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform sampler2D texture_diffuse1;
uniform bool useTexture;

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

    vec4 texColor = useTexture ? texture(texture_diffuse1, TexCoords) : vec4(1.0);
    vec3 result = (ambient + diffuse + specular) * objectColor * texColor.rgb;
    FragColor = vec4(result, texColor.a);
}
)";

const char* menuVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 projection;
void main() {
    gl_Position = projection * model * vec4(aPos, 1.0);
}
)";

const char* menuFragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
uniform vec3 objectColor;
void main() {
    FragColor = vec4(objectColor, 1.0);
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
// Nuevas constantes para el suelo
constexpr float kGroundSegmentLength = 20.0f;
constexpr int kNumGroundSegments = 5;
}

Game::Game(int w, int h)
    : window(nullptr), width(w), height(h), shaderProgram(0), VAO(0),
      playerModel(nullptr), trains(), nextTrainLane(0), 
      currentState(GameState::MENU), menu(new Menu()) {
    instance = this;
    
    // Configurar botones del menú (Tamaño 600x150)
    menu->AddButton("Iniciar Juego", 660, 200, 600, 150, [this](){ 
        this->currentState = GameState::PLAYING;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    });
    menu->AddButton("Cambiar Personajes", 660, 370, 600, 150, [](){ std::cout << "Personajes\n"; });
    menu->AddButton("Opciones", 660, 540, 600, 150, [](){ std::cout << "Opciones\n"; });
    menu->AddButton("Creditos", 660, 710, 600, 150, [](){ std::cout << "Creditos\n"; });
    
    ResetRun();
}

Game::~Game() {
    delete playerModel;
    delete menu;
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
    
    // Inicializar menú
    menu->Init("assets/fonts/stocky.ttf");

    // Cargar modelo del jugador
    playerModel = new Model("assets/models/run_forrest/scene.gltf");

    // Configurar callback
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);

    // Pantalla completa y ocultar cursor (cambiado inicialmente para ver el menu)
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Se gestionará según el estado

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

    // Compilar Menu Shader
    unsigned int vMenuShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vMenuShader, 1, &menuVertexShaderSource, NULL);
    glCompileShader(vMenuShader);
    unsigned int fMenuShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fMenuShader, 1, &menuFragmentShaderSource, NULL);
    glCompileShader(fMenuShader);
    menuShaderProgram = glCreateProgram();
    glAttachShader(menuShaderProgram, vMenuShader);
    glAttachShader(menuShaderProgram, fMenuShader);
    glLinkProgram(menuShaderProgram);

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
        if (key == GLFW_KEY_ESCAPE) {
            if (instance->currentState == GameState::PLAYING) {
                instance->currentState = GameState::MENU;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            } else {
                glfwSetWindowShouldClose(window, true);
            }
        }
        
        if (instance->currentState == GameState::PLAYING) {
            if (key == GLFW_KEY_R && instance->player.HasCrashed()) {
                instance->ResetRun();
                return;
            }

            if (key == GLFW_KEY_A || key == GLFW_KEY_LEFT) instance->player.MoveLeft();
            if (key == GLFW_KEY_D || key == GLFW_KEY_RIGHT) instance->player.MoveRight();
            if (key == GLFW_KEY_SPACE || key == GLFW_KEY_W) instance->player.Jump();
        }
    }
}

void Game::CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (instance && instance->currentState == GameState::MENU) {
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        instance->menu->Update(xpos, ypos, fbWidth, fbHeight);
    }
}

void Game::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (instance && instance->currentState == GameState::MENU && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        if (instance->menu->HandleClick(xpos, ypos)) {
            // Si el clic fue en "Iniciar", cambiar estado
            // (esto se conectará mejor cuando implementemos la función lambda real en el constructor)
        }
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
    if (currentState == GameState::MENU) return;

    if (player.HasCrashed()) {
        return;
    }

    UpdateTrains(deltaTime);
    UpdateGround(deltaTime); // Nuevo

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

void Game::UpdateGround(float deltaTime) {
    const float playerZ = player.GetPosition().z;
    
    // Buscar el segmento mas lejano
    float farthestZ = 0.0f;
    for (const auto& segment : groundSegments) {
        farthestZ = std::min(farthestZ, segment.GetPosition().z);
    }

    // Reciclar segmentos que quedaron detras
    for (auto& segment : groundSegments) {
        if (segment.GetPosition().z > playerZ + kTrainDespawnDistance) {
            segment.SetPosition(glm::vec3(0.0f, 0.0f, farthestZ - kGroundSegmentLength));
            farthestZ = segment.GetPosition().z;
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

    // Inicializar suelo
    groundSegments.clear();
    for (int i = 0; i < kNumGroundSegments; ++i) {
        float zPos = playerZ - (i * kGroundSegmentLength);
        groundSegments.emplace_back(glm::vec3(0.0f, 0.0f, zPos), glm::vec3(5.0f, 0.1f, kGroundSegmentLength));
    }
}

void Game::Render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if (currentState == GameState::MENU) {
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        menu->Render(menuShaderProgram, VAO, fbWidth, fbHeight);
        glfwSwapBuffers(window);
        return;
    }

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
    model = glm::scale(model, glm::vec3(playerSize.x * 0.8f,
                                        playerSize.y * 0.8f,
                                        playerSize.z * 0.8f) * scaleFactor);
    
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 1.0f, 1.0f, 2.0f);
    glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), 0.0f, 1.0f, 2.5f); // Actualizado con la posición de la cámara
    glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);
    if (player.HasCrashed()) {
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.9f, 0.15f, 0.1f);
    } else {
        glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 1.0f, 1.0f, 1.0f);
    }
    
    // Dibuja el jugador con la posicion calculada por Player.
    if (playerModel) {
        // Pass animated time
        float animTime = (float)glfwGetTime();

        // Enable animation in shader
        glUniform1i(glGetUniformLocation(shaderProgram, "isAnimated"), 1);

        // Ajuste de centrado (ejemplo: offset en Y si el modelo está alto)
        glm::mat4 visualModel = glm::translate(model, glm::vec3(0.0f, -0.5f, 0.0f)); 
        visualModel = glm::scale(visualModel, glm::vec3(0.3f)); // Reduced size
        visualModel = glm::rotate(visualModel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        playerModel->Draw(shaderProgram, visualModel, animTime);
    } else {
        glUniform1i(glGetUniformLocation(shaderProgram, "isAnimated"), 0);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }

    // Dibuja los trenes usando el mismo cubo base del jugador.
    glUniform1i(glGetUniformLocation(shaderProgram, "isAnimated"), 0); // Disable animation for trains
    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.85f, 0.2f, 0.12f);
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0); // Disable texture
    glBindVertexArray(VAO);
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

    // Dibuja el suelo
    glUniform1i(glGetUniformLocation(shaderProgram, "isAnimated"), 0); // Disable animation for ground
    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.3f, 0.3f, 0.3f);
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0); // Disable texture
    for (const auto& segment : groundSegments) {
        const glm::vec3 objectPosition = segment.GetPosition();
        const glm::vec3 objectSize = segment.GetHitboxSize();
        glm::mat4 objectModel = glm::mat4(1.0f);
        objectModel = glm::translate(objectModel, glm::vec3(objectPosition.x * kWorldScale,
                                                           kRenderGroundY - 0.1f, // Ligeramente debajo
                                                           (objectPosition.z - pos.z) * kWorldScale));
        objectModel = glm::scale(objectModel, glm::vec3(objectSize.x * 1.5f,
                                                        objectSize.y,
                                                        objectSize.z * 1.25f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(objectModel));
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }

    // Dibuja el suelo
    glUniform1i(glGetUniformLocation(shaderProgram, "isAnimated"), 0); // Disable animation for ground
    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.3f, 0.3f, 0.3f);
    // ... (rest of code for ground)


    // Dibuja los trenes usando el mismo cubo base del jugador.
    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.85f, 0.2f, 0.12f);
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0); // Disable texture
    glBindVertexArray(VAO);
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

    // Dibuja el suelo
    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.3f, 0.3f, 0.3f);
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0); // Disable texture
    for (const auto& segment : groundSegments) {
        const glm::vec3 objectPosition = segment.GetPosition();
        const glm::vec3 objectSize = segment.GetHitboxSize();
        glm::mat4 objectModel = glm::mat4(1.0f);
        objectModel = glm::translate(objectModel, glm::vec3(objectPosition.x * kWorldScale,
                                                           kRenderGroundY - 0.1f, // Ligeramente debajo
                                                           (objectPosition.z - pos.z) * kWorldScale));
        objectModel = glm::scale(objectModel, glm::vec3(objectSize.x * 1.5f,
                                                        objectSize.y,
                                                        objectSize.z * 1.25f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(objectModel));
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }
}
