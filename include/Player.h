#ifndef PLAYER_H
#define PLAYER_H

#include "GameObject.h"
#include "Model.h"
#include "CharacterState.h"
#include <vector>
#include <memory>

// Player es otro GameObject: su modelo visual puede cambiar, pero la fisica
// solo necesita su posicion, velocidad e hitbox.
class Player : public GameObject {
public:
    enum class AnimState { Run, Jump, Fall, Hit, Die, Roll, Dance };

    // Altura objetivo del jugador en unidades del mundo. El modelo se escala
    // dinámicamente desde su AABB para alcanzar esta altura.
    static constexpr float kTargetHeight = 1.3f;

    // Escala visual calculada al cargar el modelo desde su AABB.
    float GetVisualScale() const { return visualScale; }

    Player();

    void SetState(std::unique_ptr<CharacterState> newState);

    // Update sin objetos mantiene compatibilidad con pruebas simples.
    void Update(float deltaTime) override;
    // collisionObjects son objetos genericos con hitbox, no necesariamente trenes.
    void Update(float deltaTime, const std::vector<GameObject*>& collisionObjects);

    // Comandos de entrada por carril.
    void MoveLeft();
    void MoveRight();
    void Jump();
    void FastFall();
    void Roll();
    void InputS(); // Nueva funcion de entrada
    void Reset();

    // Tamano actual de la hitbox. El renderer temporal lo usa para escalar el cubo.
    glm::vec3 GetCollisionSize() const { return GetHitboxSize(); }
    bool IsJumping() const { return isJumping; }
    bool IsOnObject() const { return isOnObject; }
    bool HasCrashed() const { return hasCrashed; }
    bool IsWeakened() const { return isWeakened; }
    float GetWeakenedTimer() const { return weakenedTimer; }
    bool GetHitFromLeft() const { return hitFromLeft; }
    AnimState GetAnimState() const;

public:
    // Ajusta la hitbox del jugador usando un AABB calculado del modelo.
    // Esto permite que la física coincida con el mesh visual.
    void SetHitboxFromModelAABB(const ModelAABB& aabb);

private:
    // targetX permite interpolar suavemente al cambiar de carril.
    float targetX;
    int currentLane;
    int previousLane;

    // Estado fisico vertical. X/Z se controlan por carriles y por el avance de objetos.
    glm::vec3 velocity;
    bool isGrounded;
    bool isJumping;
    bool isOnObject;
    bool hasCrashed;
    bool isWeakened;
    float weakenedTimer;
    float lateralCooldownTimer = 0.0f;
    bool hitFromLeft = false;

    float visualScale = 0.7f;

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

    std::unique_ptr<CharacterState> currentState;
};

#endif
