#pragma once

class Animator;
class PlayerStateMachine
{
public:
    PlayerStateMachine(shared_ptr<Animator> animator);

    void Update(const InputState& input, float deltaTime);
    void TransitionTo(PLAYER_STATE newState);
    PLAYER_STATE GetCurrentState() const { return currentState; }

private:
    PLAYER_STATE currentState;
    PLAYER_STATE previousState;
    shared_ptr<Animator> animator;
    float stateTimer;

    void UpdateIdleState(const InputState& input);
    void UpdateWalkState(const InputState& input);
    void UpdateRunSlowState(const InputState& input);
    void UpdateRunFastState(const InputState& input);
    void UpdateRollState(const InputState& input, float deltaTime);
    void UpdateAimState(const InputState& input);
    void UpdateFireState(const InputState& input, float deltaTime);

    bool CanTransitionTo(PLAYER_STATE newState) const;
};