#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

#include <glm/glm.hpp>

// Bounds representa una caja AABB en coordenadas de mundo.
// Es el formato comun que usa la fisica para comparar bordes reales.
struct Bounds {
    glm::vec3 min;
    glm::vec3 max;
};

// Hitbox describe la caja de colision local de un objeto.
// centerOffset permite que un modelo visual futuro tenga su hitbox desplazada
// sin cambiar el origen/posicion principal del GameObject.
struct Hitbox {
    glm::vec3 centerOffset{0.0f};
    glm::vec3 size{1.0f};
    bool enabled{true};
};

// Base para cualquier entidad del juego con posicion e hitbox.
// La intencion es que Player, Train y futuros modelos cargados compartan
// esta interfaz, aunque cada uno se renderice de forma distinta.
class GameObject {
public:
    GameObject() = default;
    GameObject(const glm::vec3& startPosition, const glm::vec3& hitboxSize);
    virtual ~GameObject() = default;

    // Update es virtual para que cada tipo de objeto defina su propio movimiento.
    virtual void Update(float deltaTime);
    virtual bool IsRamp() const { return false; }

    // Transform basico del objeto en el mundo.
    const glm::vec3& GetPosition() const { return position; }
    void SetPosition(const glm::vec3& newPosition) { position = newPosition; }
    void Move(const glm::vec3& offset) { position += offset; }

    // Acceso a la hitbox. En el futuro, un loader de modelos puede asignar
    // una hitbox especifica al modelo sin afectar el resto de la logica.
    const Hitbox& GetHitbox() const { return hitbox; }
    glm::vec3 GetHitboxSize() const { return hitbox.size; }
    void SetHitbox(const Hitbox& newHitbox) { hitbox = newHitbox; }
    void SetHitboxSize(const glm::vec3& size) { hitbox.size = size; }

    // Calcula los bordes reales de la hitbox en mundo.
    Bounds GetBounds() const;
    float Bottom() const { return GetBounds().min.y; }
    float Top() const { return GetBounds().max.y; }

protected:
    // position representa el origen logico del objeto. Para las cajas actuales,
    // coincide con el centro de la hitbox.
    glm::vec3 position{0.0f};
    Hitbox hitbox{};
};

// Helpers compartidos por las colisiones. Overlap permite tocar bordes;
// Intersect requiere penetracion real.
bool RangesOverlap(float minA, float maxA, float minB, float maxB);
bool RangesIntersect(float minA, float maxA, float minB, float maxB);
bool BoundsIntersect(const Bounds& a, const Bounds& b);

#endif
