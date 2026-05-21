#include "Game.h"
#include <iostream>

Game::Game() : window(nullptr) {}
Game::~Game() {}

void Game::init(GLFWwindow* currentWindow) {
    window = currentWindow;
    std::cout << "Juego de OpenGL Inicializado de forma modular." << std::endl;
}

void Game::update(float deltaTime) {
    // Aquí irá la lógica de físicas y movimientos más adelante
}

void Game::render() {
    // El limpiado de pantalla ahora se maneja de forma modular aquí
    glClearColor(0.05f, 0.05f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}
