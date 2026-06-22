#ifndef GAME_H
#define GAME_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "GameObject.h"
#include "Player.h"
#include "LevelGenerator.h"
#include "Model.h"
#include "AudioManager.h"

#include <vector>
#include <string>
#include <nlohmann/json.hpp>

// Forward declaration
class Menu;

enum class GameState { MENU, PLAYING, GAME_OVER, PAUSED, HELP, CHARACTER_SELECT, CREDITS };

// Clase sencilla para los segmentos del suelo
class GroundSegment : public GameObject {
public:
    GroundSegment(const glm::vec3& position, const glm::vec3& size) 
        : GameObject(position, size) {}
    void Update(float /*deltaTime*/) override {} // No se mueven solos
};

class Game {
public:
    Game(int width, int height);
    ~Game();

    bool Init();
    void Run();

    // Callback estático para GLFW
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

    float GetCurrentSpeed() const;

private:
    void Update(float deltaTime);
    void Render();
    void UpdateTrains(float deltaTime, float currentSpeed);
    void UpdateGround(float deltaTime, float currentSpeed);
    void ResetTrain(Train& train);
    void ResetRun();
    void RenderGameScene();
    void RenderCharacterSelect();
    void RenderHUD();
    void SaveProgress();
    void LoadProgress();

    GLFWwindow* window;
    int width, height;
    GameState currentState;
    GameState prevState = GameState::PLAYING;
    Menu* menu;
    Menu* gameOverMenu;
    Menu* pauseMenu;
    Menu* helpMenu;
    Menu* helpMenuKeys; // NEW
    Menu* creditsMenu;
    float creditsScroll = 0.0f;
    
    AudioManager audioManager; // Added here properly
    // Shader program y buffers
    unsigned int shaderProgram;
    unsigned int menuShaderProgram;
    unsigned int skyboxShaderProgram;
    unsigned int VAO;
    Model* playerModel;
    Model* skyboxModel;
    LevelGenerator levelGen;
    std::vector<GroundSegment> groundSegments;
    std::vector<GroundSegment> wallSegments[3];

    float gameTime;
    float groundScroll;
    float gameStartTimer = 0.0f;
    double animStateStartTime = 0.0;
    Player::AnimState lastAnimState = Player::AnimState::Run;
    std::string prevAnimName = "";
    float prevAnimTime = 0.0f;
    bool prevAnimLoop = true;
    std::string lastAnimName = "";
    float lastAnimTime = 0.0f;
    bool lastAnimLoop = true;
    float crossFadeDuration = 0.25f;

    struct CharacterOption {
        std::string name;
        std::string modelPath;
        Model* model;
    };

    // Instancia estática para acceder desde el callback
    unsigned int groundVAO, groundVBO, groundEBO;
    unsigned int groundTexture;
    unsigned int wallTextures[3];

    Model* coinModel;
    Model* trainObstacleModel;
    Model* rampObstacleModel;
    Model* overheadObstacleModel;
    ALuint gameAmbientBuffer = 0;
    ALuint characterSelectAmbientBuffer = 0;
    ALuint creditsAmbientBuffer = 0;
    ALuint gameOverSoundBuffer = 0;
    int runCoins = 0;

    int totalCoins = 0;
    float score = 0.0f;
    std::vector<bool> characterUnlocked;

    std::vector<CharacterOption> characters;
    int focusedSlot = 0;
    int selectedModelIndex = 0;
    float charSelectTime = 0.0f;
    float charFocusStartTime = 0.0f;
    int lastFocusedSlot = -1;
    bool charSelectBackHovered = false;
    float charSelectBackScale = 1.0f;

    static Game* instance;
public:
    Player player; // Público para acceso desde callback estático (temporal)
};

#endif
