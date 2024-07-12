#pragma once
class ServerPlayerStateMachine {
public:
    ServerPlayerStateMachine() : currentState(PLAYER_STATE::IDLE), previousState(PLAYER_STATE::IDLE),
        stateTimer(0.0f), position(PxVec3(0, 0, 0)), direction(PxVec3(0, 0, 1)), velocity(0.0f) {}

    void Update(const PlayerInput& input, float deltaTime, PxController* controller);
    PLAYER_STATE GetCurrentState() const { return currentState; }
    PxVec3 GetPosition() const { return position; }
    PxVec3 GetDirection() const { return direction; }
    float GetVelocity() const { return velocity; }

private:
    void UpdateIdleState(const PlayerInput& input, PxController* controller);
    void UpdateWalkState(const PlayerInput& input, PxController* controller);
    void UpdateRunSlowState(const PlayerInput& input, PxController* controller);
    void UpdateRunFastState(const PlayerInput& input, PxController* controller);
    void UpdateRollState(const PlayerInput& input, float deltaTime, PxController* controller);
    void UpdateAimState(const PlayerInput& input, PxController* controller);
    void UpdateFireState(const PlayerInput& input, float deltaTime, PxController* controller);
    void UpdateMovement(const PlayerInput& input, float speedMultiplier, PxController* controller);

    void TransitionTo(PLAYER_STATE newState);

    PLAYER_STATE currentState;
    PLAYER_STATE previousState;
    float stateTimer;
    PxVec3 position;
    PxVec3 direction;
    float velocity;
};