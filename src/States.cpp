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
    Hitbox hb = player.GetHitbox();
    hb.size = originalHitboxSize;
    hb.size.y *= 0.5f;
    hb.centerOffset.y = hb.size.y * 0.5f;
    player.SetHitbox(hb);
}

void RollingState::Update(Player& player, float deltaTime) {
    timer += deltaTime;
    if (timer >= kDuration) {
        player.SetState(std::make_unique<RunningState>());
    }
}

void RollingState::OnExit(Player& player) {
    Hitbox hb = player.GetHitbox();
    hb.size = originalHitboxSize;
    hb.centerOffset.y = originalHitboxSize.y * 0.5f;
    player.SetHitbox(hb);
}
