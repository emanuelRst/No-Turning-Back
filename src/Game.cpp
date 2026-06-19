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
constexpr float kBaseTrainSpeed = 20.0f;
constexpr float kSpeedRampTime = 300.0f;
constexpr float kGroundSegmentLength = 20.0f;
constexpr int kNumGroundSegments = 5;
}

Game::Game(int w, int h)
    : window(nullptr), width(w), height(h), shaderProgram(0), VAO(0),
      playerModel(nullptr), gameTime(0.0f), groundScroll(0.0f),
      currentState(GameState::MENU), menu(new Menu()), gameOverMenu(new Menu()), pauseMenu(new Menu()), helpMenu(new Menu()) {
    instance = this;

    float buttonWidth = 250.0f;
    
   int targetButtonWidth = 400;  // Ancho óptimo centrado dentro del monitor
    int targetButtonHeight = 20; // Alto reducido para que quepan los 5 botones
    int fixedX = 953;             // Posición X calculada para centrado perfecto

// Botón 1: Start Game (Y: 620)
    menu->AddButton("Start Game", fixedX, 625, targetButtonWidth, targetButtonHeight, [this](){ 
    this->ResetRun();
    this->currentState = GameState::PLAYING;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }, "assets/audio/Menu/MenuEfecto.wav");

    // Botón 2: Change Characters (Y: 675)
    menu->AddButton("Change Characters", fixedX, 690, targetButtonWidth, targetButtonHeight, [](){ 
        std::cout << "Characters\n"; 
    }, "assets/audio/Menu/MenuEfecto.wav");

    // Botón 3: Help (Y: 730)
    menu->AddButton("Help", fixedX, 775, targetButtonWidth, targetButtonHeight, [this](){
        this->currentState = GameState::HELP;
    }, "assets/audio/Menu/MenuEfecto.wav");

    // Botón 4: Credits (Y: 785)
    menu->AddButton("Credits", fixedX, 840, targetButtonWidth, targetButtonHeight, [](){ 
        std::cout << "Credits\n"; 
    }, "assets/audio/Menu/MenuEfecto.wav");

    // Botón 5: Exit (Y: 840)
    menu->AddButton("Exit", fixedX, 900, targetButtonWidth, targetButtonHeight, [this](){
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
    helpMenu->AddButton("Back", ((float)width / 2.0f) + 567.0f, (float)height + 300.0f, 300, 100, [this](){
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
    menu->Init("assets/fonts/gunmetl.ttf", "assets/textures/Manu/FondoMenu.png");
    gameOverMenu->Init("assets/fonts/gunmetl.ttf", "assets/textures/Manu/FondoMenu.jpg");
    pauseMenu->Init("assets/fonts/gunmetl.ttf", "assets/textures/Manu/FondoMenu.jpg");
    helpMenu->Init("assets/fonts/gunmetl.ttf", "assets/textures/Manu/FondoMenu.png");

    // Cargar modelo del jugador
    playerModel = new Model("assets/models/soldier/Soldier.glb");

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
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Se gestionará según el estado

    // Habilitar el test de profundidad
    glEnable(GL_DEPTH_TEST);

    // Crear FBO para el fondo del menú
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glGenFramebuffers(1, &menuFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, menuFBO);
    glGenTextures(1, &menuFBOTexture);
    glBindTexture(GL_TEXTURE_2D, menuFBOTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, fbWidth, fbHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, menuFBOTexture, 0);
    glGenRenderbuffers(1, &menuFBORBO);
    glBindRenderbuffer(GL_RENDERBUFFER, menuFBORBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fbWidth, fbHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, menuFBORBO);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    menuFBOWidth = fbWidth;
    menuFBOHeight = fbHeight;

    // Quad fullscreen para blur
    float quadVertices[] = {
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f
    };
    glGenVertexArrays(1, &blurVAO);
    glGenBuffers(1, &blurVBO);                              
    glBindVertexArray(blurVAO);
    glBindBuffer(GL_ARRAY_BUFFER, blurVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

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
            ResetRun();
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

    const float playerZ = player.GetPosition().z;
    levelGen.Update(deltaTime, currentSpeed, playerZ, gameTime);
    UpdateGround(deltaTime, currentSpeed);

    std::vector<GameObject*> collisionObjects = levelGen.GetCollisionObjects();
    player.Update(deltaTime, collisionObjects);
}


void Game::UpdateGround(float /*deltaTime*/, float /*currentSpeed*/) {
    // El suelo se desplaza visualmente mediante groundScroll en Render().
    // Los segmentos permanecen en su posicion inicial, evitando saltos visuales.
}

void Game::ResetRun() {
    player.Reset();
    gameTime = 0.0f;
    groundScroll = 0.0f;

    const float playerZ = player.GetPosition().z;
    levelGen.Reset(playerZ);

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

        // Limpiar el buffer de pantalla con color negro en lugar de renderizar el FBO
        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        menu->Render(menuShaderProgram, VAO, fbWidth, fbHeight, true);

        glfwSwapBuffers(window);
        return;
    }

    if (currentState == GameState::HELP) {
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        helpMenu->Render(menuShaderProgram, VAO, fbWidth, fbHeight, true);

       // Eliminamos el "- 50.0f" para que use el centro geométrico exacto
        helpMenu->RenderImage("assets/textures/Manu/wasd.png", fbWidth / 2.0f, fbHeight / 2.0f + 200.0f, 400.0f, 400.0f, fbWidth, fbHeight);
        helpMenu->RenderImage("assets/textures/Manu/simbol1.png", fbWidth / 2.0f, fbHeight / 2.0f - 100.0f, 200.0f, 200.0f, fbWidth, fbHeight);

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

    RenderGameScene();

    if (currentState == GameState::PAUSED) {
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        pauseMenu->Render(menuShaderProgram, VAO, fbWidth, fbHeight, false);
    }
}

void Game::RenderGameScene() {
    glUseProgram(shaderProgram);

    glm::vec3 pos = player.GetPosition();
    glm::vec3 playerSize = player.GetCollisionSize();

    glm::vec3 cameraTarget(pos.x, pos.y, pos.z);
    glm::vec3 cameraPos = cameraTarget + glm::vec3(0.0f, 3.0f, 6.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, glm::vec3(0.0f, 1.0f, 0.0f));

    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    float aspect = (float)fbWidth / (float)fbHeight;
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 100.0f);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 5.0f, 10.0f, 5.0f);
    glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), cameraPos.x, cameraPos.y, cameraPos.z);
    glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 1.0f, 1.0f, 1.0f);

    constexpr float kCubeScaleFactor = 5.0f;

    if (playerModel) {
        glUniform1i(glGetUniformLocation(shaderProgram, "isAnimated"), 1);
        const ModelAABB& meshAABB = playerModel->GetMeshAABB();
        const float scale = player.GetVisualScale();
        const float translateY = pos.y - meshAABB.min.y * scale;
        glm::mat4 visualModel = glm::translate(glm::mat4(1.0f),
                                               glm::vec3(pos.x, translateY, pos.z));
        visualModel = glm::rotate(visualModel, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        visualModel = glm::scale(visualModel, glm::vec3(scale));
        if (player.IsWeakened()) {
            glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 1.0f, 0.6f, 0.2f);
        } else {
            glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 1.0f, 1.0f, 1.0f);
        }
        unsigned int animIndex = 4;
        switch (player.GetAnimState()) {
            case Player::AnimState::Jump: animIndex = 3; break;
            case Player::AnimState::Fall: animIndex = 1; break;
            case Player::AnimState::Hit:  animIndex = 2; break;
            case Player::AnimState::Die:  animIndex = 0; break;
            default:                      animIndex = 4; break;
        }
        playerModel->Draw(shaderProgram, visualModel, (float)glfwGetTime(), animIndex);
    } else {
        glUniform1i(glGetUniformLocation(shaderProgram, "isAnimated"), 0);
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

    // Dibujar Trenes
    glUniform1i(glGetUniformLocation(shaderProgram, "isAnimated"), 0);
    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.85f, 0.2f, 0.12f);
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0);
    glBindVertexArray(VAO);
    for (const Train& train : levelGen.GetTrains()) {
        const glm::vec3 tp = train.GetPosition();
        const glm::vec3 ts = train.GetHitboxSize();
        glm::mat4 trainModel = glm::translate(glm::mat4(1.0f), tp);
        trainModel = glm::scale(trainModel, ts * kCubeScaleFactor);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(trainModel));
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }

    // Dibujar Obstaculos Overhead
    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.2f, 0.8f, 0.2f);
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0);
    for (const auto& obs : levelGen.GetOverheads()) {
        const glm::vec3 op = obs.GetPosition();
        const glm::vec3 os = obs.GetHitboxSize();
        glm::mat4 obsModel = glm::translate(glm::mat4(1.0f), op);
        obsModel = glm::scale(obsModel, os * kCubeScaleFactor);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(obsModel));
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }

    // Dibujar Rampas
    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.2f, 0.2f, 0.8f);
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0);
    for (const auto& ramp : levelGen.GetRamps()) {
        const glm::vec3 rp = ramp.GetPosition();
        const glm::vec3 rs = ramp.GetHitboxSize();
        glm::mat4 rampModel = glm::translate(glm::mat4(1.0f), rp);
        rampModel = glm::scale(rampModel, rs * kCubeScaleFactor);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(rampModel));
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }

    glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.5f, 0.5f, 0.5f);
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0);
    glBindVertexArray(groundVAO);
    float scrollZ = groundScroll - (int)(groundScroll / kGroundSegmentLength) * kGroundSegmentLength;
    for (const auto& segment : groundSegments) {
        const glm::vec3 sp = segment.GetPosition();
        const glm::vec3 ss = segment.GetHitboxSize();
        glm::mat4 segModel = glm::translate(glm::mat4(1.0f),
                                            glm::vec3(sp.x, sp.y - ss.y * 0.5f, sp.z + scrollZ));
        segModel = glm::scale(segModel, ss * kCubeScaleFactor);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(segModel));
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }
}
