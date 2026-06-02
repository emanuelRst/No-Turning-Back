#include "GameObject.h"

GameObject::GameObject(const glm::vec3& startPosition, const glm::vec3& hitboxSize)
    : position(startPosition) {
    // Por defecto la hitbox queda centrada en position y solo se define su tamano.
    hitbox.size = hitboxSize;
}

void GameObject::Update(float deltaTime) {
    // Base sin comportamiento. Las clases hijas sobreescriben Update si se mueven.
    (void)deltaTime;
}

Bounds GameObject::GetBounds() const {
    // Convertimos la hitbox local a una caja AABB en mundo usando position + offset.
    const glm::vec3 center = position + hitbox.centerOffset;
    const glm::vec3 halfSize = hitbox.size * 0.5f;
    return {center - halfSize, center + halfSize};
}

bool RangesOverlap(float minA, float maxA, float minB, float maxB) {
    // Permite igualdad para que aterrizar justo en el borde cuente como contacto.
    return maxA >= minB && minA <= maxB;
}

bool RangesIntersect(float minA, float maxA, float minB, float maxB) {
    // Usa comparacion estricta para detectar penetracion, no solo contacto.
    return maxA > minB && minA < maxB;
}

bool BoundsIntersect(const Bounds& a, const Bounds& b) {
    // Dos AABB chocan si sus rangos se intersectan en los tres ejes.
    return RangesIntersect(a.min.x, a.max.x, b.min.x, b.max.x) &&
           RangesIntersect(a.min.y, a.max.y, b.min.y, b.max.y) &&
           RangesIntersect(a.min.z, a.max.z, b.min.z, b.max.z);
}
