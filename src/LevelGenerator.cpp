#include "LevelGenerator.h"
#include <algorithm>
#include <cmath>

LevelGenerator::LevelGenerator()
    : rng(std::random_device{}())
{}

int LevelGenerator::PickLane() {
    std::uniform_int_distribution<int> dist(0, 2);
    return dist(rng);
}

void LevelGenerator::Reset(float playerZ) {
    trains.clear();
    trains.reserve(100);
    overheads.clear();
    overheads.reserve(50);
    ramps.clear();
    ramps.reserve(50);
    coins.clear();
    coins.reserve(100);

    lastSections[0] = -1;
    lastSections[1] = -1;

    float z = playerZ - kSpawnDistance;
    for (int i = 0; i < 2; ++i) {
        SpawnPattern(z, 0.0f);
        z -= kMinSpacing;
    }

    RebuildCollisionCache();
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
    for (Coin& coin : coins) {
        coin.Move(glm::vec3(0.0f, 0.0f, moveAmount));
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

    float targetZ = playerZ - kSpawnDistance - kMinSpacing;
    if (farthestZ > targetZ) {
        SpawnPattern(farthestZ - kMinSpacing, gameTime);
    }

    // Cleanup obstacles that passed behind the player
    Cleanup(playerZ);

    // Actualizar cache de colisiones
    RebuildCollisionCache();
}

void LevelGenerator::SpawnPattern(float spawnZ, float gameTime) {

    int available[5];
    int count = 0;
    for (int s = 0; s < 5; ++s) {
        if (s != lastSections[0] && s != lastSections[1]) {
            available[count++] = s;
        }
    }

    int chosen = available[std::uniform_int_distribution<int>(0, count - 1)(rng)];

    lastSections[1] = lastSections[0];
    lastSections[0] = chosen;

    switch (chosen) {
    case 0: {
        // Seccion 1: 2 trenes laterales + overhead central + monedas
        trains.emplace_back(0, spawnZ,
            glm::vec3(kTrainWidth, kTrainHeight, kTrainDepth), 0.0f);
        trains.emplace_back(2, spawnZ,
            glm::vec3(kTrainWidth, kTrainHeight, kTrainDepth), 0.0f);
        overheads.emplace_back(
            glm::vec3(0.0f, kOverheadHeight, spawnZ - 10.0f),
            glm::vec3(kOverheadSizeX, 0.5f, kOverheadSizeZ));
        coins.emplace_back(glm::vec3(0.0f, 0.5f, spawnZ - 12.0f));
        coins.emplace_back(glm::vec3(0.0f, 0.5f, spawnZ - 8.0f));
        break;
    }
    case 1: {
        // Seccion 2: 3 overheads + rampa + 3 trenes + monedas
        for (int lane = 0; lane < 3; lane++) {
            overheads.emplace_back(
                glm::vec3((lane - 1) * 3.0f, kOverheadHeight, spawnZ + 25.0f),
                glm::vec3(kOverheadSizeX, 0.5f, kOverheadSizeZ));
        }
        ramps.emplace_back(
            glm::vec3(0.0f, 0.0f, spawnZ + 10.0f),
            glm::vec3(kRampWidth, kRampHeight, kRampDepth));
        for (int lane = 0; lane < 3; lane++) {
            trains.emplace_back(lane, spawnZ,
                glm::vec3(kTrainWidth, kTrainHeight, kTrainDepth), 0.0f);
        }
        for (int i = 0; i < 3; i++) {
            coins.emplace_back(
                glm::vec3(0.0f, 2.8f, spawnZ - 3.0f + i * 3.0f));
        }
        break;
    }
    case 2: {
        // Seccion 3: 2 overheads laterales + tren central + monedas
        overheads.emplace_back(
            glm::vec3(-3.0f, kOverheadHeight, spawnZ + 12.0f),
            glm::vec3(kOverheadSizeX, 0.5f, kOverheadSizeZ));
        overheads.emplace_back(
            glm::vec3(3.0f, kOverheadHeight, spawnZ + 12.0f),
            glm::vec3(kOverheadSizeX, 0.5f, kOverheadSizeZ));
        trains.emplace_back(1, spawnZ,
            glm::vec3(kTrainWidth, kTrainHeight, kTrainDepth), 0.0f);
        coins.emplace_back(glm::vec3(0.0f, 0.5f, spawnZ + 14.0f));
        coins.emplace_back(glm::vec3(-3.0f, 0.5f, spawnZ + 6.0f));
        coins.emplace_back(glm::vec3(3.0f, 0.5f, spawnZ + 6.0f));
        break;
    }
    case 3: {
        // Seccion 4: rampa al frente, trenes anchos medio, overheads justo antes, monedas
        ramps.emplace_back(
            glm::vec3(0.0f, 0.0f, spawnZ + 15.0f),
            glm::vec3(kRampWidth, kRampHeight, kRampDepth));
        trains.emplace_back(1, spawnZ + 5.0f,
            glm::vec3(kTrainWidth, kTrainHeight, kTrainDepth), 0.0f);
        overheads.emplace_back(
            glm::vec3(-3.0f, kOverheadHeight, spawnZ + 12.0f),
            glm::vec3(kOverheadSizeX, 0.5f, kOverheadSizeZ));
        overheads.emplace_back(
            glm::vec3(3.0f, kOverheadHeight, spawnZ + 12.0f),
            glm::vec3(kOverheadSizeX, 0.5f, kOverheadSizeZ));
        trains.emplace_back(0, spawnZ + 5.0f,
            glm::vec3(kTrainWidth, kTrainHeight, kTrainDepth), 0.0f);
        trains.emplace_back(2, spawnZ + 5.0f,
            glm::vec3(kTrainWidth, kTrainHeight, kTrainDepth), 0.0f);
        for (int i = 0; i < 3; i++) {
            coins.emplace_back(
                glm::vec3(3.0f, 2.8f, spawnZ + 3.0f + i * 1.5f));
        }
        break;
    }
    default: {
        // Seccion 5: trenes izq encadenados + rampa, overhead+monedas centro, tren+monedas der
        trains.emplace_back(0, spawnZ,
            glm::vec3(kTrainWidth, kTrainHeight, kTrainDepth), 0.0f);
        trains.emplace_back(0, spawnZ + 10.0f,
            glm::vec3(kTrainWidth, kTrainHeight, kTrainDepth), 0.0f);
        ramps.emplace_back(
            glm::vec3(-3.0f, 0.0f, spawnZ + 20.0f),
            glm::vec3(kRampWidth, kRampHeight, kRampDepth));
        overheads.emplace_back(
            glm::vec3(0.0f, kOverheadHeight, spawnZ + 10.0f),
            glm::vec3(kOverheadSizeX, 0.5f, kOverheadSizeZ));
        for (int i = 0; i < 3; i++) {
            coins.emplace_back(
                glm::vec3(0.0f, 0.5f, spawnZ + 7.0f + i * 3.0f));
        }
        trains.emplace_back(2, spawnZ - 20.0f,
            glm::vec3(kTrainWidth, kTrainHeight, kTrainDepth), 0.0f);
        for (int i = 0; i < 3; i++) {
            coins.emplace_back(
                glm::vec3(3.0f, 2.8f, spawnZ - 22.0f + i * 2.0f));
        }
        trains.emplace_back(2, spawnZ - 10.0f,
            glm::vec3(kTrainWidth, kTrainHeight, kTrainDepth), 0.0f);
        for (int i = 0; i < 3; i++) {
            coins.emplace_back(
                glm::vec3(3.0f, 2.8f, spawnZ - 12.0f + i * 2.0f));
        }
        break;
    }
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

    auto coinPast = [&](const Coin& c) {
        return c.GetPosition().z > playerZ + kDespawnDistance;
    };
    coins.erase(std::remove_if(coins.begin(), coins.end(), coinPast), coins.end());
}

void LevelGenerator::RebuildCollisionCache() {
    collisionCache.clear();
    collisionCache.reserve(trains.size() + overheads.size() + ramps.size());
    for (RampTrain& ramp : ramps) {
        collisionCache.push_back(&ramp);
    }
    for (ObstacleOverhead& obs : overheads) {
        collisionCache.push_back(&obs);
    }
    for (Train& train : trains) {
        collisionCache.push_back(&train);
    }
}
