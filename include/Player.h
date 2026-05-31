#ifndef PLAYER_H
#define PLAYER_H

#include <glm/glm.hpp>

class Player {
public:
    Player();

    // Actualiza la posición, física y movimiento del jugador
    void Update(float deltaTime);

    // Métodos para cambiar de carril
    void MoveLeft();
    void MoveRight();

    // Método para saltar
    void Jump();

    // Obtener posición para renderizado
    glm::vec3 GetPosition() const { return position; }

private:
    glm::vec3 position;
    float targetX;       // Carril objetivo (-2.0, 0.0, 2.0)
    int currentLane;     // 0: Izq, 1: Centro, 2: Der

    glm::vec3 velocity;
    bool isGrounded;
    bool isJumping;

    // Constantes de física
    const float jumpForce = 8.0f;
    const float gravity = 20.0f;
    const float laneSpeed = 15.0f;

    // Helper para actualizar X con Lerp
    void UpdateLaneMovement(float deltaTime);
    // Helper para actualizar Y con Gravedad
    void UpdatePhysics(float deltaTime);
};

#endif
