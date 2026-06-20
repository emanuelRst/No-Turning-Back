#ifndef CHARACTER_STATE_H
#define CHARACTER_STATE_H

class Player;

enum class StateType { RUNNING, ROLLING };

class CharacterState {
public:
    virtual ~CharacterState() = default;
    virtual void Update(Player& player, float deltaTime) = 0;
    virtual void OnEnter(Player& player) = 0;
    virtual void OnExit(Player& player) = 0;
    virtual StateType GetType() const = 0;
};

#endif
