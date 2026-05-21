#ifndef GAME_H
#define GAME_H

#include <vendor/glad/glad.h> // <- ¡IMPORTANTE! GLAD siempre va primero
#include <GLFW/glfw3.h>

class Game {
private:
    GLFWwindow* window;

public:
    Game();
    ~Game();
    void init(GLFWwindow* currentWindow);
    void update(float deltaTime);
    void render();
};

#endif
