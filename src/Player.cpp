#include "Player.h"
#include <algorithm>
#include <iostream>

Player::Player() 
    : position(0.0f, 0.0f, -5.0f), targetX(0.0f), currentLane(1), 
      velocity(0.0f), isGrounded(true), isJumping(false) {}

void Player::Update(float deltaTime) {
    UpdateLaneMovement(deltaTime);
    UpdatePhysics(deltaTime);
}

void Player::UpdateLaneMovement(float deltaTime) {
    // Interpola suavemente (Lerp) la posición actual hacia targetX
    float lerpSpeed = laneSpeed * deltaTime;
    position.x += (targetX - position.x) * lerpSpeed;
}

void Player::UpdatePhysics(float deltaTime) {
    if (isJumping) {
        // Aplicar gravedad
        velocity.y -= gravity * deltaTime;
        position.y += velocity.y * deltaTime;

        // Comprobar si ha llegado al suelo
        if (position.y <= 0.0f) {
            position.y = 0.0f;
            velocity.y = 0.0f;
            isJumping = false;
            isGrounded = true;
        }
    }
}

void Player::MoveLeft() {
    if (currentLane > 0) {
        currentLane--;
        targetX = (currentLane - 1) * 2.0f; // -2.0, 0.0, 2.0
    }
}

void Player::MoveRight() {
    if (currentLane < 2) {
        currentLane++;
        targetX = (currentLane - 1) * 2.0f;
    }
}

void Player::Jump() {
    if (isGrounded) {
        isJumping = true;
        isGrounded = false;
        velocity.y = jumpForce;
    }
}
