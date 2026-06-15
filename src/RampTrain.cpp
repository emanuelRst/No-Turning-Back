#include "RampTrain.h"

RampTrain::RampTrain(const glm::vec3& position, const glm::vec3& size) 
    : GameObject(position, size) {
    // Definir los bounds de la rampa y techo relativos a la posición
    // Esto es un ejemplo, se debe ajustar según tus necesidades.
    rampBounds.min = position + glm::vec3(-size.x/2, 0.0f, -size.z/2);
    rampBounds.max = position + glm::vec3(size.x/2, size.y, -size.z/2 + 2.0f); // 2 unidades de rampa

    roofBounds.min = position + glm::vec3(-size.x/2, size.y, -size.z/2 + 2.0f);
    roofBounds.max = position + glm::vec3(size.x/2, size.y, size.z/2);
}

float RampTrain::GetHeightAt(float playerZ) const {
    // Interpolación simple: Y aumenta de 0 a size.y en los primeros 2 unidades de Z
    float t = (playerZ - rampBounds.min.z) / 2.0f;
    t = glm::clamp(t, 0.0f, 1.0f);
    return rampBounds.min.y + t * (rampBounds.max.y - rampBounds.min.y);
}
