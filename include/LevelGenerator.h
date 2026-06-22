#ifndef LEVEL_GENERATOR_H
#define LEVEL_GENERATOR_H

#include "GameObject.h"
#include "Train.h"
#include "ObstacleOverhead.h"
#include "RampTrain.h"
#include "Coin.h"
#include <vector>
#include <random>

class LevelGenerator {
public:
    LevelGenerator();
    ~LevelGenerator() = default;

    void Update(float deltaTime, float currentSpeed, float playerZ, float gameTime);
    void Reset(float playerZ);

    const std::vector<Train>& GetTrains() const { return trains; }
    const std::vector<ObstacleOverhead>& GetOverheads() const { return overheads; }
    const std::vector<RampTrain>& GetRamps() const { return ramps; }
    std::vector<Coin>& GetCoins() { return coins; }

    const std::vector<GameObject*>& GetCollisionObjects() { return collisionCache; }

    static constexpr float kSpawnDistance = 60.0f;
    static constexpr float kMinSpacing = 50.0f;
    static constexpr float kDespawnDistance = 10.0f;
    static constexpr float kTrainWidth = 2.5f;
    static constexpr float kTrainHeight = 2.5f;
    static constexpr float kTrainDepth = 10.0f;
    static constexpr float kOverheadHeight = 1.55f;
    static constexpr float kOverheadSizeX = 2.5f;
    static constexpr float kOverheadSizeY = 1.3f;
    static constexpr float kOverheadSizeZ = 1.0f;
    static constexpr float kRampWidth = 2.5f;
    static constexpr float kRampHeight = 2.5f;
    static constexpr float kRampDepth = 10.0f;

private:
    std::vector<Train> trains;
    std::vector<ObstacleOverhead> overheads;
    std::vector<RampTrain> ramps;
    std::vector<Coin> coins;

    std::mt19937 rng;
    int lastSections[2] = {-1, -1};

    std::vector<GameObject*> collisionCache;

    void SpawnPattern(float spawnZ, float gameTime);
    void Cleanup(float playerZ);
    int PickLane();
    void RebuildCollisionCache();
};

#endif
