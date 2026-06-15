#include "States.h"
#include "Player.h"

// RunningState implementation
RunningState::RunningState() {}
RunningState::~RunningState() {}
void RunningState::OnEnter(Player& player) {}
void RunningState::Update(Player& player, float deltaTime) {
    // Basic running logic (mostly handled in Player::UpdatePhysics)
}
void RunningState::OnExit(Player& player) {}

// RollingState implementation
RollingState::RollingState() : timer(0.0f) {}
RollingState::~RollingState() {}

void RollingState::OnEnter(Player& player) {
    originalHitboxSize = player.GetHitboxSize();
    glm::vec3 newSize = originalHitboxSize;
    newSize.y *= 0.5f; // Reducción a la mitad
    player.SetHitboxSize(newSize);
}

void RollingState::Update(Player& player, float deltaTime) {
    timer += deltaTime;
    if (timer >= kDuration) {
        player.SetState(std::make_unique<RunningState>());
    }
}

void RollingState::OnExit(Player& player) {
    player.SetHitboxSize(originalHitboxSize);
}
