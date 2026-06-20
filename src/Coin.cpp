#include "Coin.h"

Coin::Coin(const glm::vec3& startPosition)
    : GameObject(startPosition, glm::vec3(1.3f, 1.0f, 0.6f))
    , collected(false)
{
}
