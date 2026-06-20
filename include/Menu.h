#ifndef MENU_H
#define MENU_H

#include <glad/glad.h>
#include <vector>
#include <string>
#include <functional>
#include <map>
#include <glm/glm.hpp>
#include "AudioManager.h"
#include <AL/al.h>

// Struct for character glyph info
struct Character {
    unsigned int TextureID;
    glm::ivec2   Size;
    glm::ivec2   Bearing;
    unsigned int Advance;
    float u0, v0, u1, v1; // Tex coords
};

struct Button {
    std::string text;
    float x, y, width, height;
    bool isHovered;
    bool wasHovered = false;
    float currentScale = 1.0f;
    std::function<void()> onClick;
    ALuint hoverSoundBuffer = 0;
};

class Menu {
public:
    Menu();
    ~Menu();
    void Init(const std::string& fontPath);
    void SetMousePos(double x, double y);
    void Update(float deltaTime, int width, int height);
    void Render(unsigned int shaderProgram, unsigned int quadVAO, int width, int height);
    void RenderText(const std::string& text, float x, float y, float scale, glm::vec3 color, int width, int height);
    void RenderImage(const std::string& imagePath, float x, float y, float w, float h, int width, int height);
    bool HandleClick(double mouseX, double mouseY);
    void HandleKeyEvent(int key); // Nueva función
    void AddButton(const std::string& text, float x, float y, float w, float h, std::function<void()> onClick, const std::string& audioPath = "");
    void StartAmbient();
    void StopAmbient();
    float GetTextWidth(const std::string& text, float scale);
    void GetTextVerticalBounds(const std::string& text, float scale, float& minBearingY, float& maxBearingY);

private:
    std::vector<Button> buttons;
    double mouseX = 0.0, mouseY = 0.0;
    int selectedButtonIndex = -1; // Índice del botón seleccionado por teclado
    
    AudioManager audioManager;
    
    ALuint ambientBuffer = 0;

    struct MenuUniformCache {
        GLint textColor;
        GLint text;
        GLint projection;
        GLint image;
    } menuUC;

    bool menuUCInitialized = false;
    
    std::map<char, Character> Characters;
    unsigned int textVAO, textVBO;
    unsigned int textShaderProgram;
    unsigned int atlasTexture;
    unsigned int backgroundTexture;
    unsigned int wasdTexture;
    unsigned int backgroundShaderProgram;
    unsigned int imageShaderProgram;
    unsigned int backgroundVAO, backgroundVBO;
};

#endif


