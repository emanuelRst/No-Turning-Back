#include "Game.h"
#include "Menu.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Inicialización de la instancia estática
Game* Game::instance = nullptr;

static std::string ReadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "ERROR::SHADER::FILE_NOT_FOUND: " << path << std::endl;
        return "";
    }
    std::stringstream stream;
    stream << file.rdbuf();
    file.close();
    return stream.str();
}

namespace {
constexpr float kRenderGroundY = 0.0f;
constexpr float kTrainWidth = 1.20f;
constexpr float kTrainHeight = 1.15f;
constexpr float kTrainDepth = 5.0f;
constexpr float kBaseTrainSpeed = 20.0f;
// Rampa de velocidad: la velocidad crece sin tope. Con 300s tarda 5 minutos
// en duplicar la base (de 20 a 40 m/s) y se sigue incrementando linealmente.
constexpr float kSpeedRampTime = 300.0f;
constexpr float kTrainSpawnDistance = 50.0f;
constexpr float kTrainSpacing = 16.0f;
constexpr float kTrainDespawnDistance = 8.0f;
// Nuevas constantes para el suelo
constexpr float kGroundSegmentLength = 20.0f;
constexpr int kNumGroundSegments = 5;
}

Game::Game(int w, int h)
    : window(nullptr), width(w), height(h), shaderProgram(0), VAO(0),
      playerModel(nullptr), trains(), nextTrainLane(0), gameTime(0.0f), groundScroll(0.0f),
      currentState(GameState::MENU), menu(new Menu()), gameOverMenu(new Menu()), pauseMenu(new Menu()), helpMenu(new Menu()) {
    instance = this;

    float buttonWidth = 250.0f;
    
    // Configurar botones del menú (Tamaño 600x150)
    menu->AddButton("Start Game", 660 + buttonWidth, 200, 600, 150, [this](){ 
        this->ResetRun();
        this->currentState = GameState::PLAYING;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }, "assets/audio/Menu/MenuEfecto.wav");
    menu->AddButton("Change Characters", 680 + buttonWidth, 370, 600, 150, [](){ std::cout << "Characters\n"; }, "assets/audio/Menu/MenuEfecto.wav");
    menu->AddButton("Help", 660 + buttonWidth, 540, 600, 150, [this](){
        this->currentState = GameState::HELP;
    }, "assets/audio/Menu/MenuEfecto.wav");
    menu->AddButton("Credits", 660 + buttonWidth, 710, 600, 150, [](){ std::cout << "Credits\n"; }, "assets/audio/Menu/MenuEfecto.wav");
    menu->AddButton("Exit", 660 + buttonWidth, 880, 600, 150, [this](){
        glfwSetWindowShouldClose(this->window, true);
    }, "assets/audio/Menu/MenuEfecto.wav");
    
    // Configurar botones de Game Over (uno al lado del otro, más abajo)
    float gameOverY = 700.0f;
    float btnGap = 400.0f;
    gameOverMenu->AddButton("Restart", (float)width / 2.0f + btnGap / 2.0f, gameOverY, 300, 100, [this](){
        this->ResetRun();
        this->currentState = GameState::PLAYING;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }, "assets/audio/Menu/MenuEfecto.wav");
    gameOverMenu->AddButton("Back to Menu", (float)width / 2.0f + btnGap + 400.0f, gameOverY, 300, 100, [this](){
        this->currentState = GameState::MENU;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }, "assets/audio/Menu/MenuEfecto.wav");

    // Configurar botones de Pausa
    float pauseY = (float)height / 2.0f;
    pauseMenu->AddButton("Resume", (float)width / 2.0f, pauseY - 100.0f, 300, 100, [this](){
        this->currentState = GameState::PLAYING;
        glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }, "assets/audio/Menu/MenuEfecto.wav");
    pauseMenu->AddButton("Back to Menu", (float)width / 2.0f, pauseY + 50.0f, 300, 100, [this](){
        this->currentState = GameState::MENU;
        glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }, "assets/audio/Menu/MenuEfecto.wav");

    // Configurar boton de Ayuda
    helpMenu->AddButton("Back", (float)width / 2.0f, (float)height - 100.0f, 300, 100, [this](){
        this->currentState = GameState::MENU;
    }, "assets/audio/Menu/MenuEfecto.wav");

    ResetRun();
}

Game::~Game() {
    delete playerModel;
    delete menu;
    delete gameOverMenu;
    delete pauseMenu;
    delete helpMenu;
    glfwTerminate();
}

bool Game::Init() {
    if (!glfwInit()) return false;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Endless Runner", NULL, NULL);
    if (!window) return false;
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    
    // Inicializar menú
    menu->Init("assets/fonts/Hippopotamus Apocalypse.otf");
    gameOverMenu->Init("assets/fonts/Hippopotamus Apocalypse.otf");
    pauseMenu->Init("assets/fonts/Hippopotamus Apocalypse.otf");
    helpMenu->Init("assets/fonts/Hippopotamus Apocalypse.otf");

    // Cargar modelo del jugador
    playerModel = new Model("assets/models/run_forrest/scene.gltf");

    // Ajustar hitbox del jugador usando el AABB del modelo (calculado en Model via Assimp).
    // Si la carga falla, el AABB cae al fallback unitario del Model y la hitbox
    // resultante no sería representativa. En ese caso conservamos la provisional.
    const ModelAABB loadedAABB = playerModel->GetAABB();
    if (loadedAABB.min.x < loadedAABB.max.x && loadedAABB.min.y < loadedAABB.max.y) {
        player.SetHitboxFromModelAABB(loadedAABB);
        std::cout << "Player AABB loaded: min=(" << loadedAABB.min.x << "," << loadedAABB.min.y << "," << loadedAABB.min.z
                  << ") max=(" << loadedAABB.max.x << "," << loadedAABB.max.y << "," << loadedAABB.max.z << ")" << std::endl;
    } else {
        std::cerr << "WARN: player model AABB invalid, keeping provisional hitbox." << std::endl;
    }

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
    std::string vertexCode = ReadFile("assets/shaders/shader.vert");
    std::string fragmentCode = ReadFile("assets/shaders/shader.frag");
    const char* vSrc = vertexCode.c_str();
    const char* fSrc = fragmentCode.c_str();
    unsigned int vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &vSrc, NULL);
    glCompileShader(vShader);
    unsigned int fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &fSrc, NULL);
    glCompileShader(fShader);
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vShader);
    glAttachShader(shaderProgram, fShader);
    glLinkProgram(shaderProgram);

    // Compilar Menu Shader
    std::string menuVCode = ReadFile("assets/shaders/menu.vert");
    std::string menuFCode = ReadFile("assets/shaders/menu.frag");
    const char* menuVSrc = menuVCode.c_str();
    const char* menuFSrc = menuFCode.c_str();
    unsigned int vMenuShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vMenuShader, 1, &menuVSrc, NULL);
    glCompileShader(vMenuShader);
    unsigned int fMenuShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fMenuShader, 1, &menuFSrc, NULL);
    glCompileShader(fMenuShader);
    menuShaderProgram = glCreateProgram();
    glAttachShader(menuShaderProgram, vMenuShader);
    glAttachShader(menuShaderProgram, fMenuShader);
    glLinkProgram(menuShaderProgram);

    // VAO del Jugador (y otros objetos sin textura)
    float verticesNoTex[] = {
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
         0.1f, -0.1f, -0.1f, 1.0f,  0.0f,  0.0f,
         0.1f,  0.1f, -0.1f, 1.0f,  0.0f,  0.0f,
         0.1f,  0.1f,  0.1f, 1.0f,  0.0f,  0.0f,
         0.1f, -0.1f,  0.1f, 1.0f,  0.0f,  0.0f
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesNoTex), verticesNoTex, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // VAO del Suelo (con textura)
    float verticesTex[] = {
        // positions          // normals            // texCoords
        -0.1f, -0.1f,  0.1f,  0.0f,  0.0f,  1.0f,   0.0f, 0.0f,
         0.1f, -0.1f,  0.1f,  0.0f,  0.0f,  1.0f,   1.0f, 0.0f,
         0.1f,  0.1f,  0.1f,  0.0f,  0.0f,  1.0f,   1.0f, 1.0f,
        -0.1f,  0.1f,  0.1f,  0.0f,  0.0f,  1.0f,   0.0f, 1.0f,
        -0.1f, -0.1f, -0.1f,  0.0f,  0.0f, -1.0f,   0.0f, 0.0f,
        -0.1f,  0.1f, -0.1f,  0.0f,  0.0f, -1.0f,   0.0f, 1.0f,
         0.1f,  0.1f, -0.1f,  0.0f,  0.0f, -1.0f,   1.0f, 1.0f,
         0.1f, -0.1f, -0.1f,  0.0f,  0.0f, -1.0f,   1.0f, 0.0f,
        -0.1f,  0.1f, -0.1f,  0.0f,  1.0f,  0.0f,   0.0f, 0.0f,
        -0.1f,  0.1f,  0.1f,  0.0f,  1.0f,  0.0f,   0.0f, 1.0f,
         0.1f,  0.1f,  0.1f,  0.0f,  1.0f,  0.0f,   1.0f, 1.0f,
         0.1f,  0.1f, -0.1f,  0.0f,  1.0f,  0.0f,   1.0f, 0.0f,
        -0.1f, -0.1f, -0.1f,  0.0f, -1.0f,  0.0f,   0.0f, 0.0f,
         0.1f, -0.1f, -0.1f,  0.0f, -1.0f,  0.0f,   1.0f, 0.0f,
         0.1f, -0.1f,  0.1f,  0.0f, -1.0f,  0.0f,   1.0f, 1.0f,
        -0.1f, -0.1f,  0.1f,  0.0f, -1.0f,  0.0f,   0.0f, 1.0f,
        -0.1f, -0.1f, -0.1f, -1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
        -0.1f, -0.1f,  0.1f, -1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
        -0.1f,  0.1f,  0.1f, -1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
        -0.1f,  0.1f, -0.1f, -1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
         0.1f, -0.1f, -0.1f, 1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
         0.1f,  0.1f, -0.1f, 1.0f,  0.0f,  0.0f,   0.0f, 1.0f,
         0.1f,  0.1f,  0.1f, 1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
         0.1f, -0.1f,  0.1f, 1.0f,  0.0f,  0.0f,   1.0f, 0.0f
    };
    glGenVertexArrays(1, &groundVAO);
    glGenBuffers(1, &groundVBO);
    glGenBuffers(1, &groundEBO);
    glBindVertexArray(groundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, groundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verticesTex), verticesTex, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, groundEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // TexCoord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);


    
    return true;
}

void Game::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (instance && action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            if (instance->currentState == GameState::MENU) {
                glfwSetWindowShouldClose(window, true);
            } else if (instance->currentState == GameState::PLAYING) {
                instance->currentState = GameState::PAUSED;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            } else if (instance->currentState == GameState::PAUSED) {
                instance->currentState = GameState::PLAYING;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            } else if (instance->currentState == GameState::HELP) {
                instance->currentState = GameState::MENU;
            }
            // GAME_OVER: ESC no hace nada
        }
        
        if (instance->currentState == GameState::MENU) {
            instance->menu->HandleKeyEvent(key);
        } else if (instance->currentState == GameState::GAME_OVER) {
            instance->gameOverMenu->HandleKeyEvent(key);
        } else if (instance->currentState == GameState::PLAYING) {
            if (key == GLFW_KEY_R && instance->player.HasCrashed()) {
                instance->ResetRun();
                return;
            }

            if (key == GLFW_KEY_A || key == GLFW_KEY_LEFT) instance->player.MoveLeft();
            if (key == GLFW_KEY_D || key == GLFW_KEY_RIGHT) instance->player.MoveRight();
            if (key == GLFW_KEY_SPACE || key == GLFW_KEY_W) instance->player.Jump();
            if (key == GLFW_KEY_S) instance->player.FastFall();
        } else if (instance->currentState == GameState::PAUSED) {
            instance->pauseMenu->HandleKeyEvent(key);
        } else if (instance->currentState == GameState::HELP) {
            instance->helpMenu->HandleKeyEvent(key);
        }
    }
}

void Game::CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (!instance) return;
    if (instance->currentState == GameState::MENU) {
        instance->menu->SetMousePos(xpos, ypos);
    } else if (instance->currentState == GameState::GAME_OVER) {
        instance->gameOverMenu->SetMousePos(xpos, ypos);
    } else if (instance->currentState == GameState::PAUSED) {
        instance->pauseMenu->SetMousePos(xpos, ypos);
    } else if (instance->currentState == GameState::HELP) {
        instance->helpMenu->SetMousePos(xpos, ypos);
    }
}

void Game::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (!instance || action != GLFW_PRESS) return;
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    if (instance->currentState == GameState::MENU) {
        instance->menu->HandleClick(xpos, ypos);
    } else if (instance->currentState == GameState::GAME_OVER) {
        instance->gameOverMenu->HandleClick(xpos, ypos);
    } else if (instance->currentState == GameState::PAUSED) {
        instance->pauseMenu->HandleClick(xpos, ypos);
    } else if (instance->currentState == GameState::HELP) {
        instance->helpMenu->HandleClick(xpos, ypos);
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

float Game::GetCurrentSpeed() const {
    // Crecimiento lineal sin tope. La velocidad base se duplica cada
    // kSpeedRampTime segundos y sigue aumentando mientras gameTime crezca.
    float multiplier = 1.0f + gameTime / kSpeedRampTime;
    return kBaseTrainSpeed * multiplier;
}

void Game::Update(float deltaTime) {
    if (prevState != currentState) {
        if (currentState == GameState::MENU) {
            menu->StartAmbient();
        } else if (prevState == GameState::MENU) {
            menu->StopAmbient();
        }
        prevState = currentState;
    }

    if (currentState == GameState::MENU) {
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        menu->Update(deltaTime, fbWidth, fbHeight);
        return;
    }

    if (currentState == GameState::HELP) {
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        helpMenu->Update(deltaTime, fbWidth, fbHeight);
        return;
    }

    if (currentState == GameState::PAUSED) {
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        pauseMenu->Update(deltaTime, fbWidth, fbHeight);
        return;
    }

    if (player.HasCrashed()) {
        if (currentState != GameState::GAME_OVER) {
            currentState = GameState::GAME_OVER;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        gameOverMenu->Update(deltaTime, fbWidth, fbHeight);
        return;
    }

    gameTime += deltaTime;
    float currentSpeed = GetCurrentSpeed();
    groundScroll += currentSpeed * deltaTime;

    UpdateTrains(deltaTime, currentSpeed);
    UpdateGround(deltaTime, currentSpeed);

    std::vector<GameObject*> collisionObjects;
    collisionObjects.reserve(trains.size());
    for (Train& train : trains) {
        collisionObjects.push_back(&train);
    }

    player.Update(deltaTime, collisionObjects);
}

void Game::UpdateTrains(float deltaTime, float currentSpeed) {
    const float playerZ = player.GetPosition().z;

    for (Train& train : trains) {
        train.SetSpeed(currentSpeed);
        train.Update(deltaTime);

        if (train.BackEdgeZ() > playerZ + kTrainDespawnDistance) {
            ResetTrain(train);
        }
    }
}

void Game::UpdateGround(float /*deltaTime*/, float /*currentSpeed*/) {
    // El suelo se desplaza visualmente mediante groundScroll en Render().
    // Los segmentos permanecen en su posicion inicial, evitando saltos visuales.
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
                glm::vec3(kTrainWidth, kTrainHeight, kTrainDepth), kBaseTrainSpeed);
    nextTrainLane = (nextTrainLane + 2) % 3;
}

void Game::ResetRun() {
    player.Reset();
    nextTrainLane = 0;
    gameTime = 0.0f;
    groundScroll = 0.0f;

    const glm::vec3 trainSize(kTrainWidth, kTrainHeight, kTrainDepth);
    const float playerZ = player.GetPosition().z;
    trains = {
        Train(1, playerZ - kTrainSpawnDistance, trainSize, kBaseTrainSpeed),
        Train(0, playerZ - kTrainSpawnDistance - kTrainSpacing, trainSize, kBaseTrainSpeed),
        Train(2, playerZ - kTrainSpawnDistance - kTrainSpacing * 2.0f, trainSize, kBaseTrainSpeed)
    };

    // Inicializar suelo (ancho suficiente para cubrir la vía completa + margen).
    constexpr float kGroundWidth = 8.0f;
    groundSegments.clear();
    for (int i = 0; i < kNumGroundSegments; ++i) {
        float zPos = playerZ - (i * kGroundSegmentLength);
        groundSegments.emplace_back(glm::vec3(0.0f, 0.0f, zPos), glm::vec3(kGroundWidth, 0.1f, kGroundSegmentLength));
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

    if (currentState == GameState::HELP) {
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        helpMenu->Render(menuShaderProgram, VAO, fbWidth, fbHeight);

        helpMenu->RenderText("HELP - CONTROLS", fbWidth / 2.0f, fbHeight / 2.0f - 200.0f, 1.2f, glm::vec3(1.0f, 1.0f, 0.0f), fbWidth, fbHeight);
        helpMenu->RenderText("A / ← : Move left",     fbWidth / 2.0f, fbHeight / 2.0f - 100.0f, 0.7f, glm::vec3(1.0f, 1.0f, 1.0f), fbWidth, fbHeight);
        helpMenu->RenderText("D / → : Move right",       fbWidth / 2.0f, fbHeight / 2.0f - 50.0f,  0.7f, glm::vec3(1.0f, 1.0f, 1.0f), fbWidth, fbHeight);
        helpMenu->RenderText("W / SPACE : Jump",                 fbWidth / 2.0f, fbHeight / 2.0f,           0.7f, glm::vec3(1.0f, 1.0f, 1.0f), fbWidth, fbHeight);
        helpMenu->RenderText("S : Fast fall",            fbWidth / 2.0f, fbHeight / 2.0f + 50.0f,  0.7f, glm::vec3(0.0f, 0.8f, 1.0f), fbWidth, fbHeight);
        helpMenu->RenderText("ESC : Pause",                       fbWidth / 2.0f, fbHeight / 2.0f + 100.0f, 0.7f, glm::vec3(1.0f, 1.0f, 1.0f), fbWidth, fbHeight);

        glfwSwapBuffers(window);
        return;
    }

    if (currentState == GameState::GAME_OVER) {
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        gameOverMenu->Render(menuShaderProgram, VAO, fbWidth, fbHeight);
        
        // Dibujar texto "Perdiste"
        gameOverMenu->RenderText("Game Over", fbWidth / 2.0f, fbHeight / 2.0f - 100.0f, 1.5f, glm::vec3(1.0f, 0.0f, 0.0f), fbWidth, fbHeight);
        
        glfwSwapBuffers(window);
        return;
    }

    glUseProgram(shaderProgram);

    glm::vec3 pos = player.GetPosition();
    glm::vec3 playerSize = player.GetCollisionSize();

    // Cámara que sigue al jugador en X y Z, con vista más cenital que
    // permite ver la vía completa. El target se eleva ligeramente para
    // centrar la escena en la pantalla en vez de en el suelo.
    glm::vec3 cameraTarget(pos.x, 0.3f, pos.z);
    glm::vec3 cameraPos = cameraTarget + glm::vec3(0.0f, 2.5f, 6.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    float aspect = (float)fbWidth / (float)fbHeight;
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 200.0f);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 5.0f, 10.0f, 5.0f);
    glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);
    glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);

    // Cubo base: vértices en ±0.1, por lo que un factor 5*size lo lleva al
    // tamaño exacto de la hitbox (visual == hitbox).
    constexpr float kCubeScaleFactor = 5.0f;

    // --- Jugador ---
    if (playerModel) {
        glUniform1i(glGetUniformLocation(shaderProgram, "isAnimated"), 1);
        // Para que los pies del modelo caigan en world y = pos.y, hay que
        // colocar el origen local del asset de forma que su punto más bajo
        // (AABB.min.y en espacio del asset) quede en pos.y tras la escala.
        //   model_feet_y = translateY + (AABB.min.y * kPlayerScale)
        //   pos.y = model_feet_y  =>  translateY = pos.y - AABB.min.y * scale
        const ModelAABB& modelAABB = playerModel->GetAABB();
        const float translateY = pos.y - modelAABB.min.y * Player::kPlayerScale;
        glm::mat4 visualModel = glm::translate(glm::mat4(1.0f),
                                               glm::vec3(pos.x, translateY, pos.z));
        visualModel = glm::rotate(visualModel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        visualModel = glm::scale(visualModel, glm::vec3(Player::kPlayerScale));
        if (player.IsWeakened()) {
            glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 1.0f, 0.6f, 0.2f);
        } else {
            glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 1.0f, 1.0f, 1.0f);
        }
        playerModel->Draw(shaderProgram, visualModel, (float)glfwGetTime());
    } else {
        glUniform1i(glGetUniformLocation(shaderProgram, "isAnimated"), 0);
        // Anclar el cubo a los pies (mismo criterio que la hitbox y el modelo).
        glm::mat4 playerCube = glm::translate(glm::mat4(1.0f),
                                              glm::vec3(pos.x, pos.y + playerSize.y * 0.5f, pos.z));
        playerCube = glm::scale(playerCube, playerSize * kCubeScaleFactor);
        if (player.IsWeakened()) {
            glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 1.0f, 0.6f, 0.2f);
        } else {
            glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 1.0f, 1.0f, 1.0f);
        }
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(playerCube));
        glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }

    // --- Trenes ---
    glUniform1i(glGetUniformLocation(shaderProgram, "isAnimated"), 0);
    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.85f, 0.2f, 0.12f);
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0);
    glBindVertexArray(VAO);
    for (const Train& train : trains) {
        const glm::vec3 tp = train.GetPosition();
        const glm::vec3 ts = train.GetHitboxSize();
        glm::mat4 trainModel = glm::translate(glm::mat4(1.0f), tp);
        trainModel = glm::scale(trainModel, ts * kCubeScaleFactor);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(trainModel));
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }

    // --- Suelo con scroll continuo ---
    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.5f, 0.5f, 0.5f); // Gris para el camino sin textura
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0);
    // glActiveTexture(GL_TEXTURE0);
    // glBindTexture(GL_TEXTURE_2D, groundTexture);
    // glUniform1i(glGetUniformLocation(shaderProgram, "texture_diffuse1"), 0);
    glBindVertexArray(groundVAO);
    float scrollZ = groundScroll - (int)(groundScroll / kGroundSegmentLength) * kGroundSegmentLength;
    for (const auto& segment : groundSegments) {
        const glm::vec3 sp = segment.GetPosition();
        const glm::vec3 ss = segment.GetHitboxSize();
        // La cara superior del segmento queda en y=0 restando media altura.
        glm::mat4 segModel = glm::translate(glm::mat4(1.0f),
                                            glm::vec3(sp.x, sp.y - ss.y * 0.5f, sp.z + scrollZ));
        segModel = glm::scale(segModel, ss * kCubeScaleFactor);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(segModel));
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }

    if (currentState == GameState::PAUSED) {
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        pauseMenu->Render(menuShaderProgram, VAO, fbWidth, fbHeight);
    }
}
