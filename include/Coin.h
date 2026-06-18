#ifndef COIN_H
#define COIN_H

#include "GameObject.h"

class Coin : public GameObject {
public:
    Coin(const glm::vec3& position);

    bool IsCollected() const { return collected; }
    void Collect() { collected = true; }

private:
    bool collected = false;
};

#endif
