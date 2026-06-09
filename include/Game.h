#ifndef GAME_H
#define GAME_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "GameObject.h"
#include "Player.h"
#include "Train.h"
#include "Model.h"

#include <vector>
#include <string>

// Forward declaration
class Menu;

enum class GameState { MENU, PLAYING, GAME_OVER, PAUSED, HELP };

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

    GLFWwindow* window;
    int width, height;
    GameState currentState;
    GameState prevState = GameState::PLAYING;
    Menu* menu;
    Menu* gameOverMenu;
    Menu* pauseMenu;
    Menu* helpMenu;
    
    // Shader program y buffers
    unsigned int shaderProgram;
    unsigned int menuShaderProgram;
    unsigned int VAO;
    Model* playerModel;
    std::vector<Train> trains;
    std::vector<GroundSegment> groundSegments;
    int nextTrainLane;

    float gameTime;
    float groundScroll;

    // Instancia estática para acceder desde el callback
    unsigned int groundVAO, groundVBO, groundEBO;
    unsigned int groundTexture;

    unsigned int menuFBO = 0;
    unsigned int menuFBOTexture = 0;
    unsigned int menuFBORBO = 0;
    int menuFBOWidth = 0, menuFBOHeight = 0;
    unsigned int blurVAO = 0, blurVBO = 0;

    static Game* instance;
public:
    Player player; // Público para acceso desde callback estático (temporal)
};

#endif
