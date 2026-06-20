#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in ivec4 aBoneIDs;
layout (location = 4) in vec4 aWeights;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 boneMatrices[100];
uniform bool isAnimated;
uniform mat3 normalMatrix;

void main() {
    vec4 totalPosition;
    if (isAnimated) {
        mat4 BoneTransform = boneMatrices[aBoneIDs[0]] * aWeights[0];
        BoneTransform += boneMatrices[aBoneIDs[1]] * aWeights[1];
        BoneTransform += boneMatrices[aBoneIDs[2]] * aWeights[2];
        BoneTransform += boneMatrices[aBoneIDs[3]] * aWeights[3];
        totalPosition = BoneTransform * vec4(aPos, 1.0);
    } else {
        totalPosition = vec4(aPos, 1.0);
    }
    
    FragPos = vec3(model * totalPosition);
    Normal = normalMatrix * aNormal;
    TexCoords = aTexCoords;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
