#include "LevelGenerator.h"
#include <algorithm>
#include <cmath>

LevelGenerator::LevelGenerator()
    : nextSpawnZ(0.0f)
    , trainsSpawned(0)
    , rng(std::random_device{}())
{}

int LevelGenerator::PickLane() {
    std::uniform_int_distribution<int> dist(0, 2);
    return dist(rng);
}

void LevelGenerator::Reset(float playerZ) {
    trains.clear();
    overheads.clear();
    ramps.clear();

    nextSpawnZ = playerZ - kSpawnDistance;
    trainsSpawned = 0;

    // Spawn initial set of patterns
    for (int i = 0; i < 4; ++i) {
        SpawnPattern(playerZ, 0.0f);
    }
}

void LevelGenerator::Update(float deltaTime, float currentSpeed, float playerZ, float gameTime) {
    // Move all obstacles toward the player
    float moveAmount = currentSpeed * deltaTime;
    for (Train& train : trains) {
        train.SetSpeed(currentSpeed);
        train.Update(deltaTime);
    }
    for (ObstacleOverhead& obs : overheads) {
        obs.Move(glm::vec3(0.0f, 0.0f, moveAmount));
    }
    for (RampTrain& ramp : ramps) {
        ramp.Move(glm::vec3(0.0f, 0.0f, moveAmount));
    }

    // Spawn new patterns when there is room ahead
    float farthestZ = playerZ - kSpawnDistance;
    for (const Train& train : trains) {
        farthestZ = std::min(farthestZ, train.GetPosition().z);
    }
    for (const ObstacleOverhead& obs : overheads) {
        farthestZ = std::min(farthestZ, obs.GetPosition().z);
    }
    for (const RampTrain& ramp : ramps) {
        farthestZ = std::min(farthestZ, ramp.GetPosition().z);
    }

    while (farthestZ > playerZ - kSpawnDistance - trainsSpawned * kMinSpacing) {
        SpawnPattern(playerZ, gameTime);
        farthestZ = playerZ - kSpawnDistance - trainsSpawned * kMinSpacing;
    }

    // Cleanup obstacles that passed behind the player
    Cleanup(playerZ);
}

void LevelGenerator::SpawnPattern(float playerZ, float gameTime) {
    float spawnZ = playerZ - kSpawnDistance - trainsSpawned * kMinSpacing;
    trainsSpawned++;

    // Difficulty factor: 0.0 at start, approaches 1.0 over time
    float difficulty = std::min(1.0f, gameTime / 120.0f);

    // Pattern weights shift with difficulty
    // Easy: single trains, overheads
    // Medium: doubles, ramps
    // Hard: combos
    float roll = std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
    float adjusted = roll - difficulty * 0.3f;

    if (adjusted < 0.25f) {
        // Single train on random lane
        trains.emplace_back(PickLane(), spawnZ,
            glm::vec3(kTrainWidth, kTrainHeight, kTrainDepth), 0.0f);
    } else if (adjusted < 0.45f) {
        // Two trains, different lanes
        int lane1 = PickLane();
        int lane2 = (lane1 + 1 + PickLane() % 2) % 3;
        trains.emplace_back(lane1, spawnZ,
            glm::vec3(kTrainWidth, kTrainHeight, kTrainDepth), 0.0f);
        trains.emplace_back(lane2, spawnZ - 2.0f,
            glm::vec3(kTrainWidth, kTrainHeight, kTrainDepth), 0.0f);
    } else if (adjusted < 0.60f) {
        // Overhead obstacle only
        overheads.emplace_back(
            glm::vec3((PickLane() - 1) * 3.0f, kOverheadHeight, spawnZ),
            glm::vec3(kOverheadSizeX, 0.5f, kOverheadSizeZ));
    } else if (adjusted < 0.75f) {
        // Overhead + train below
        int lane = PickLane();
        float laneX = (lane - 1) * 3.0f;
        overheads.emplace_back(
            glm::vec3(laneX, kOverheadHeight, spawnZ),
            glm::vec3(kOverheadSizeX, 0.5f, kOverheadSizeZ));
        trains.emplace_back(lane, spawnZ + 3.0f,
            glm::vec3(kTrainWidth, kTrainHeight, kTrainDepth), 0.0f);
    } else if (adjusted < 0.88f) {
        // Ramp train
        ramps.emplace_back(
            glm::vec3((PickLane() - 1) * 3.0f, 0.0f, spawnZ),
            glm::vec3(kRampWidth, kRampHeight, kRampDepth));
    } else {
        // Ramp + overhead combo (hard)
        int lane = PickLane();
        float laneX = (lane - 1) * 3.0f;
        ramps.emplace_back(
            glm::vec3(laneX, 0.0f, spawnZ),
            glm::vec3(kRampWidth, kRampHeight, kRampDepth));
        // Overhead on the ramp
        overheads.emplace_back(
            glm::vec3(laneX, kOverheadHeight, spawnZ - 2.0f),
            glm::vec3(kOverheadSizeX, 0.5f, kOverheadSizeZ));
    }
}

void LevelGenerator::Cleanup(float playerZ) {
    auto trainPast = [&](const Train& t) {
        return t.BackEdgeZ() > playerZ + kDespawnDistance;
    };
    trains.erase(std::remove_if(trains.begin(), trains.end(), trainPast), trains.end());

    auto obsPast = [&](const ObstacleOverhead& o) {
        return o.GetPosition().z > playerZ + kDespawnDistance;
    };
    overheads.erase(std::remove_if(overheads.begin(), overheads.end(), obsPast), overheads.end());

    auto rampPast = [&](const RampTrain& r) {
        return r.GetPosition().z > playerZ + kDespawnDistance;
    };
    ramps.erase(std::remove_if(ramps.begin(), ramps.end(), rampPast), ramps.end());
}

std::vector<GameObject*> LevelGenerator::GetCollisionObjects() {
    std::vector<GameObject*> objects;
    objects.reserve(trains.size() + overheads.size() + ramps.size());
    for (Train& train : trains) {
        objects.push_back(&train);
    }
    for (ObstacleOverhead& obs : overheads) {
        objects.push_back(&obs);
    }
    for (RampTrain& ramp : ramps) {
        objects.push_back(&ramp);
    }
    return objects;
}
