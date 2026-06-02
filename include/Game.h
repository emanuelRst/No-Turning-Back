#ifndef GAME_H
#define GAME_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include "Player.h"
#include "Train.h"
#include "Model.h"

class Game {
public:
    Game(int width, int height);
    ~Game();

    bool Init();
    void Run();

    // Callback estático para GLFW
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void FramebufferSizeCallback(GLFWwindow* window, int width, int height);

private:
    void Update(float deltaTime);
    void Render();
    void UpdateTrains(float deltaTime);
    void ResetTrain(Train& train);
    void ResetRun();

    GLFWwindow* window;
    int width, height;
    
    // Shader program y buffers
    unsigned int shaderProgram;
    unsigned int VAO;
    Model* playerModel;
    std::vector<Train> trains;
    int nextTrainLane;

    // Instancia estática para acceder desde el callback
    static Game* instance;
public:
    Player player; // Público para acceso desde callback estático (temporal)
};

#endif
