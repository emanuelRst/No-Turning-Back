#include "Train.h"

namespace {
constexpr float kLaneWidth = 2.0f;

// Convierte el indice de carril al eje X del mundo logico.
float LaneX(int lane) {
    return (lane - 1) * kLaneWidth;
}
}

Train::Train()
    : GameObject(glm::vec3(0.0f), glm::vec3(1.0f)), lane(1), speed(0.0f) {}

Train::Train(int lane, float zPosition, const glm::vec3& hitboxSize, float speed)
    // Y se coloca a la mitad de la altura para que la base de la hitbox toque el suelo.
    : GameObject(glm::vec3(LaneX(lane), hitboxSize.y * 0.5f, zPosition), hitboxSize),
      lane(lane),
      speed(speed) {}

void Train::Update(float deltaTime) {
    // En este prototipo los trenes avanzan por Z positiva hacia el jugador/camara.
    Move(glm::vec3(0.0f, 0.0f, speed * deltaTime));
}

void Train::Reset(int newLane, float zPosition, const glm::vec3& hitboxSize, float newSpeed) {
    // Reset mantiene el objeto vivo y solo cambia su estado. Esto evita crear/destruir
    // trenes constantemente y deja preparado el flujo para modelos futuros.
    lane = newLane;
    speed = newSpeed;
    SetHitboxSize(hitboxSize);
    SetPosition(glm::vec3(LaneX(lane), hitboxSize.y * 0.5f, zPosition));
}
