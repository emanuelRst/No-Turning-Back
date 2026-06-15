#ifndef RAMP_TRAIN_H
#define RAMP_TRAIN_H

#include "GameObject.h"

class RampTrain : public GameObject {
public:
    RampTrain(const glm::vec3& position, const glm::vec3& size);
    
    // Función para obtener la altura del jugador en la rampa dado su Z
    float GetHeightAt(float playerZ) const;
    
    Bounds rampBounds; // Parte inclinada
    Bounds roofBounds; // Parte horizontal
};

#endif
