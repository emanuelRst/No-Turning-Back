#ifndef PLAYER_H
#define PLAYER_H

#include "GameObject.h"
#include <vector>

// Player es otro GameObject: su modelo visual puede cambiar, pero la fisica
// solo necesita su posicion, velocidad e hitbox.
class Player : public GameObject {
public:
    Player();

    // Update sin objetos mantiene compatibilidad con pruebas simples.
    void Update(float deltaTime) override;
    // collisionObjects son objetos genericos con hitbox, no necesariamente trenes.
    void Update(float deltaTime, const std::vector<GameObject*>& collisionObjects);

    // Comandos de entrada por carril.
    void MoveLeft();
    void MoveRight();
    void Jump();
    void Reset();

    // Tamano actual de la hitbox. El renderer temporal lo usa para escalar el cubo.
    glm::vec3 GetCollisionSize() const { return GetHitboxSize(); }
    bool IsJumping() const { return isJumping; }
    bool IsOnObject() const { return isOnObject; }
    bool HasCrashed() const { return hasCrashed; }

private:
    // targetX permite interpolar suavemente al cambiar de carril.
    float targetX;
    int currentLane;

    // Estado fisico vertical. X/Z se controlan por carriles y por el avance de objetos.
    glm::vec3 velocity;
    bool isGrounded;
    bool isJumping;
    bool isOnObject;
    bool hasCrashed;

    const float jumpForce = 8.0f;
    const float gravity = 20.0f;
    const float laneSpeed = 15.0f;

    // Helpers internos separados para mantener Update legible.
    void UpdateLaneMovement(float deltaTime);
    void UpdatePhysics(float deltaTime, const std::vector<GameObject*>& collisionObjects);
    // Soporte usa solape X/Z y el techo mas alto bajo la hitbox del player.
    const GameObject* FindObjectUnderPlayer(const std::vector<GameObject*>& collisionObjects) const;
    // Bloqueo usa interseccion AABB completa para detectar choque contra el cuerpo.
    const GameObject* FindBlockingObject(const std::vector<GameObject*>& collisionObjects) const;
};

#endif
