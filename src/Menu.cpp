#include <sstream>
#include "Menu.h"
#include <glad/glad.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "../vendor/stb/stb_truetype.h"
#include <SOIL2/SOIL2.h>

const char* textVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec4 vertex; // <vec2 pos, vec2 tex>
out vec2 TexCoords;
uniform mat4 projection;
void main() {
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
}
)";

const char* textFragmentShaderSource = R"(
#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D text;
uniform vec3 textColor;
void main() {
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r);
    FragColor = vec4(textColor, 1.0) * sampled;
}
)";

const char* imageFragmentShaderSource = R"(
#version 330 core
in vec2 TexCoords;
out vec4 FragColor;
uniform sampler2D image;
void main() {
    FragColor = texture(image, TexCoords);
}
)";

// Helper to load shader file
std::string ReadShaderFile(const std::string& path) {
    std::ifstream file(path);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

Menu::Menu() {}

Menu::~Menu() {
    // Detener todo el audio antes de borrar buffers
    audioManager.StopAmbient();
    audioManager.StopAllSources();

    // Borrar buffers de sonido
    if (ambientBuffer != 0) alDeleteBuffers(1, &ambientBuffer);
    if (sharedHoverSoundBuffer != 0) alDeleteBuffers(1, &sharedHoverSoundBuffer);
    for (auto& button : buttons) {
        if (button.hoverSoundBuffer != 0) {
            alDeleteBuffers(1, &button.hoverSoundBuffer);
        }
    }

    // Limpieza OpenGL
    glDeleteVertexArrays(1, &textVAO);
    glDeleteBuffers(1, &textVBO);
    glDeleteVertexArrays(1, &backgroundVAO);
    glDeleteBuffers(1, &backgroundVBO);
    glDeleteProgram(textShaderProgram);
    glDeleteProgram(backgroundShaderProgram);
    glDeleteTextures(1, &atlasTexture);
    glDeleteTextures(1, &backgroundTexture);
    for (auto const& [path, tex] : imageTextures) {
        glDeleteTextures(1, &tex);
    }
}

float Menu::GetTextWidth(const std::string& text, float scale) {
    float width = 0.0f;
    for (char c : text) {
        if (Characters.find(c) != Characters.end())
            width += Characters[c].Advance * scale;
    }
    return width;
}

void Menu::GetTextVerticalBounds(const std::string& text, float scale, float& minBearingY, float& maxBearingY) {
    minBearingY = 10000.0f;
    maxBearingY = -10000.0f;

    for (char c : text) {
        if (Characters.find(c) != Characters.end()) {
            Character ch = Characters[c];
            float top = -ch.Bearing.y * scale;
            float bottom = (ch.Size.y - ch.Bearing.y) * scale;
            
            if (top < minBearingY) minBearingY = top;
            if (bottom > maxBearingY) maxBearingY = bottom;
        }
    }
}

void Menu::Init(const std::string& fontPath, const std::string& bgPath) {
    // Compile shaders
    unsigned int vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &textVertexShaderSource, NULL);
    glCompileShader(vShader);
    unsigned int fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &textFragmentShaderSource, NULL);
    glCompileShader(fShader);
    textShaderProgram = glCreateProgram();
    glAttachShader(textShaderProgram, vShader);
    glAttachShader(textShaderProgram, fShader);
    glLinkProgram(textShaderProgram);

    // Load and compile background shader
    std::string vSource = ReadShaderFile("assets/shaders/background.vert");
    std::string fSource = ReadShaderFile("assets/shaders/background.frag");
    const char* vSourceC = vSource.c_str();
    const char* fSourceC = fSource.c_str();

    unsigned int vShaderBg = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShaderBg, 1, &vSourceC, NULL);
    glCompileShader(vShaderBg);
    unsigned int fShaderBg = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShaderBg, 1, &fSourceC, NULL);
    glCompileShader(fShaderBg);
    backgroundShaderProgram = glCreateProgram();
    glAttachShader(backgroundShaderProgram, vShaderBg);
    glAttachShader(backgroundShaderProgram, fShaderBg);
    glLinkProgram(backgroundShaderProgram);

    // Setup background VAO/VBO
    float quadVertices[] = {
        // positions        // texCoords
        -1.0f,  1.0f,       0.0f, 1.0f,
        -1.0f, -1.0f,       0.0f, 0.0f,
         1.0f, -1.0f,       1.0f, 0.0f,

        -1.0f,  1.0f,       0.0f, 1.0f,
         1.0f, -1.0f,       1.0f, 0.0f,
         1.0f,  1.0f,       1.0f, 1.0f
    };
    glGenVertexArrays(1, &backgroundVAO);
    glGenBuffers(1, &backgroundVBO);
    glBindVertexArray(backgroundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, backgroundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    // Compile image shader
    unsigned int vShaderImg = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShaderImg, 1, &textVertexShaderSource, NULL);
    glCompileShader(vShaderImg);
    unsigned int fShaderImg = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShaderImg, 1, &imageFragmentShaderSource, NULL);
    glCompileShader(fShaderImg);
    imageShaderProgram = glCreateProgram();
    glAttachShader(imageShaderProgram, vShaderImg);
    glAttachShader(imageShaderProgram, fShaderImg);
    glLinkProgram(imageShaderProgram);

    // Load background texture
    backgroundTexture = SOIL_load_OGL_texture(
        bgPath.c_str(),
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS
    );
    if (backgroundTexture == 0) {
        std::cerr << "Failed to load background image: " << SOIL_last_result() << std::endl;
    }

    // Load wasd texture
    wasdTexture = SOIL_load_OGL_texture(
        "assets/textures/Menu/wasd.png",
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS
    );
    if (wasdTexture == 0) {
        std::cerr << "Failed to load wasd image: " << SOIL_last_result() << std::endl;
    }

    // Prepare VAO/VBO
    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Load font file
    std::ifstream fontFile(fontPath, std::ios::binary);
    if (!fontFile.is_open()) {
        std::cerr << "Failed to load font: " << fontPath << std::endl;
        return;
    }
    std::vector<unsigned char> fontBuffer((std::istreambuf_iterator<char>(fontFile)), std::istreambuf_iterator<char>());
    
    // Bake font
    unsigned char atlasData[512 * 512];
    stbtt_bakedchar cdata[96];
    stbtt_BakeFontBitmap(fontBuffer.data(), 0, 64.0f, atlasData, 512, 512, 32, 96, cdata);

    // Create texture
    glGenTextures(1, &atlasTexture);
    glBindTexture(GL_TEXTURE_2D, atlasTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 512, 512, 0, GL_RED, GL_UNSIGNED_BYTE, atlasData);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Fill characters map
    for (int i = 0; i < 96; i++) {
        Character ch = {
            atlasTexture,
            glm::ivec2(cdata[i].x1 - cdata[i].x0, cdata[i].y1 - cdata[i].y0),
            glm::ivec2(cdata[i].xoff, cdata[i].yoff),
            (unsigned int)cdata[i].xadvance,
            cdata[i].x0 / 512.0f, cdata[i].y0 / 512.0f,
            cdata[i].x1 / 512.0f, cdata[i].y1 / 512.0f
        };
        Characters.insert(std::pair<char, Character>(32 + i, ch));
    }

    // Actualizar dimensiones de botones basados en el texto
    for (auto& button : buttons) {
        // Solo actualizar si no se especificaron dimensiones en AddButton, 
        // pero actualmente AddButton siempre recibe valores.
        // Vamos a mantener los valores de AddButton.
        if (button.width == 0.0f) button.width = GetTextWidth(button.text, 1.0f);
        if (button.height == 0.0f) button.height = 64.0f; 
    }

    audioManager.LoadSound("assets/audio/Menu/kiss_in_the_dark.wav", ambientBuffer);
    audioManager.LoadSound("assets/audio/Menu/Voicy_Obtain.wav", sharedHoverSoundBuffer);
}

void Menu::SetMousePos(double x, double y) {
    mouseX = x;
    mouseY = y;
}

void Menu::Update(float deltaTime, int width, int height) {
    time += deltaTime;
    float smoothSpeed = 10.0f; // Velocidad de suavizado

    for (int i = 0; i < buttons.size(); ++i) {
        auto& button = buttons[i];
        float adjustedButtonY = button.y;
        
        float halfW = button.width / 2.0f;
        float halfH = button.height / 2.0f;
        
        bool isCurrentlyMouseHovered = (mouseX >= button.x - halfW && mouseX <= button.x + halfW &&
                                       mouseY >= adjustedButtonY - halfH && mouseY <= adjustedButtonY + halfH);
        
        if (isCurrentlyMouseHovered) {
            button.isHovered = true;
            selectedButtonIndex = i; // Mouse overrides keyboard
        } else {
            button.isHovered = (i == selectedButtonIndex);
        }
        
        if (button.isHovered && !button.wasHovered) {
            ALuint soundToPlay = (button.hoverSoundBuffer != 0) ? button.hoverSoundBuffer : sharedHoverSoundBuffer;
            if (soundToPlay != 0) {
                audioManager.PlaySound(soundToPlay);
            }
        }
        button.wasHovered = button.isHovered;
        
        float targetScale = button.isHovered ? 1.2f : 1.0f;
        button.currentScale += (targetScale - button.currentScale) * smoothSpeed * deltaTime;
    }
}

void Menu::HandleKeyEvent(int key) {
    if (buttons.empty()) return;

    if (key == GLFW_KEY_UP) {
        selectedButtonIndex--;
        if (selectedButtonIndex < 0) selectedButtonIndex = (int)buttons.size() - 1;
        if (sharedHoverSoundBuffer != 0) audioManager.PlaySound(sharedHoverSoundBuffer);
    } else if (key == GLFW_KEY_DOWN) {
        selectedButtonIndex++;
        if (selectedButtonIndex >= (int)buttons.size()) selectedButtonIndex = 0;
        if (sharedHoverSoundBuffer != 0) audioManager.PlaySound(sharedHoverSoundBuffer);
    } else if (key == GLFW_KEY_ENTER) {
        if (selectedButtonIndex >= 0 && selectedButtonIndex < (int)buttons.size()) {
            if (buttons[selectedButtonIndex].onClick) {
                buttons[selectedButtonIndex].onClick();
            }
        }
    }
}

void Menu::Render(unsigned int shaderProgram, unsigned int quadVAO, int width, int height, bool drawBackground) {
    glDisable(GL_DEPTH_TEST);
    
    // Draw background
    if (drawBackground) {
        glUseProgram(backgroundShaderProgram);
        glUniform1i(glGetUniformLocation(backgroundShaderProgram, "image"), 0);
        glUniform1f(glGetUniformLocation(backgroundShaderProgram, "u_time"), time);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, backgroundTexture);
        glBindVertexArray(backgroundVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);
    }

    // Dibujar texto directamente
    for (const auto& button : buttons) {
        glm::vec3 color = button.isHovered ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(1.0f, 1.0f, 1.0f);
        // Usar la escala suavizada
        float scale = button.currentScale;
        // Usar button.x y button.y como centro
        RenderText(button.text, button.x, button.y, scale, color, width, height);
    }
    
    glEnable(GL_DEPTH_TEST);
}

void Menu::RenderText(const std::string& text, float x, float y, float scale, glm::vec3 color, int width, int height) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glUseProgram(textShaderProgram);
    glUniform3f(glGetUniformLocation(textShaderProgram, "textColor"), color.x, color.y, color.z);
    glUniform1i(glGetUniformLocation(textShaderProgram, "text"), 0); // Set sampler to texture unit 0
    
    glm::mat4 projection = glm::ortho(0.0f, (float)width, (float)height, 0.0f);
    glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    float textWidth = 0;
    for (char c : text) {
        if (Characters.find(c) != Characters.end())
            textWidth += Characters[c].Advance * scale;
    }
    
    float minBearingY, maxBearingY;
    GetTextVerticalBounds(text, scale, minBearingY, maxBearingY);
    float textCenterY = (minBearingY + maxBearingY) / 2.0f;
    
    float cursorX = x - textWidth / 2.0f;
    float cursorY = y - textCenterY; 
    
    for (char c : text) {
        if (Characters.find(c) == Characters.end()) continue;
        Character ch = Characters[c];
        
        float xpos = cursorX + ch.Bearing.x * scale;
        float ypos = cursorY - ch.Bearing.y * scale;
        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        
        float vertices[6][4] = {
            { xpos,     ypos + h,   ch.u0, ch.v1 },
            { xpos,     ypos,       ch.u0, ch.v0 },
            { xpos + w, ypos,       ch.u1, ch.v0 },

            { xpos,     ypos + h,   ch.u0, ch.v1 },
            { xpos + w, ypos,       ch.u1, ch.v0 },
            { xpos + w, ypos + h,   ch.u1, ch.v1 }
        };

        
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        cursorX += ch.Advance * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glDisable(GL_BLEND);
}

void Menu::RenderImage(const std::string& imagePath, float x, float y, float w, float h, int width, int height) {
    unsigned int textureID = 0;
    if (imageTextures.find(imagePath) == imageTextures.end()) {
        textureID = SOIL_load_OGL_texture(
            imagePath.c_str(),
            SOIL_LOAD_AUTO,
            SOIL_CREATE_NEW_ID,
            SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS
        );
        imageTextures[imagePath] = textureID;
    } else {
        textureID = imageTextures[imagePath];
    }
    
    if (textureID == 0) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glUseProgram(imageShaderProgram);
    glUniform1i(glGetUniformLocation(imageShaderProgram, "image"), 0);
    
    glm::mat4 projection = glm::ortho(0.0f, (float)width, (float)height, 0.0f);
    glUniformMatrix4fv(glGetUniformLocation(imageShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glBindVertexArray(textVAO);

    float xpos = x - w / 2.0f;
    float ypos = y - h / 2.0f;
    
    float vertices[6][4] = {
        { xpos,     ypos + h,   0.0, 1.0 },
        { xpos,     ypos,       0.0, 0.0 },
        { xpos + w, ypos,       1.0, 0.0 },

        { xpos,     ypos + h,   0.0, 1.0 },
        { xpos + w, ypos,       1.0, 0.0 },
        { xpos + w, ypos + h,   1.0, 1.0 }
    };
    
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
}

void Menu::LoadImage(const std::string& imagePath) {
    if (imageTextures.find(imagePath) == imageTextures.end()) {
        unsigned int textureID = SOIL_load_OGL_texture(
            imagePath.c_str(),
            SOIL_LOAD_AUTO,
            SOIL_CREATE_NEW_ID,
            SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS
        );
        if (textureID != 0) {
            imageTextures[imagePath] = textureID;
        } else {
            std::cerr << "Failed to load image: " << imagePath << " - " << SOIL_last_result() << std::endl;
        }
    }
}

bool Menu::HandleClick(double mouseX, double mouseY) {
    for (auto& button : buttons) {
        float adjustedButtonY = button.y;
        
        float halfW = button.width / 2.0f;
        float halfH = button.height / 2.0f;
        
        if (mouseX >= button.x - halfW && mouseX <= button.x + halfW &&
            mouseY >= adjustedButtonY - halfH && mouseY <= adjustedButtonY + halfH) {
            if (button.onClick) {
                button.onClick();
                return true;
            }
        }
    }
    return false;
}

void Menu::AddButton(const std::string& text, float x, float y, float w, float h, std::function<void()> onClick, const std::string& audioPath) {
    ALuint buffer = 0;
    if (!audioPath.empty()) {
        audioManager.LoadSound(audioPath, buffer);
    }
    buttons.push_back({text, x, y, w, h, false, false, 1.0f, onClick, buffer});
}

void Menu::StartAmbient() {
    if (ambientBuffer != 0) {
        audioManager.PlayAmbient(ambientBuffer);
    }
}

void Menu::StopAmbient() {
    audioManager.StopAmbient();
}

