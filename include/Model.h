#pragma once

#include <string>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <glad/glad.h>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

class Model {
public:
    Model(const std::string& path);
    ~Model();

    void Draw(unsigned int shaderProgram, const glm::mat4& modelMatrix);

private:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    unsigned int VAO, VBO, EBO;

    void LoadModel(const std::string& path);
    void SetupMesh();
};
