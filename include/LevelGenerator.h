#ifndef LEVEL_GENERATOR_H
#define LEVEL_GENERATOR_H

#include "GameObject.h"
#include "Train.h"
#include "ObstacleOverhead.h"
#include "RampTrain.h"
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

    std::vector<GameObject*> GetCollisionObjects();

private:
    std::vector<Train> trains;
    std::vector<ObstacleOverhead> overheads;
    std::vector<RampTrain> ramps;

    float nextSpawnZ;
    int trainsSpawned;

    std::mt19937 rng;

    void SpawnPattern(float playerZ, float gameTime);
    void Cleanup(float playerZ);
    int PickLane();

    static constexpr float kSpawnDistance = 80.0f;
    static constexpr float kMinSpacing = 25.0f;
    static constexpr float kDespawnDistance = 10.0f;
    static constexpr float kTrainWidth = 1.20f;
    static constexpr float kTrainHeight = 1.6f;
    static constexpr float kTrainDepth = 10.0f;
    static constexpr float kOverheadHeight = 1.5f;
    static constexpr float kOverheadSizeX = 2.5f;
    static constexpr float kOverheadSizeZ = 1.0f;
    static constexpr float kRampWidth = 1.20f;
    static constexpr float kRampHeight = 1.6f;
    static constexpr float kRampDepth = 10.0f;
};

#endif
