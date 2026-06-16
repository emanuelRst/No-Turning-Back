#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <glad/glad.h>

#define MAX_BONE_INFLUENCE 4

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    int m_BoneIDs[MAX_BONE_INFLUENCE];
    float m_Weights[MAX_BONE_INFLUENCE];
};

struct Texture {
    unsigned int id;
    std::string type;
    std::string path;
};

struct Mesh {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    unsigned int VAO, VBO, EBO;
};

#include <map>

struct ModelAABB {
    glm::vec3 min{0.0f};
    glm::vec3 max{0.0f};
};

class Model {
public:
    Model(const std::string& path);
    ~Model();

    float GetAnimationDurationInSeconds(const std::string& animName) const;
    void Draw(unsigned int shaderProgram, const glm::mat4& modelMatrix, float animationTime, const std::string& animName = "", bool loop = true);
    int GetAnimationIndex(const std::string& name) const;

    // AABB en espacio del asset (con transforms de nodos). Sirve para escenas estáticas.
    ModelAABB GetAABB() const { return modelAABB; }
    // AABB en espacio del mesh (vértices crudos, sin transforms de nodos).
    // Para modelos skinned el render usa huesos que cancelan los transforms del armature,
    // por lo que este AABB refleja el tamaño real del modelo renderizado.
    ModelAABB GetMeshAABB() const { return meshAABB; }

private:
    std::vector<Mesh> meshes;
    std::string directory;
    std::vector<Texture> textures_loaded;
    ModelAABB modelAABB{};
    ModelAABB meshAABB{};

    // Bone data
    std::map<std::string, unsigned int> boneMapping;
    unsigned int boneCount = 0;
    std::vector<glm::mat4> boneOffset;
    
    // Animation data
    Assimp::Importer importer;
    const aiScene* scene;
    std::vector<glm::mat4> m_BoneMatrices;
    std::map<std::string, unsigned int> animationMapping;

    void LoadModel(const std::string& path);
    void processNode(aiNode *node, const aiScene *scene);
    // Calcula el AABB aplicando recursivamente las transformaciones de los nodos.
    // Sin esto, los meshes hijos quedan en espacios locales disjuntos y el AABB
    // resultante no representa la silueta real del modelo.
    void ComputeAABB(aiNode* node, const glm::mat4& parentTransform);
    Mesh processMesh(aiMesh *mesh, const aiScene *scene);
    std::vector<Texture> loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);
    unsigned int TextureFromFile(const char *path, const std::string &directory);
    
    // Animation helpers
    void extractBoneWeightForMesh(aiMesh* mesh, Mesh& myMesh);
    glm::mat4 aiMatrixToGlm(const aiMatrix4x4& from);
    void ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const glm::mat4& ParentTransform, unsigned int animIndex = 0);
    const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, const std::string NodeName);
    void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
};
