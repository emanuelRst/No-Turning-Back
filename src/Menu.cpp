#include "Menu.h"
#include <glad/glad.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_TRUETYPE_IMPLEMENTATION
#include "../vendor/stb/stb_truetype.h"

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

Menu::Menu() {}

Menu::~Menu() {
    glDeleteVertexArrays(1, &textVAO);
    glDeleteBuffers(1, &textVBO);
    glDeleteProgram(textShaderProgram);
    glDeleteTextures(1, &atlasTexture);
}

void Menu::Init(const std::string& fontPath) {
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
}

void Menu::Update(double mouseX, double mouseY, int width, int height) {
    for (auto& button : buttons) {
        button.isHovered = (mouseX >= button.x && mouseX <= button.x + button.width &&
                            mouseY >= button.y && mouseY <= button.y + button.height);
    }
}

void Menu::Render(unsigned int shaderProgram, unsigned int quadVAO, int width, int height) {
    glDisable(GL_DEPTH_TEST);
    glUseProgram(shaderProgram);
    glBindVertexArray(quadVAO);

    glm::mat4 projection = glm::ortho(0.0f, (float)width, (float)height, 0.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    for (const auto& button : buttons) {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(button.x + button.width/2, button.y + button.height/2, 0.0f));
        model = glm::scale(model, glm::vec3(button.width, button.height, 1.0f));
        
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        
        if (button.isHovered) {
            glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.0f, 0.5f, 0.8f);
        } else {
            glUniform3f(glGetUniformLocation(shaderProgram, "objectColor"), 0.0f, 0.75f, 1.0f);
        }
        
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        
        RenderText(button.text, button.x + button.width/2, button.y + button.height/2, 1.0f, glm::vec3(1.0f, 1.0f, 1.0f), width, height);
        
        // Restore shader for button rendering
        glUseProgram(shaderProgram);
        glBindVertexArray(quadVAO);
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
    
    float cursorX = x - textWidth / 2.0f;
    float cursorY = y + 20.0f; 
    
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

bool Menu::HandleClick(double mouseX, double mouseY) {
    for (auto& button : buttons) {
        if (mouseX >= button.x && mouseX <= button.x + button.width &&
            mouseY >= button.y && mouseY <= button.y + button.height) {
            if (button.onClick) {
                button.onClick();
                return true;
            }
        }
    }
    return false;
}

void Menu::AddButton(const std::string& text, float x, float y, float w, float h, std::function<void()> onClick) {
    buttons.push_back({text, x, y, w, h, false, onClick});
}
