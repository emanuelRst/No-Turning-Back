#ifndef TRAIN_H
#define TRAIN_H

#include "GameObject.h"

// Train encapsula la logica propia de los trenes: carril, avance en Z y respawn.
// Sigue siendo un GameObject, asi que sus colisiones dependen solo de su hitbox.
class Train : public GameObject {
public:
    Train();
    Train(int lane, float zPosition, const glm::vec3& hitboxSize, float speed);

    // Mueve el tren hacia el jugador.
    void Update(float deltaTime) override;
    // Reutiliza el mismo objeto cuando ya paso al jugador.
    void Reset(int lane, float zPosition, const glm::vec3& hitboxSize, float speed);

    int GetLane() const { return lane; }
    // Borde trasero usado para saber cuando el tren ya salio de la zona jugable.
    float BackEdgeZ() const { return GetBounds().min.z; }

private:
    // lane es solo logica de posicionamiento; la colision real sale de GetBounds().
    int lane{1};
    float speed{0.0f};
};

#endif
