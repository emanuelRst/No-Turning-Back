#ifndef OBSTACLE_OVERHEAD_H
#define OBSTACLE_OVERHEAD_H

#include "GameObject.h"

class ObstacleOverhead : public GameObject {
public:
    ObstacleOverhead(const glm::vec3& position, const glm::vec3& size) 
        : GameObject(position, size) {}
    
    // El renderizado se maneja en el Game::Render o se podría añadir aquí
    // dependiendo de cómo estructures tus objetos.
};

#endif
