#include "Game.h"
#include "Menu.h"
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <utility> // For std::pair
#include <thread>
#include <chrono>
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
constexpr float kBaseTrainSpeed = 20.0f;
constexpr float kSpeedRampTime = 300.0f;
constexpr float kGroundSegmentLength = 20.0f;
constexpr int kNumGroundSegments = 5;
constexpr float kWallHeight = 5.0f;
constexpr float kWallThickness = 0.6f;
}

Game::Game(int w, int h)
    : window(nullptr), width(w), height(h), shaderProgram(0), VAO(0),
      playerModel(nullptr), skyboxModel(nullptr), skyboxShaderProgram(0),
      coinModel(nullptr), trainObstacleModel(nullptr), rampObstacleModel(nullptr), overheadObstacleModel(nullptr),
      gameTime(0.0f), groundScroll(0.0f),
      currentState(GameState::MENU), menu(new Menu()), gameOverMenu(new Menu()), pauseMenu(new Menu()), helpMenu(new Menu()), helpMenuKeys(new Menu()) {
    instance = this;

    float buttonWidth = 250.0f;
    
    int targetButtonWidth = 400;
    int targetButtonHeight = 20;
    int fixedX = 953;

    // Botón 1: Start Game (Y: 620)
    menu->AddButton("Start Game", fixedX, 625, targetButtonWidth, targetButtonHeight, [this](){ 
        this->ResetRun();
        this->currentState = GameState::PLAYING;
        this->gameStartTimer = 4.0f;
        this->animStateStartTime = glfwGetTime();
        this->lastAnimState = Player::AnimState::Dance;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }, "assets/audio/Menu/Voicy_Obtain.wav");

    // Botón 2: Change Characters (Y: 675)
    menu->AddButton("Change Characters", fixedX, 690, targetButtonWidth, targetButtonHeight, [this](){ 
        this->focusedSlot = 0;
        this->charSelectTime = 0.0f;
        this->currentState = GameState::CHARACTER_SELECT;
    }, "assets/audio/Menu/Voicy_Obtain.wav");

    // Botón 3: Help (Y: 730)
    menu->AddButton("Help", fixedX, 775, targetButtonWidth, targetButtonHeight, [this](){
        this->currentState = GameState::HELP;
    }, "assets/audio/Menu/Voicy_Obtain.wav");

    // Botón 4: Credits (Y: 785)
    menu->AddButton("Credits", fixedX, 840, targetButtonWidth, targetButtonHeight, [](){ 
        std::cout << "Credits\n"; 
    }, "assets/audio/Menu/Voicy_Obtain.wav");

    // Botón 5: Exit (Y: 840)
    menu->AddButton("Exit", fixedX, 900, targetButtonWidth, targetButtonHeight, [this](){
        glfwSetWindowShouldClose(this->window, true);
    }, "assets/audio/Menu/Voicy_Obtain.wav");

    // Configurar botones de Game Over (uno al lado del otro, más abajo)
    float gameOverY = 700.0f;
    float btnGap = 400.0f;
    gameOverMenu->AddButton("Restart", (float)width / 2.0f + btnGap / 2.0f, gameOverY, 300, 100, [this](){
        this->ResetRun();
        this->currentState = GameState::PLAYING;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }, "assets/audio/Menu/Voicy_Obtain.wav");
    gameOverMenu->AddButton("Back to Menu", (float)width / 2.0f + btnGap + 400.0f, gameOverY, 300, 100, [this](){
        this->currentState = GameState::MENU;
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }, "assets/audio/Menu/Voicy_Obtain.wav");

    // Configurar botones de Pausa
    float pauseY = (float)height / 2.0f;
    pauseMenu->AddButton("Resume", (float)width / 2.0f, pauseY - 100.0f, 300, 100, [this](){
        this->currentState = GameState::PLAYING;
        glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }, "assets/audio/Menu/Voicy_Obtain.wav");
    pauseMenu->AddButton("Back to Menu", (float)width / 2.0f, pauseY + 50.0f, 300, 100, [this](){
        this->currentState = GameState::MENU;
        glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }, "assets/audio/Menu/Voicy_Obtain.wav");

    // Configurar boton de Ayuda
    helpMenu->AddButton("Back", ((float)width / 2.0f) + 567.0f, (float)height + 300.0f, 300, 100, [this](){
        this->currentState = GameState::MENU;
    }, "assets/audio/Menu/Voicy_Obtain.wav");
    
    ResetRun();
}

Game::~Game() {
    SaveProgress();
    // characters es dueño de los modelos; playerModel solo apunta a uno de ellos
    for (auto& c : characters) {
        delete c.model;
    }
    playerModel = nullptr;
    delete coinModel;
    delete skyboxModel;
    delete trainObstacleModel;
    delete rampObstacleModel;
    delete overheadObstacleModel;
    delete menu;
    delete gameOverMenu;
    delete pauseMenu;
    delete helpMenu;
    delete helpMenuKeys;
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
    glfwSwapInterval(1); // Enable VSync
    
    // Inicializar menú
    // Inicializar menú
    menu->Init("assets/fonts/gunmetl.ttf", "assets/textures/Menu/FondoMenu.png");
    gameOverMenu->Init("assets/fonts/gunmetl.ttf", "assets/textures/Menu/FondoMenu.png");
    pauseMenu->Init("assets/fonts/gunmetl.ttf", "assets/textures/Menu/FondoMenu.png");
    helpMenu->Init("assets/fonts/gunmetl.ttf", "assets/textures/Menu/FondoMenu.png");
    helpMenuKeys->Init("assets/fonts/DirtyWar.otf", "assets/textures/Menu/FondoMenu.png");
    
    // Pre-cargar imágenes para evitar lag al renderizar
    helpMenu->LoadImage("assets/textures/Menu/wasd.png");
    helpMenu->LoadImage("assets/textures/Menu/simbol1.png");
    menu->LoadImage("assets/textures/Menu/NO-TURNING-BACK.png");

    // Cargar modelos de personajes seleccionables
    // characters es dueño de los modelos; playerModel solo apunta al actual
    characters.push_back({"Thug", "assets/models/thug/tung.glb", new Model("assets/models/thug/tung.glb")});
    characters.push_back({"Alien", "assets/models/alien/alien.glb", new Model("assets/models/alien/alien.glb")});
    characters.push_back({"Teto", "assets/models/Teto/Teto.glb", new Model("assets/models/Teto/Teto.glb")});
    characterUnlocked.assign(characters.size(), false);
    characterUnlocked[0] = true; // Thug siempre desbloqueado
    LoadProgress();
    playerModel = characters[selectedModelIndex].model;

    // Ajustar hitbox del jugador usando el AABB en espacio del mesh (vértices crudos).
    // Para modelos skinned los transforms de nodo se cancelan vía huesos en bind pose,
    // por lo que el meshAABB refleja el tamaño real del modelo renderizado.
    const ModelAABB loadedMeshAABB = playerModel->GetMeshAABB();
    if (loadedMeshAABB.min.x < loadedMeshAABB.max.x && loadedMeshAABB.min.y < loadedMeshAABB.max.y) {
        player.SetHitboxFromModelAABB(loadedMeshAABB);
        std::cout << "Player mesh AABB: min=(" << loadedMeshAABB.min.x << "," << loadedMeshAABB.min.y << "," << loadedMeshAABB.min.z
                  << ") max=(" << loadedMeshAABB.max.x << "," << loadedMeshAABB.max.y << "," << loadedMeshAABB.max.z << ")" << std::endl;
    } else {
        std::cerr << "WARN: player mesh AABB invalid, keeping provisional hitbox." << std::endl;
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
    glfwSwapInterval(1);
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

    // Compilar Skybox Shader
    {
        std::string skyboxVCode = ReadFile("assets/shaders/skybox.vert");
        std::string skyboxFCode = ReadFile("assets/shaders/skybox.frag");
        const char* skyboxVSrc = skyboxVCode.c_str();
        const char* skyboxFSrc = skyboxFCode.c_str();
        unsigned int vSkybox = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vSkybox, 1, &skyboxVSrc, NULL);
        glCompileShader(vSkybox);
        unsigned int fSkybox = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fSkybox, 1, &skyboxFSrc, NULL);
        glCompileShader(fSkybox);
        skyboxShaderProgram = glCreateProgram();
        glAttachShader(skyboxShaderProgram, vSkybox);
        glAttachShader(skyboxShaderProgram, fSkybox);
        glLinkProgram(skyboxShaderProgram);
    }

    // Cargar modelo de moneda
    coinModel = new Model("assets/scene/coin.glb");

    // Cargar modelo del skybox
    skyboxModel = new Model("assets/models/skybox/skyboxGalax.glb");

    // Cargar modelos de obstáculos
    trainObstacleModel = new Model("assets/models/Obstacles/destroyedBus.glb");
    rampObstacleModel = new Model("assets/models/Obstacles/OldCar.glb");
    overheadObstacleModel = new Model("assets/models/Obstacles/roadSign.glb");

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
            if (instance->currentState == GameState::CHARACTER_SELECT) {
                instance->currentState = GameState::MENU;
            } else if (instance->currentState == GameState::MENU) {
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
        
        if (instance->currentState == GameState::CHARACTER_SELECT) {
            int numChars = (int)instance->characters.size();
            // focusedSlot: 0..numChars-1 = characters, numChars = Back button
            if (key == GLFW_KEY_RIGHT || key == GLFW_KEY_D) {
                if (instance->focusedSlot < numChars) {
                    instance->focusedSlot = (instance->focusedSlot + 1) % numChars;
                }
            } else if (key == GLFW_KEY_LEFT || key == GLFW_KEY_A) {
                if (instance->focusedSlot < numChars) {
                    instance->focusedSlot = (instance->focusedSlot - 1 + numChars) % numChars;
                }
            } else if (key == GLFW_KEY_DOWN || key == GLFW_KEY_S) {
                instance->focusedSlot = numChars; // Back button
            } else if (key == GLFW_KEY_UP || key == GLFW_KEY_W) {
                if (instance->focusedSlot == numChars) {
                    instance->focusedSlot = 0;
                }
            } else if (key == GLFW_KEY_ENTER) {
                if (instance->focusedSlot == numChars) {
                    instance->currentState = GameState::MENU;
                } else if (instance->focusedSlot < numChars) {
                    if (instance->characterUnlocked[instance->focusedSlot]) {
                        instance->selectedModelIndex = instance->focusedSlot;
                        instance->playerModel = instance->characters[instance->focusedSlot].model;
                        const ModelAABB loadedMeshAABB = instance->playerModel->GetMeshAABB();
                        if (loadedMeshAABB.min.x < loadedMeshAABB.max.x && loadedMeshAABB.min.y < loadedMeshAABB.max.y) {
                            instance->player.SetHitboxFromModelAABB(loadedMeshAABB);
                        }
                        instance->currentState = GameState::MENU;
                    } else if (instance->totalCoins >= 100) {
                        instance->characterUnlocked[instance->focusedSlot] = true;
                        instance->totalCoins -= 100;
                        instance->SaveProgress();
                    }
                }
            }
        } else if (instance->currentState == GameState::MENU) {
            instance->menu->HandleKeyEvent(key);
        } else if (instance->currentState == GameState::GAME_OVER) {
            instance->gameOverMenu->HandleKeyEvent(key);
        } else if (instance->currentState == GameState::PLAYING) {
            if (key == GLFW_KEY_R && instance->player.HasCrashed()) {
                instance->ResetRun();
                instance->currentState = GameState::PLAYING;
                glfwSetInputMode(instance->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                return;
            }

            if (key == GLFW_KEY_A || key == GLFW_KEY_LEFT) instance->player.MoveLeft();
            if (key == GLFW_KEY_D || key == GLFW_KEY_RIGHT) instance->player.MoveRight();
            if (key == GLFW_KEY_SPACE || key == GLFW_KEY_W) instance->player.Jump();
            if (key == GLFW_KEY_S || key == GLFW_KEY_DOWN) instance->player.InputS();
        } else if (instance->currentState == GameState::PAUSED) {
            instance->pauseMenu->HandleKeyEvent(key);
        } else if (instance->currentState == GameState::HELP) {
            instance->helpMenu->HandleKeyEvent(key);
        }
    }
}

void Game::CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    if (!instance) return;
    if (instance->currentState == GameState::CHARACTER_SELECT) {
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        int numChars = (int)instance->characters.size();
        float centerY = (float)fbHeight * 0.45f;
        float charWidth = (float)fbWidth / (numChars + 1);
        float startX = charWidth;
        bool hoveredAny = false;
        for (int i = 0; i < numChars; i++) {
            float cx = startX + i * charWidth;
            float halfW = charWidth * 0.3f;
            float halfH = 150.0f;
            if (xpos >= cx - halfW && xpos <= cx + halfW &&
                ypos >= centerY - halfH && ypos <= centerY + halfH) {
                instance->focusedSlot = i;
                hoveredAny = true;
                break;
            }
        }
        if (!hoveredAny) {
            // Check Back button
            float backX = (float)fbWidth / 2.0f;
            float backY = (float)fbHeight * 0.85f;
            float halfW = 150.0f;
            float halfH = 40.0f;
            if (xpos >= backX - halfW && xpos <= backX + halfW &&
                ypos >= backY - halfH && ypos <= backY + halfH) {
                instance->focusedSlot = numChars;
                instance->charSelectBackHovered = true;
            } else {
                instance->charSelectBackHovered = false;
            }
        } else {
            instance->charSelectBackHovered = false;
        }
    } else if (instance->currentState == GameState::MENU) {
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
    if (instance->currentState == GameState::CHARACTER_SELECT) {
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        int numChars = (int)instance->characters.size();
        float centerY = (float)fbHeight * 0.45f;
        float charWidth = (float)fbWidth / (numChars + 1);
        float startX = charWidth;
        for (int i = 0; i < numChars; i++) {
            float cx = startX + i * charWidth;
            float halfW = charWidth * 0.3f;
            float halfH = 150.0f;
            if (xpos >= cx - halfW && xpos <= cx + halfW &&
                ypos >= centerY - halfH && ypos <= centerY + halfH) {
                if (instance->characterUnlocked[i]) {
                    instance->selectedModelIndex = i;
                    instance->playerModel = instance->characters[i].model;
                    const ModelAABB loadedMeshAABB = instance->playerModel->GetMeshAABB();
                    if (loadedMeshAABB.min.x < loadedMeshAABB.max.x && loadedMeshAABB.min.y < loadedMeshAABB.max.y) {
                        instance->player.SetHitboxFromModelAABB(loadedMeshAABB);
                    }
                    instance->currentState = GameState::MENU;
                } else if (instance->totalCoins >= 100) {
                    instance->characterUnlocked[i] = true;
                    instance->totalCoins -= 100;
                    instance->SaveProgress();
                }
                return;
            }
        }
        // Check Back button
        float backX = (float)fbWidth / 2.0f;
        float backY = (float)fbHeight * 0.85f;
        float halfW = 150.0f;
        float halfH = 40.0f;
        if (xpos >= backX - halfW && xpos <= backX + halfW &&
            ypos >= backY - halfH && ypos <= backY + halfH) {
            instance->currentState = GameState::MENU;
            return;
        }
    } else if (instance->currentState == GameState::MENU) {
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
    const double kTargetFrameTime = 1.0 / 60.0;
    float lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        Update(deltaTime);
        Render();

        glfwSwapBuffers(window);
        glfwPollEvents();

        // Frame limiter de respaldo (si VSync falla)
        double elapsed = glfwGetTime() - currentTime;
        if (elapsed < kTargetFrameTime) {
            double sleepMs = (kTargetFrameTime - elapsed) * 1000.0;
            std::this_thread::sleep_for(std::chrono::milliseconds((int)sleepMs));
        }
    }
}

float Game::GetCurrentSpeed() const {
    // Crecimiento lineal sin tope. La velocidad base se duplica cada
    // kSpeedRampTime segundos y sigue aumentando mientras gameTime crezca.
    float multiplier = std::min(1.0f + gameTime / kSpeedRampTime, 2.5f);
    return kBaseTrainSpeed * multiplier;
}

void Game::Update(float deltaTime) {
    if (prevState != currentState) {
        if (currentState == GameState::MENU) {
            ResetRun();
            menu->StartAmbient();
        } else if (prevState == GameState::MENU) {
            menu->StopAmbient();
        }
        prevState = currentState;
    }

    if (currentState == GameState::CHARACTER_SELECT) {
        if (focusedSlot != lastFocusedSlot) {
            charFocusStartTime = charSelectTime;
            lastFocusedSlot = focusedSlot;
        }
        charSelectTime += deltaTime;
        // Smooth scale for back button
        float smoothSpeed = 10.0f;
        float targetScale = charSelectBackHovered ? 1.2f : 1.0f;
        charSelectBackScale += (targetScale - charSelectBackScale) * smoothSpeed * deltaTime;
        return;
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

    if (currentState == GameState::PLAYING) {
        // Si estamos en la fase inicial de emote, no actualizar gameplay
        if (gameStartTimer > 0.0f) {
            gameStartTimer -= deltaTime;
            if (gameStartTimer <= 0.0f) {
                gameStartTimer = 0.0f;
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            }
            return;
        }
    }

    if (player.HasCrashed()) {
        if (currentState != GameState::GAME_OVER) {
            currentState = GameState::GAME_OVER;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            SaveProgress();
        }
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        gameOverMenu->Update(deltaTime, fbWidth, fbHeight);
        return;
    }

    gameTime += deltaTime;
    float currentSpeed = GetCurrentSpeed();
    groundScroll += currentSpeed * deltaTime;

    const float playerZ = player.GetPosition().z;
    levelGen.Update(deltaTime, currentSpeed, playerZ, gameTime);
    UpdateGround(deltaTime, currentSpeed);

    const auto& collisionObjects = levelGen.GetCollisionObjects();
    player.Update(deltaTime, collisionObjects);

    // Score por distancia
    score += currentSpeed * deltaTime;

    // Colision con monedas
    Bounds playerBounds = player.GetBounds();
    for (Coin& coin : levelGen.GetCoins()) {
        if (!coin.IsCollected()) {
            Bounds coinBounds = coin.GetBounds();
            if (BoundsIntersect(playerBounds, coinBounds)) {
                coin.Collect();
                runCoins++;
                totalCoins++;
            }
        }
    }
}


void Game::UpdateGround(float /*deltaTime*/, float /*currentSpeed*/) {
    // El suelo se desplaza visualmente mediante groundScroll en Render().
    // Los segmentos permanecen en su posicion inicial, evitando saltos visuales.
}

void Game::ResetRun() {
    player.Reset();
    gameTime = 0.0f;
    gameStartTimer = 0.0f;
    groundScroll = 0.0f;
    runCoins = 0;
    score = 0.0f;

    const float playerZ = player.GetPosition().z;
    levelGen.Reset(playerZ);

    // Inicializar suelo (ancho suficiente para cubrir la vía completa + margen).
    constexpr float kGroundWidth = 8.0f;
    groundSegments.clear();
    for (int i = 0; i < kNumGroundSegments; ++i) {
        float zPos = playerZ - (i * kGroundSegmentLength);
        groundSegments.emplace_back(glm::vec3(0.0f, 0.0f, zPos), glm::vec3(kGroundWidth, 0.1f, kGroundSegmentLength));
    }

    wallSegments.clear();
    for (int i = 0; i < kNumGroundSegments; ++i) {
        float zPos = playerZ - (i * kGroundSegmentLength);
        wallSegments.emplace_back(
            glm::vec3(-kGroundWidth / 2.0f - kWallThickness / 2.0f, kWallHeight, zPos),
            glm::vec3(kWallThickness, kWallHeight, kGroundSegmentLength));
        wallSegments.emplace_back(
            glm::vec3(kGroundWidth / 2.0f + kWallThickness / 2.0f, kWallHeight, zPos),
            glm::vec3(kWallThickness, kWallHeight, kGroundSegmentLength));
    }
    SaveProgress();
}

static void RenderMixedText(Menu* normal, Menu* keyFont,
    const std::vector<std::pair<std::string, bool>>& segments,
    float baseX, float y, float scale, glm::vec3 color,
    int fbWidth, int fbHeight)
{
    float totalWidth = 0;
    for (auto& seg : segments) {
        Menu* m = seg.second ? keyFont : normal;
        totalWidth += m->GetTextWidth(seg.first, scale);
    }
    float cx = baseX - totalWidth / 2.0f;
    for (auto& seg : segments) {
        Menu* m = seg.second ? keyFont : normal;
        float w = m->GetTextWidth(seg.first, scale);
        m->RenderText(seg.first, cx + w / 2.0f, y, scale, color, fbWidth, fbHeight);
        cx += w;
    }
}

void Game::Render() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (currentState == GameState::CHARACTER_SELECT) {
        RenderCharacterSelect();
        glfwSwapBuffers(window);
        return;
    }

    if (currentState == GameState::MENU) {
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

        // Limpiar el buffer de pantalla con color negro en lugar de renderizar el FBO
        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        menu->Render(menuShaderProgram, VAO, fbWidth, fbHeight, true);
        menu->RenderImage("assets/textures/Menu/NO-TURNING-BACK.png", fbWidth * 0.5f, fbHeight * 0.25f, fbWidth * 0.8f, fbHeight * 0.30f, fbWidth, fbHeight);

        glfwSwapBuffers(window);
        return;
    }

    if (currentState == GameState::HELP) {
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

        // Limpiar buffer y configurar viewport para renderizado limpio
        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        helpMenu->Render(menuShaderProgram, VAO, fbWidth, fbHeight, true);
        helpMenu->RenderImage("assets/textures/Menu/wasd.png", fbWidth / 2.0f, fbHeight / 2.0f + 200.0f, 400.0f, 400.0f, fbWidth, fbHeight);
        helpMenu->RenderImage("assets/textures/Menu/simbol1.png", fbWidth / 2.0f - 500.0f, fbHeight / 2.0f - 250.0f, 500.0f, 300.0f, fbWidth, fbHeight);

        glfwSwapBuffers(window);
        return;
    }

    if (currentState == GameState::GAME_OVER) {
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        gameOverMenu->Render(menuShaderProgram, VAO, fbWidth, fbHeight, false);
        
        // Dibujar texto "Perdiste"
        gameOverMenu->RenderText("Game Over", fbWidth / 2.0f, fbHeight / 2.0f - 100.0f, 1.5f, glm::vec3(1.0f, 0.0f, 0.0f), fbWidth, fbHeight);
        
        glfwSwapBuffers(window);
        return;
    }

    if (currentState == GameState::PAUSED) {
        RenderHUD();
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        pauseMenu->Render(menuShaderProgram, VAO, fbWidth, fbHeight, false);
        glfwSwapBuffers(window);
        return;
    }

    RenderGameScene();

    RenderHUD();
}

void Game::RenderGameScene() {
    glm::vec3 pos = player.GetPosition();
    glm::vec3 playerSize = player.GetCollisionSize();

    glm::vec3 cameraTarget(pos.x, pos.y, pos.z);
    glm::vec3 cameraPos = cameraTarget + glm::vec3(0.0f, 3.0f, 6.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    float aspect = (float)fbWidth / (float)fbHeight;
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);

    // --- Skybox: se renderiza primero, detrás de todo ---
    if (skyboxModel && currentState == GameState::PLAYING) {
        glDepthMask(GL_FALSE);
        glUseProgram(skyboxShaderProgram);

        static struct { GLint view, projection; unsigned int lastProgram = 0; } skyUC;
        if (skyboxShaderProgram != skyUC.lastProgram) {
            skyUC.lastProgram = skyboxShaderProgram;
            skyUC.view = glGetUniformLocation(skyboxShaderProgram, "view");
            skyUC.projection = glGetUniformLocation(skyboxShaderProgram, "projection");
        }

        glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
        float rotationAngle = gameTime * 30.0f;
        glm::mat4 skyboxModelMat = glm::rotate(glm::mat4(1.0f), glm::radians(rotationAngle), glm::vec3(1.0f, 0.0f, 0.0f));
        skyboxModelMat = glm::scale(skyboxModelMat, glm::vec3(50.0f));

        glUniformMatrix4fv(skyUC.view, 1, GL_FALSE, glm::value_ptr(skyboxView));
        glUniformMatrix4fv(skyUC.projection, 1, GL_FALSE, glm::value_ptr(projection));

        skyboxModel->Draw(skyboxShaderProgram, skyboxModelMat, 0.0f, "", false);

        glDepthMask(GL_TRUE);
    }

    // --- Escena normal ---
    glUseProgram(shaderProgram);

    struct UniformCache {
        GLint view, projection, lightPos, viewPos, lightColor, isAnimated, objectColor, model, useTexture, normalMatrix;
    };
    static UniformCache uc = {};
    static unsigned int lastProgram = 0;
    if (shaderProgram != lastProgram) {
        lastProgram = shaderProgram;
        uc.view = glGetUniformLocation(shaderProgram, "view");
        uc.projection = glGetUniformLocation(shaderProgram, "projection");
        uc.lightPos = glGetUniformLocation(shaderProgram, "lightPos");
        uc.viewPos = glGetUniformLocation(shaderProgram, "viewPos");
        uc.lightColor = glGetUniformLocation(shaderProgram, "lightColor");
        uc.isAnimated = glGetUniformLocation(shaderProgram, "isAnimated");
        uc.objectColor = glGetUniformLocation(shaderProgram, "objectColor");
        uc.model = glGetUniformLocation(shaderProgram, "model");
        uc.useTexture = glGetUniformLocation(shaderProgram, "useTexture");
        uc.normalMatrix = glGetUniformLocation(shaderProgram, "normalMatrix");
    }

    glUniformMatrix4fv(uc.view, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(uc.projection, 1, GL_FALSE, glm::value_ptr(projection));

    glUniform3f(uc.lightPos, 5.0f, 10.0f, 5.0f);
    glUniform3f(uc.viewPos, cameraPos.x, cameraPos.y, cameraPos.z);
    glUniform3f(uc.lightColor, 1.0f, 1.0f, 1.0f);

    constexpr float kCubeScaleFactor = 5.0f;

    if (playerModel) {
        glUniform1i(uc.isAnimated, 1);
        const ModelAABB& meshAABB = playerModel->GetMeshAABB();
        const float scale = player.GetVisualScale();
        const float translateY = pos.y - meshAABB.min.y * scale;
        glm::mat4 visualModel = glm::translate(glm::mat4(1.0f),
                                               glm::vec3(pos.x, translateY, pos.z));
        visualModel = glm::rotate(visualModel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        visualModel = glm::scale(visualModel, glm::vec3(scale));
        {
            glm::mat3 normalM = glm::transpose(glm::inverse(glm::mat3(visualModel)));
            glUniformMatrix3fv(uc.normalMatrix, 1, GL_FALSE, glm::value_ptr(normalM));
        }
        if (player.IsWeakened()) {
            glUniform3f(uc.objectColor, 1.0f, 0.6f, 0.2f);
        } else {
            glUniform3f(uc.objectColor, 1.0f, 1.0f, 1.0f);
        }
        std::string animName = "Run";
        bool loop = true;
        Player::AnimState animState = player.GetAnimState();

        // Sobreescribir animacion para Dance si estamos en emote inicial o pausa
        if (gameStartTimer > 0.0f || currentState == GameState::PAUSED) {
            animState = Player::AnimState::Dance;
        }

        switch (animState) {
            case Player::AnimState::Jump: animName = "JumpUp"; break;
            case Player::AnimState::Fall: animName = "JumpDown"; break;
            case Player::AnimState::Hit:  animName = player.GetHitFromLeft() ? "HitOnRight" : "HitOnLeft"; loop = false; break;
            case Player::AnimState::Die:  animName = ""; break;
            case Player::AnimState::Roll: animName = "Roll"; loop = false; break;
            case Player::AnimState::Dance: animName = "Dance"; loop = true; break;
            default:                      animName = "Run"; break;
        }

        // Crossfade: detectar transición de estado
        if (animState != lastAnimState) {
            prevAnimName = lastAnimName;
            prevAnimTime = lastAnimTime;
            prevAnimLoop = lastAnimLoop;
            animStateStartTime = glfwGetTime();
            lastAnimState = animState;
        }

        float animTime = (float)(glfwGetTime() - animStateStartTime);
        if (animState == Player::AnimState::Hit) {
            animTime = 5.0f - player.GetWeakenedTimer();
        } else if (animState == Player::AnimState::Dance && gameStartTimer > 0.0f) {
            animTime = 4.0f - gameStartTimer;
        }

        // Calcular blend factor para crossfade
        float blendFactor = 0.0f;
        float blendPrevTime = 0.0f;
        std::string blendPrevAnim = "";
        bool blendPrevLoop = true;
        if (!prevAnimName.empty() && prevAnimName != animName) {
            float elapsedSinceTransition = (float)(glfwGetTime() - animStateStartTime);
            blendFactor = glm::clamp(elapsedSinceTransition / crossFadeDuration, 0.0f, 1.0f);
            blendPrevTime = prevAnimTime;
            blendPrevAnim = prevAnimName;
            blendPrevLoop = prevAnimLoop;
            if (blendFactor >= 1.0f) {
                prevAnimName = "";
            }
        }
        
        playerModel->Draw(shaderProgram, visualModel, animTime, animName, loop,
                          blendFactor, blendPrevAnim, blendPrevTime, blendPrevLoop);

        // Guardar para el siguiente frame (necesario para crossfade)
        lastAnimName = animName;
        lastAnimTime = animTime;
        lastAnimLoop = loop;
    } else {
        glUniform1i(uc.isAnimated, 0);
        glm::mat4 playerCube = glm::translate(glm::mat4(1.0f),
                                              glm::vec3(pos.x, pos.y + playerSize.y * 0.5f, pos.z));
        playerCube = glm::scale(playerCube, playerSize * kCubeScaleFactor);
        if (player.IsWeakened()) {
            glUniform3f(uc.objectColor, 1.0f, 0.6f, 0.2f);
        } else {
            glUniform3f(uc.objectColor, 1.0f, 1.0f, 1.0f);
        }
        {
            glm::mat3 normalM = glm::transpose(glm::inverse(glm::mat3(playerCube)));
            glUniformMatrix3fv(uc.normalMatrix, 1, GL_FALSE, glm::value_ptr(normalM));
        }
        glUniformMatrix4fv(uc.model, 1, GL_FALSE, glm::value_ptr(playerCube));
        glUniform1i(uc.useTexture, 0);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }

    // Dibujar Monedas
    if (coinModel) {
        glUniform1i(uc.isAnimated, 0);
        glUniform3f(uc.objectColor, 1.0f, 1.0f, 1.0f);
        float coinRot = gameTime * 120.0f;
        float cullZ = pos.z - 80.0f;
        for (Coin& coin : levelGen.GetCoins()) {
            if (coin.IsCollected()) continue;
            const glm::vec3 cp = coin.GetPosition();
            if (cp.z < cullZ) continue;
            glm::mat4 coinMat = glm::translate(glm::mat4(1.0f), cp);
            coinMat = glm::rotate(coinMat, glm::radians(coinRot), glm::vec3(0.0f, 1.0f, 0.0f));
            coinMat = glm::scale(coinMat, glm::vec3(0.8f, 0.6f, 0.4f));
            {
                glm::mat3 normalM = glm::transpose(glm::inverse(glm::mat3(coinMat)));
                glUniformMatrix3fv(uc.normalMatrix, 1, GL_FALSE, glm::value_ptr(normalM));
            }
            coinModel->Draw(shaderProgram, coinMat, 0.0f, "", false);
        }
    }

    // Dibujar Trenes (modelo destroyedBus)
    glUniform1i(uc.isAnimated, 0);
    glUniform3f(uc.objectColor, 1.0f, 1.0f, 1.0f);
    {
        const ModelAABB& aabb = trainObstacleModel->GetMeshAABB();
        const glm::vec3 modelSize = aabb.max - aabb.min;
        const glm::vec3 centerOffset = (aabb.min + aabb.max) * 0.5f;
        for (const Train& train : levelGen.GetTrains()) {
            const glm::vec3 tp = train.GetPosition();
            const glm::vec3 ts = train.GetHitboxSize();
            glm::vec3 scale;
            scale.x = (modelSize.z > 0.0f) ? ts.z / modelSize.x : 1.0f;
            scale.y = (modelSize.y > 0.0f) ? ts.y / modelSize.y : 1.0f;
            scale.z = (modelSize.x > 0.0f) ? ts.x / modelSize.z : 1.0f;
            glm::vec3 rotatedCenter(
                centerOffset.z * scale.z,
                centerOffset.y * scale.y,
                -centerOffset.x * scale.x
            );
            const glm::vec3 adjustedPos = tp - rotatedCenter;
            glm::mat4 modelMat = glm::translate(glm::mat4(1.0f), adjustedPos);
            modelMat = glm::rotate(modelMat, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            modelMat = glm::scale(modelMat, scale);
            {
                glm::mat3 normalM = glm::transpose(glm::inverse(glm::mat3(modelMat)));
                glUniformMatrix3fv(uc.normalMatrix, 1, GL_FALSE, glm::value_ptr(normalM));
            }
            trainObstacleModel->Draw(shaderProgram, modelMat, 0.0f, "", false);
        }
    }

    // Dibujar Obstaculos Overhead (modelo roadSign)
    glUniform1i(uc.isAnimated, 0);
    glUniform3f(uc.objectColor, 1.0f, 1.0f, 1.0f);
    {
        const ModelAABB& aabb = overheadObstacleModel->GetMeshAABB();
        const glm::vec3 modelSize = aabb.max - aabb.min;
        const glm::vec3 centerOffset = (aabb.min + aabb.max) * 0.5f;
        const float uniformScale = (modelSize.x > 0.0f) ? LevelGenerator::kOverheadSizeX / modelSize.x : 1.0f;
        for (const auto& obs : levelGen.GetOverheads()) {
            const glm::vec3 op = obs.GetPosition();
            constexpr float kOverheadVisualYOffset = 0.3f;
            const glm::vec3 visualPos(op.x, op.y - kOverheadVisualYOffset, op.z);
            glm::mat4 modelMat = glm::translate(glm::mat4(1.0f), visualPos - centerOffset * uniformScale);
            modelMat = glm::rotate(modelMat, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            modelMat = glm::scale(modelMat, glm::vec3(uniformScale));
            {
                glm::mat3 normalM = glm::transpose(glm::inverse(glm::mat3(modelMat)));
                glUniformMatrix3fv(uc.normalMatrix, 1, GL_FALSE, glm::value_ptr(normalM));
            }
            overheadObstacleModel->Draw(shaderProgram, modelMat, 0.0f, "", false);
        }
    }

    // Dibujar Rampas (modelo OldCar)
    glUniform1i(uc.isAnimated, 0);
    glUniform3f(uc.objectColor, 1.0f, 1.0f, 1.0f);
    {
        const ModelAABB& aabb = rampObstacleModel->GetMeshAABB();
        const glm::vec3 modelSize = aabb.max - aabb.min;
        const glm::vec3 centerOffset = (aabb.min + aabb.max) * 0.5f;
        for (const auto& ramp : levelGen.GetRamps()) {
            const glm::vec3 rp = ramp.GetPosition();
            const float uniformScale = (modelSize.x > 0.0f) ? LevelGenerator::kRampWidth / modelSize.x : 1.0f;
            glm::mat4 modelMat = glm::translate(glm::mat4(1.0f), rp - centerOffset * uniformScale);
            modelMat = glm::rotate(modelMat, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            modelMat = glm::scale(modelMat, glm::vec3(uniformScale));
            {
                glm::mat3 normalM = glm::transpose(glm::inverse(glm::mat3(modelMat)));
                glUniformMatrix3fv(uc.normalMatrix, 1, GL_FALSE, glm::value_ptr(normalM));
            }
            rampObstacleModel->Draw(shaderProgram, modelMat, 0.0f, "", false);
        }
    }


    glUniform1i(uc.useTexture, 0);
    glUniform3f(uc.objectColor, 0.5f, 0.5f, 0.5f);
    glBindVertexArray(groundVAO);
    float scrollZ = groundScroll - (int)(groundScroll / kGroundSegmentLength) * kGroundSegmentLength;
    for (const auto& segment : groundSegments) {
        const glm::vec3 sp = segment.GetPosition();
        const glm::vec3 ss = segment.GetHitboxSize();
        glm::mat4 segModel = glm::translate(glm::mat4(1.0f),
                                            glm::vec3(sp.x, sp.y - ss.y * 0.5f, sp.z + scrollZ));
        segModel = glm::scale(segModel, ss * kCubeScaleFactor);
        {
            glm::mat3 normalM = glm::transpose(glm::inverse(glm::mat3(segModel)));
            glUniformMatrix3fv(uc.normalMatrix, 1, GL_FALSE, glm::value_ptr(normalM));
        }
        glUniformMatrix4fv(uc.model, 1, GL_FALSE, glm::value_ptr(segModel));
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }

    // Dibujar Paredes laterales
    glUniform1i(uc.useTexture, 0);
    glUniform3f(uc.objectColor, 0.2f, 0.15f, 0.1f);
    glBindVertexArray(VAO);
    for (const auto& wall : wallSegments) {
        const glm::vec3 wp = wall.GetPosition();
        const glm::vec3 ws = wall.GetHitboxSize();
        glm::mat4 wallModel = glm::translate(glm::mat4(1.0f),
                                            glm::vec3(wp.x, wp.y - ws.y * 0.5f, wp.z + scrollZ));
        wallModel = glm::scale(wallModel, ws * kCubeScaleFactor);
        {
            glm::mat3 normalM = glm::transpose(glm::inverse(glm::mat3(wallModel)));
            glUniformMatrix3fv(uc.normalMatrix, 1, GL_FALSE, glm::value_ptr(normalM));
        }
        glUniformMatrix4fv(uc.model, 1, GL_FALSE, glm::value_ptr(wallModel));
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }
}

void Game::RenderCharacterSelect() {
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

    glViewport(0, 0, fbWidth, fbHeight);
    glClearColor(0.05f, 0.05f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float aspect = (float)fbWidth / (float)fbHeight;
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 1.2f, 4.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    glUseProgram(shaderProgram);

    struct CharSelectUC {
        GLint view, projection, lightPos, viewPos, lightColor, objectColor, isAnimated, useTexture, normalMatrix;
    };
    static CharSelectUC csuc = {};
    static unsigned int lastCSProgram = 0;
    if (shaderProgram != lastCSProgram) {
        lastCSProgram = shaderProgram;
        csuc.view = glGetUniformLocation(shaderProgram, "view");
        csuc.projection = glGetUniformLocation(shaderProgram, "projection");
        csuc.lightPos = glGetUniformLocation(shaderProgram, "lightPos");
        csuc.viewPos = glGetUniformLocation(shaderProgram, "viewPos");
        csuc.lightColor = glGetUniformLocation(shaderProgram, "lightColor");
        csuc.objectColor = glGetUniformLocation(shaderProgram, "objectColor");
        csuc.isAnimated = glGetUniformLocation(shaderProgram, "isAnimated");
        csuc.useTexture = glGetUniformLocation(shaderProgram, "useTexture");
        csuc.normalMatrix = glGetUniformLocation(shaderProgram, "normalMatrix");
    }

    glUniformMatrix4fv(csuc.view, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(csuc.projection, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(csuc.lightPos, 5.0f, 10.0f, 5.0f);
    glUniform3f(csuc.viewPos, 0.0f, 1.2f, 4.0f);
    glUniform3f(csuc.lightColor, 1.0f, 1.0f, 1.0f);
    glUniform3f(csuc.objectColor, 1.0f, 1.0f, 1.0f);
    glUniform1i(csuc.useTexture, 1);

    int numChars = (int)characters.size();

    for (int i = 0; i < numChars; i++) {
        if (!characters[i].model) continue;

        bool isFocused = (focusedSlot == i && focusedSlot < numChars);

        const ModelAABB& meshAABB = characters[i].model->GetMeshAABB();
        glm::vec3 size = meshAABB.max - meshAABB.min;
        float maxExtent = std::max({size.x, size.y, size.z});
        float previewScale = (maxExtent > 0.0f) ? (1.5f / maxExtent) : 1.0f;

        float spacing = 1.8f;
        float totalWidth = (numChars - 1) * spacing;
        float startX = -totalWidth / 2.0f;
        float posX = startX + i * spacing;

        glm::mat4 modelMat = glm::mat4(1.0f);
        modelMat = glm::translate(modelMat, glm::vec3(posX, 0.0f, 0.0f));
        modelMat = glm::translate(modelMat, glm::vec3(0.0f, -meshAABB.min.y * previewScale, 0.0f));

        if (isFocused) {
        } else {
            float rotAngle = charSelectTime * 60.0f;
            modelMat = glm::rotate(modelMat, glm::radians(rotAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        }

        modelMat = glm::scale(modelMat, glm::vec3(previewScale));

        {
            glm::mat3 normalM = glm::transpose(glm::inverse(glm::mat3(modelMat)));
            glUniformMatrix3fv(csuc.normalMatrix, 1, GL_FALSE, glm::value_ptr(normalM));
        }

        if (isFocused) {
            float danceTime = charSelectTime - charFocusStartTime;
            glUniform1i(csuc.isAnimated, 1);
            characters[i].model->Draw(shaderProgram, modelMat, danceTime, "Dance", true);
        } else {
            glUniform1i(csuc.isAnimated, 0);
            characters[i].model->Draw(shaderProgram, modelMat, 0.0f, "", false);
        }
    }

    glDisable(GL_DEPTH_TEST);

    // Render coin balance at top
    menu->RenderText("Coins: " + std::to_string(totalCoins), (float)fbWidth / 2.0f, 30.0f, 0.8f, glm::vec3(1.0f, 0.85f, 0.2f), fbWidth, fbHeight);

    // Render names and status below models
    for (int i = 0; i < numChars; i++) {
        float spacing = 1.8f;
        float totalWidth = (numChars - 1) * spacing;
        float startX = -totalWidth / 2.0f;
        float posX = startX + i * spacing;

        // Project 3D position to screen coordinates
        glm::vec4 clipPos = projection * view * glm::vec4(posX, -0.8f, 0.0f, 1.0f);
        glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
        float screenX = (ndc.x * 0.5f + 0.5f) * fbWidth;
        float screenY = (1.0f - (ndc.y * 0.5f + 0.5f)) * fbHeight;

        bool isFocused = (focusedSlot == i && focusedSlot < numChars);
        glm::vec3 nameColor = isFocused ? glm::vec3(1.0f, 0.8f, 0.2f) : glm::vec3(0.8f, 0.8f, 0.8f);
        menu->RenderText(characters[i].name, screenX, screenY + 30.0f, 0.7f, nameColor, fbWidth, fbHeight);

        if (characterUnlocked[i]) {
            glm::vec3 statusColor = isFocused ? glm::vec3(0.2f, 1.0f, 0.2f) : glm::vec3(0.6f, 0.6f, 0.6f);
            menu->RenderText("SELECT", screenX, screenY + 65.0f, 0.5f, statusColor, fbWidth, fbHeight);
        } else {
            bool canAfford = (totalCoins >= 100);
            glm::vec3 priceColor = canAfford ? glm::vec3(1.0f, 0.85f, 0.2f) : glm::vec3(1.0f, 0.3f, 0.3f);
            menu->RenderText("100 COINS", screenX, screenY + 65.0f, 0.5f, priceColor, fbWidth, fbHeight);
        }
    }

    // Render "Back" button
    {
        bool isBackFocused = (focusedSlot == numChars);
        glm::vec3 backColor = isBackFocused ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(1.0f, 1.0f, 1.0f);
        menu->RenderText("Back", (float)fbWidth / 2.0f, (float)fbHeight * 0.85f, charSelectBackScale, backColor, fbWidth, fbHeight);
    }

    glEnable(GL_DEPTH_TEST);
}

void Game::RenderHUD() {
    if (currentState != GameState::PLAYING && currentState != GameState::PAUSED) return;

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

    int scoreInt = (int)score;
    menu->RenderText("Coins: " + std::to_string(runCoins), 70.0f, 20.0f, 0.6f, glm::vec3(1.0f, 0.85f, 0.2f), fbWidth, fbHeight);
    menu->RenderText("Score: " + std::to_string(scoreInt), 70.0f, 65.0f, 0.6f, glm::vec3(1.0f, 1.0f, 1.0f), fbWidth, fbHeight);
}

void Game::SaveProgress() {
    nlohmann::json j;
    j["totalCoins"] = totalCoins;
    j["selectedModelIndex"] = selectedModelIndex;
    for (size_t i = 0; i < characterUnlocked.size(); ++i) {
        j["unlocked"][i] = characterUnlocked[i];
    }
    std::ofstream file("save.json");
    if (file.is_open()) {
        file << j.dump(4);
    }
}

void Game::LoadProgress() {
    std::ifstream file("save.json");
    if (!file.is_open()) return;
    nlohmann::json j;
    try {
        file >> j;
        if (j.contains("totalCoins")) totalCoins = j["totalCoins"];
        if (j.contains("selectedModelIndex")) {
            int idx = j["selectedModelIndex"];
            if (idx >= 0 && idx < (int)characters.size()) {
                selectedModelIndex = idx;
            }
        }
        if (j.contains("unlocked")) {
            for (size_t i = 0; i < j["unlocked"].size() && i < characterUnlocked.size(); ++i) {
                characterUnlocked[i] = j["unlocked"][i];
            }
        }
    } catch (...) {}
}
