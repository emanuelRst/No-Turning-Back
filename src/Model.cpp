#include "Model.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <map>
#include <vector>
#include <string>
#include <algorithm>
#include <limits>
#include <cstdlib>

#include <SOIL2/SOIL2.h>

glm::mat4 Model::aiMatrixToGlm(const aiMatrix4x4& from) {
    glm::mat4 to;
    to[0][0] = from.a1; to[1][0] = from.a2; to[2][0] = from.a3; to[3][0] = from.a4;
    to[0][1] = from.b1; to[1][1] = from.b2; to[2][1] = from.b3; to[3][1] = from.b4;
    to[0][2] = from.c1; to[1][2] = from.c2; to[2][2] = from.c3; to[3][2] = from.c4;
    to[0][3] = from.d1; to[1][3] = from.d2; to[2][3] = from.d3; to[3][3] = from.d4;
    return to;
}

Model::Model(const std::string& path) {
    LoadModel(path);
}

Model::~Model() {
    for (auto& mesh : meshes) {
        glDeleteVertexArrays(1, &mesh.VAO);
        glDeleteBuffers(1, &mesh.VBO);
        glDeleteBuffers(1, &mesh.EBO);
    }
}

float Model::GetAnimationDurationInSeconds(const std::string& animName) const {
    auto it = animationMapping.find(animName);
    if (it != animationMapping.end() && scene && scene->HasAnimations()) {
        unsigned int animIndex = it->second;
        const aiAnimation* anim = scene->mAnimations[animIndex];
        float TicksPerSecond = (float)(anim->mTicksPerSecond != 0 ? anim->mTicksPerSecond : 25.0f);
        return (float)anim->mDuration / TicksPerSecond;
    }
    return 0.0f;
}

void Model::Draw(unsigned int shaderProgram, const glm::mat4& modelMatrix, float animationTime, const std::string& animName, bool loop) {
    if (shaderProgram != lastShaderProgram) {
        lastShaderProgram = shaderProgram;
        cachedModelLoc = glGetUniformLocation(shaderProgram, "model");
        cachedUseTextureLoc = glGetUniformLocation(shaderProgram, "useTexture");
        for (int i = 0; i < 100; i++) {
            cachedBoneLocs[i] = glGetUniformLocation(shaderProgram,
                ("boneMatrices[" + std::to_string(i) + "]").c_str());
        }
    }

    int animIndex = -1;
    if (!animName.empty()) {
        auto it = animationMapping.find(animName);
        if (it != animationMapping.end()) {
            animIndex = (int)it->second;
        }
    }

    if (animIndex != -1 && scene && scene->HasAnimations() && animIndex < (int)scene->mNumAnimations) {
        const aiAnimation* anim = scene->mAnimations[animIndex];
        float TicksPerSecond = (float)(anim->mTicksPerSecond != 0 ? anim->mTicksPerSecond : 25.0f);
        float TimeInTicks = animationTime * TicksPerSecond;
        float AnimationTime = loop ? fmod(TimeInTicks, (float)anim->mDuration) : std::min(TimeInTicks, (float)anim->mDuration);

        m_BoneMatrices.resize(boneCount);
        ReadNodeHierarchy(AnimationTime, scene->mRootNode, glm::mat4(1.0f), animIndex);
        for (unsigned int i = 0; i < m_BoneMatrices.size(); i++) {
             glUniformMatrix4fv(cachedBoneLocs[i], 1, GL_FALSE, glm::value_ptr(m_BoneMatrices[i]));
        }
    } else {
        glm::mat4 identity(1.0f);
        for (int i = 0; i < 100; i++) {
            if (cachedBoneLocs[i] == -1) break;
            glUniformMatrix4fv(cachedBoneLocs[i], 1, GL_FALSE, glm::value_ptr(identity));
        }
    }

    glUniformMatrix4fv(cachedModelLoc, 1, GL_FALSE, glm::value_ptr(modelMatrix));
    
    bool hasTextures = false;
    for (const auto& mesh : meshes) {
        if (!mesh.textures.empty()) {
            hasTextures = true;
            break;
        }
    }
    glUniform1i(cachedUseTextureLoc, hasTextures ? 1 : 0);

    for (auto& mesh : meshes) {
        unsigned int diffuseNr = 1;
        for (unsigned int i = 0; i < mesh.textures.size(); i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            std::string number;
            std::string name = mesh.textures[i].type;
            if (name == "texture_diffuse") number = std::to_string(diffuseNr++);
            
            GLint loc = glGetUniformLocation(shaderProgram, (name + number).c_str());
            glUniform1i(loc, i);
            glBindTexture(GL_TEXTURE_2D, mesh.textures[i].id);
        }

        glBindVertexArray(mesh.VAO);
        glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(mesh.indices.size()), GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        
        glActiveTexture(GL_TEXTURE0);
    }
    
    glDisableVertexAttribArray(3);
    glDisableVertexAttribArray(4);
}

int Model::GetAnimationIndex(const std::string& name) const {
    auto it = animationMapping.find(name);
    if (it != animationMapping.end()) {
        return (int)it->second;
    }
    return -1;
}

void Model::LoadModel(const std::string& path) {
    scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
        return;
    }

    directory = std::filesystem::path(path).parent_path().string();
    modelAABB = ModelAABB{};
    modelAABB.min = glm::vec3(std::numeric_limits<float>::max());
    modelAABB.max = glm::vec3(std::numeric_limits<float>::lowest());
    meshAABB = ModelAABB{};
    meshAABB.min = glm::vec3(std::numeric_limits<float>::max());
    meshAABB.max = glm::vec3(std::numeric_limits<float>::lowest());

    // AABB real: recorrer la jerarquía aplicando mTransformation de cada nodo.
    ComputeAABB(scene->mRootNode, glm::mat4(1.0f));

    // Mapear animaciones por nombre
    for (unsigned int i = 0; i < scene->mNumAnimations; ++i) {
        std::string name = scene->mAnimations[i]->mName.C_Str();
        animationMapping[name] = i;
    }

    // Si la carga no produjo vértices (escena vacía), dejar un AABB unitario
    // para que la hitbox resultante no sea degenerada.
    if (modelAABB.min.x > modelAABB.max.x) {
        modelAABB.min = glm::vec3(-0.5f);
        modelAABB.max = glm::vec3(0.5f);
    }

    processNode(scene->mRootNode, scene);
}

void Model::ComputeAABB(aiNode* node, const glm::mat4& parentTransform) {
    const glm::mat4 nodeTransform = parentTransform * aiMatrixToGlm(node->mTransformation);

    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        if (!mesh || mesh->mNumVertices == 0) continue;

        for (unsigned int v = 0; v < mesh->mNumVertices; ++v) {
            const aiVector3D p = mesh->mVertices[v];
            const glm::vec4 worldPos = nodeTransform * glm::vec4(p.x, p.y, p.z, 1.0f);
            const glm::vec3 pos(worldPos.x, worldPos.y, worldPos.z);
            modelAABB.min = glm::min(modelAABB.min, pos);
            modelAABB.max = glm::max(modelAABB.max, pos);
        }
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        ComputeAABB(node->mChildren[i], nodeTransform);
    }
}

void Model::processNode(aiNode *node, const aiScene *scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        // Acumular AABB en espacio del mesh (vértices crudos, sin transforms).
        // Para modelos skinned los transforms de nodo se cancelan vía huesos en bind pose,
        // por lo que el tamaño renderizado corresponde a estos vértices sin transformar.
        for (unsigned int v = 0; v < mesh->mNumVertices; v++) {
            glm::vec3 pos(mesh->mVertices[v].x, mesh->mVertices[v].y, mesh->mVertices[v].z);
            meshAABB.min = glm::min(meshAABB.min, pos);
            meshAABB.max = glm::max(meshAABB.max, pos);
        }
        meshes.push_back(processMesh(mesh, scene));
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

void Model::extractBoneWeightForMesh(aiMesh* mesh, Mesh& myMesh) {
    for (unsigned int i = 0; i < mesh->mNumBones; i++) {
        unsigned int boneIndex = 0;
        std::string boneName = mesh->mBones[i]->mName.C_Str();
        if (boneMapping.find(boneName) == boneMapping.end()) {
            boneIndex = boneCount;
            boneCount++;
            boneOffset.push_back(aiMatrixToGlm(mesh->mBones[i]->mOffsetMatrix));
            boneMapping[boneName] = boneIndex;
        } else {
            boneIndex = boneMapping[boneName];
        }

        for (unsigned int j = 0; j < mesh->mBones[i]->mNumWeights; j++) {
            unsigned int vertexID = mesh->mBones[i]->mWeights[j].mVertexId;
            float weight = mesh->mBones[i]->mWeights[j].mWeight;
            
            for (int k = 0; k < MAX_BONE_INFLUENCE; k++) {
                if (myMesh.vertices[vertexID].m_Weights[k] == 0.0f) {
                    myMesh.vertices[vertexID].m_BoneIDs[k] = boneIndex;
                    myMesh.vertices[vertexID].m_Weights[k] = weight;
                    break;
                }
            }
        }
    }
}

Mesh Model::processMesh(aiMesh *mesh, const aiScene *scene) {
    Mesh newMesh;
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex vertex;
        vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        if (mesh->mTextureCoords[0])
            vertex.TexCoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        else
            vertex.TexCoords = glm::vec2(0.0f, 0.0f);
        
        for (int j = 0; j < MAX_BONE_INFLUENCE; j++) {
            vertex.m_BoneIDs[j] = 0;
            vertex.m_Weights[j] = 0.0f;
        }

        newMesh.vertices.push_back(vertex);
    }

    extractBoneWeightForMesh(mesh, newMesh);

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++)
            newMesh.indices.push_back(face.mIndices[j]);
    }

    if (mesh->mMaterialIndex >= 0) {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
        std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        if (diffuseMaps.empty()) {
            diffuseMaps = loadMaterialTextures(material, aiTextureType_BASE_COLOR, "texture_diffuse");
        }
        newMesh.textures.insert(newMesh.textures.end(), diffuseMaps.begin(), diffuseMaps.end());
    }

    glGenVertexArrays(1, &newMesh.VAO);
    glGenBuffers(1, &newMesh.VBO);
    glGenBuffers(1, &newMesh.EBO);

    glBindVertexArray(newMesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, newMesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, newMesh.vertices.size() * sizeof(Vertex), &newMesh.vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, newMesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, newMesh.indices.size() * sizeof(unsigned int), &newMesh.indices[0], GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, Normal));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoords));
    
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, MAX_BONE_INFLUENCE, GL_INT, sizeof(Vertex), (void*)offsetof(Vertex, m_BoneIDs));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, MAX_BONE_INFLUENCE, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Weights));
    
    glBindVertexArray(0);

    return newMesh;
}

std::vector<Texture> Model::loadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName) {
    std::vector<Texture> textures;
    for (unsigned int i = 0; i < mat->GetTextureCount(type); i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
        bool skip = false;
        for (unsigned int j = 0; j < textures_loaded.size(); j++) {
            if (std::strcmp(textures_loaded[j].path.c_str(), str.C_Str()) == 0) {
                textures.push_back(textures_loaded[j]);
                skip = true;
                break;
            }
        }
        if (!skip) {
            Texture texture;
            if (str.C_Str()[0] == '*') {
                int idx = std::atoi(str.C_Str() + 1);
                const aiTexture* tex = scene->mTextures[idx];
                if (tex->mHeight == 0) {
                    texture.id = SOIL_load_OGL_texture_from_memory(
                        reinterpret_cast<unsigned char*>(tex->pcData), tex->mWidth,
                        SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
                        SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
                } else {
                    texture.id = SOIL_load_OGL_texture_from_memory(
                        reinterpret_cast<unsigned char*>(tex->pcData),
                        tex->mWidth * tex->mHeight * 4,
                        SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID,
                        SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS);
                }
            } else {
                texture.id = TextureFromFile(str.C_Str(), directory);
            }
            texture.type = typeName;
            texture.path = str.C_Str();
            textures.push_back(texture);
            textures_loaded.push_back(texture);
        }
    }
    return textures;
}

unsigned int Model::TextureFromFile(const char *path, const std::string &directory) {
    std::string filename = std::string(path);
    filename = directory + '/' + filename;

    unsigned int textureID = SOIL_load_OGL_texture(
        filename.c_str(),
        SOIL_LOAD_AUTO,
        SOIL_CREATE_NEW_ID,
        SOIL_FLAG_MIPMAPS | SOIL_FLAG_TEXTURE_REPEATS
    );

    if (textureID == 0) {
        std::cerr << "Texture failed to load at path: " << path << std::endl;
    }

    return textureID;
}

const aiNodeAnim* Model::FindNodeAnim(const aiAnimation* pAnimation, const std::string NodeName) {
    for (unsigned int i = 0; i < pAnimation->mNumChannels; i++) {
        const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];
        if (std::string(pNodeAnim->mNodeName.data) == NodeName) {
            return pNodeAnim;
        }
    }
    return nullptr;
}

void Model::CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim) {
    if (pNodeAnim->mNumRotationKeys == 1) {
        Out = pNodeAnim->mRotationKeys[0].mValue;
        return;
    }
    unsigned int RotationIndex = 0;
    for (unsigned int i = 0; i < pNodeAnim->mNumRotationKeys - 1; i++) {
        if (AnimationTime < pNodeAnim->mRotationKeys[i + 1].mTime) {
            RotationIndex = i;
            break;
        }
    }
    unsigned int NextRotationIndex = RotationIndex + 1;
    float DeltaTime = pNodeAnim->mRotationKeys[NextRotationIndex].mTime - pNodeAnim->mRotationKeys[RotationIndex].mTime;
    float Factor = (AnimationTime - pNodeAnim->mRotationKeys[RotationIndex].mTime) / DeltaTime;
    const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
    const aiQuaternion& EndRotationQ = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;
    aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);
    Out = Out.Normalize();
}

void Model::CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim) {
    if (pNodeAnim->mNumPositionKeys == 1) {
        Out = pNodeAnim->mPositionKeys[0].mValue;
        return;
    }
    unsigned int PositionIndex = 0;
    for (unsigned int i = 0; i < pNodeAnim->mNumPositionKeys - 1; i++) {
        if (AnimationTime < pNodeAnim->mPositionKeys[i + 1].mTime) {
            PositionIndex = i;
            break;
        }
    }
    unsigned int NextPositionIndex = PositionIndex + 1;
    float DeltaTime = pNodeAnim->mPositionKeys[NextPositionIndex].mTime - pNodeAnim->mPositionKeys[PositionIndex].mTime;
    float Factor = (AnimationTime - pNodeAnim->mPositionKeys[PositionIndex].mTime) / DeltaTime;
    const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
    const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
    Out = Start + Factor * (End - Start);
}

void Model::CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim) {
    if (pNodeAnim->mNumScalingKeys == 1) {
        Out = pNodeAnim->mScalingKeys[0].mValue;
        return;
    }
    unsigned int ScalingIndex = 0;
    for (unsigned int i = 0; i < pNodeAnim->mNumScalingKeys - 1; i++) {
        if (AnimationTime < pNodeAnim->mScalingKeys[i + 1].mTime) {
            ScalingIndex = i;
            break;
        }
    }
    unsigned int NextScalingIndex = ScalingIndex + 1;
    float DeltaTime = pNodeAnim->mScalingKeys[NextScalingIndex].mTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime;
    float Factor = (AnimationTime - pNodeAnim->mScalingKeys[ScalingIndex].mTime) / DeltaTime;
    const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
    const aiVector3D& End = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
    Out = Start + Factor * (End - Start);
}

void Model::ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const glm::mat4& ParentTransform, unsigned int animIndex) {
    std::string NodeName(pNode->mName.data);
    const aiAnimation* pAnimation = scene->mAnimations[animIndex];

    glm::mat4 NodeTransformation = aiMatrixToGlm(pNode->mTransformation);

    const aiNodeAnim* pNodeAnim = FindNodeAnim(pAnimation, NodeName);

    if (pNodeAnim) {
        aiVector3D Scaling;
        CalcInterpolatedScaling(Scaling, AnimationTime, pNodeAnim);
        glm::mat4 ScalingM = glm::scale(glm::mat4(1.0f), glm::vec3(Scaling.x, Scaling.y, Scaling.z));

        aiQuaternion RotationQ;
        CalcInterpolatedRotation(RotationQ, AnimationTime, pNodeAnim);
        glm::mat4 RotationM = glm::mat4_cast(glm::quat(RotationQ.w, RotationQ.x, RotationQ.y, RotationQ.z));

        aiVector3D Translation;
        CalcInterpolatedPosition(Translation, AnimationTime, pNodeAnim);
        glm::mat4 TranslationM = glm::translate(glm::mat4(1.0f), glm::vec3(Translation.x, Translation.y, Translation.z));

        NodeTransformation = TranslationM * RotationM * ScalingM;
    }

    glm::mat4 GlobalTransformation = ParentTransform * NodeTransformation;

    if (boneMapping.find(NodeName) != boneMapping.end()) {
        unsigned int BoneIndex = boneMapping[NodeName];
        m_BoneMatrices[BoneIndex] = GlobalTransformation * boneOffset[BoneIndex];
    }

    for (unsigned int i = 0; i < pNode->mNumChildren; i++) {
        ReadNodeHierarchy(AnimationTime, pNode->mChildren[i], GlobalTransformation, animIndex);
    }
}
