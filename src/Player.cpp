#include "Player.h"
#include <algorithm>
#include <limits>
#include <vector>

namespace {
constexpr float kLaneWidth = 2.0f;

// Hitbox inicial del jugador. Cuando haya modelo, esta caja puede ajustarse
// al cuerpo real sin cambiar la logica de Player.
const glm::vec3 kPlayerHitboxSize(0.9f, 1.1f, 0.9f);

// Convierte carril logico (0, 1, 2) a posicion X.
float LaneX(int lane) {
    return (lane - 1) * kLaneWidth;
}
}

Player::Player()
    : GameObject(glm::vec3(0.0f), kPlayerHitboxSize),
      targetX(0.0f),
      currentLane(1),
      velocity(0.0f),
      isGrounded(true),
      isJumping(false),
      isOnObject(false),
      hasCrashed(false) {
    Reset();
}

void Player::Update(float deltaTime) {
    // Ruta simple para actualizar sin objetos de colision externos.
    static const std::vector<GameObject*> noObjects;
    Update(deltaTime, noObjects);
}

void Player::Update(float deltaTime, const std::vector<GameObject*>& collisionObjects) {
    // Despues de un choque se congela la simulacion hasta que Game haga ResetRun().
    if (hasCrashed) {
        return;
    }

    UpdateLaneMovement(deltaTime);
    UpdatePhysics(deltaTime, collisionObjects);
}

void Player::UpdateLaneMovement(float deltaTime) {
    // Lerp limitado para evitar overshoot cuando deltaTime sube por un bajon de FPS.
    const float lerpSpeed = std::min(laneSpeed * deltaTime, 1.0f);
    position.x += (targetX - position.x) * lerpSpeed;
}

void Player::UpdatePhysics(float deltaTime, const std::vector<GameObject*>& collisionObjects) {
    // previousBottom es el borde inferior antes de aplicar gravedad.
    // Se usa para saber si cruzo el techo de un objeto desde arriba.
    const float previousBottom = GetBounds().min.y;
    const GameObject* support = FindObjectUnderPlayer(collisionObjects);
    const float groundY = 0.0f;

    isOnObject = false;

    // Si estaba sobre un objeto y ya no hay solape debajo, empieza a caer.
    if (isGrounded && previousBottom > groundY && support == nullptr) {
        isGrounded = false;
        isJumping = true;
    }

    // Mientras no este apoyado, solo se integra la velocidad vertical.
    if (!isGrounded) {
        velocity.y -= gravity * deltaTime;
        Move(glm::vec3(0.0f, velocity.y * deltaTime, 0.0f));
    }

    // Aterrizaje: el borde inferior del player cruza el techo del objeto.
    // Esto evita usar el centro del player como punto de contacto.
    support = FindObjectUnderPlayer(collisionObjects);
    if (support != nullptr && velocity.y <= 0.0f) {
        const float objectTop = support->GetBounds().max.y;
        const float playerBottom = GetBounds().min.y;

        if (previousBottom >= objectTop && playerBottom <= objectTop) {
            Move(glm::vec3(0.0f, objectTop - playerBottom, 0.0f));
            velocity.y = 0.0f;
            isJumping = false;
            isGrounded = true;
            isOnObject = true;
            return;
        }
    }

    // Si ya estaba apoyado, se mantiene pegado al techo mientras siga el solape X/Z.
    if (isGrounded && support != nullptr) {
        const float objectTop = support->GetBounds().max.y;
        const float playerBottom = GetBounds().min.y;
        const float snapTolerance = 0.05f;

        if (playerBottom >= objectTop - snapTolerance && playerBottom <= objectTop + snapTolerance) {
            Move(glm::vec3(0.0f, objectTop - playerBottom, 0.0f));
            isOnObject = true;
            return;
        }
    }

    // El suelo se trata como plano fijo Y=0. Solo corrige el borde inferior.
    const float playerBottom = GetBounds().min.y;
    if (playerBottom <= groundY) {
        Move(glm::vec3(0.0f, groundY - playerBottom, 0.0f));
        velocity.y = 0.0f;
        isJumping = false;
        isGrounded = true;
    }

    // Si no aterrizo sobre el techo, una interseccion completa cuenta como choque.
    const GameObject* blocker = FindBlockingObject(collisionObjects);
    if (blocker != nullptr) {
        hasCrashed = true;
        velocity = glm::vec3(0.0f);
        isJumping = false;
        isGrounded = true;
        isOnObject = false;
    }
}

const GameObject* Player::FindObjectUnderPlayer(const std::vector<GameObject*>& collisionObjects) const {
    const Bounds playerBounds = GetBounds();
    const GameObject* highestObject = nullptr;
    float highestTop = -std::numeric_limits<float>::infinity();

    for (const GameObject* object : collisionObjects) {
        // El player solo considera objetos con hitbox activa y evita compararse consigo mismo.
        if (object == nullptr || object == this || !object->GetHitbox().enabled) {
            continue;
        }

        const Bounds objectBounds = object->GetBounds();
        // Para saber si algo esta debajo, solo interesa el solape horizontal X/Z.
        const bool overlapsX = RangesOverlap(playerBounds.min.x, playerBounds.max.x,
                                             objectBounds.min.x, objectBounds.max.x);
        const bool overlapsZ = RangesOverlap(playerBounds.min.z, playerBounds.max.z,
                                             objectBounds.min.z, objectBounds.max.z);

        if (!overlapsX || !overlapsZ) {
            continue;
        }

        // Si hay varios objetos bajo el player, usa el techo mas alto.
        if (highestObject == nullptr || objectBounds.max.y > highestTop) {
            highestObject = object;
            highestTop = objectBounds.max.y;
        }
    }

    return highestObject;
}

const GameObject* Player::FindBlockingObject(const std::vector<GameObject*>& collisionObjects) const {
    const Bounds playerBounds = GetBounds();
    const float topTolerance = 0.03f;

    for (const GameObject* object : collisionObjects) {
        if (object == nullptr || object == this || !object->GetHitbox().enabled) {
            continue;
        }

        const Bounds objectBounds = object->GetBounds();
        // Choque real: las dos cajas penetran en X, Y y Z.
        if (!BoundsIntersect(playerBounds, objectBounds)) {
            continue;
        }

        // Si el borde inferior del player ya esta sobre el techo, es soporte, no choque.
        if (playerBounds.min.y >= objectBounds.max.y - topTolerance) {
            continue;
        }

        return object;
    }

    return nullptr;
}

void Player::Reset() {
    // El centro del player se coloca a media altura para que su borde inferior toque el suelo.
    SetHitboxSize(kPlayerHitboxSize);
    SetPosition(glm::vec3(0.0f, kPlayerHitboxSize.y * 0.5f, -5.0f));
    targetX = 0.0f;
    currentLane = 1;
    velocity = glm::vec3(0.0f);
    isGrounded = true;
    isJumping = false;
    isOnObject = false;
    hasCrashed = false;
}

void Player::MoveLeft() {
    // No se acepta input despues de chocar.
    if (hasCrashed) {
        return;
    }

    if (currentLane > 0) {
        currentLane--;
        targetX = LaneX(currentLane);
    }
}

void Player::MoveRight() {
    // No se acepta input despues de chocar.
    if (hasCrashed) {
        return;
    }

    if (currentLane < 2) {
        currentLane++;
        targetX = LaneX(currentLane);
    }
}

void Player::Jump() {
    // Solo se puede saltar si hay soporte: suelo o techo de un objeto.
    if (hasCrashed) {
        return;
    }

    if (isGrounded) {
        isJumping = true;
        isGrounded = false;
        velocity.y = jumpForce;
    }
}
