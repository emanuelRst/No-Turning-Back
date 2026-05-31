#ifndef GAME_H
#define GAME_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Player.h"

class Game {
public:
    Game(int width, int height);
    ~Game();

    bool Init();
    void Run();

    // Callback estático para GLFW
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

private:
    void Update(float deltaTime);
    void Render();

    GLFWwindow* window;
    int width, height;
    
    // Shader program y buffers
    unsigned int shaderProgram;
    unsigned int VAO;

    // Instancia estática para acceder desde el callback
    static Game* instance;
public:
    Player player; // Público para acceso desde callback estático (temporal)
};

#endif
