#ifndef STATES_H
#define STATES_H

#include "CharacterState.h"
#include <memory>
#include <glm/glm.hpp>

class RunningState : public CharacterState {
public:
    RunningState();
    virtual ~RunningState();
    void OnEnter(Player& player) override;
    void Update(Player& player, float deltaTime) override;
    void OnExit(Player& player) override;
    StateType GetType() const override { return StateType::RUNNING; }
};

class RollingState : public CharacterState {
public:
    RollingState();
    virtual ~RollingState();
    void OnEnter(Player& player) override;
    void Update(Player& player, float deltaTime) override;
    void OnExit(Player& player) override;
    StateType GetType() const override { return StateType::ROLLING; }
    static constexpr float kDuration = 0.6f;
private:
    float timer;
    glm::vec3 originalHitboxSize;
};

#endif
