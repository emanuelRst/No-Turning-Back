#include "Coin.h"

Coin::Coin(const glm::vec3& startPosition)
    : GameObject(startPosition, glm::vec3(0.5f, 0.5f, 0.5f))
    , collected(false)
{
}
