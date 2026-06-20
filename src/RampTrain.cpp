#include "RampTrain.h"

RampTrain::RampTrain(const glm::vec3& position, const glm::vec3& size) 
    : GameObject(position, size) {
    // Definir los bounds de la rampa y tren unido
    // La rampa ocupa los primeros 2 metros en Z, el tren el resto (size.z - 2)
    float rampLength = 2.0f;
    float trainLength = size.z - rampLength;

    rampBounds.min = position + glm::vec3(-size.x/2, 0.0f, -size.z/2);
    rampBounds.max = position + glm::vec3(size.x/2, size.y, -size.z/2 + rampLength);

    roofBounds.min = position + glm::vec3(-size.x/2, size.y, -size.z/2 + rampLength);
    roofBounds.max = position + glm::vec3(size.x/2, size.y, size.z/2);
}

float RampTrain::GetHeightAt(float playerZ) const {
    // Solo interpolar en la parte de la rampa
    if (playerZ >= rampBounds.min.z && playerZ <= rampBounds.max.z) {
        float t = (playerZ - rampBounds.min.z) / (rampBounds.max.z - rampBounds.min.z);
        t = glm::clamp(t, 0.0f, 1.0f);
        return rampBounds.min.y + t * (rampBounds.max.y - rampBounds.min.y);
    }
    // Si está sobre el tren, altura constante
    return rampBounds.max.y;
}
