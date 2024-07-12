#include "stdafx.h"
#include "ServerPlayerStateMachine.h"

void ServerPlayerStateMachine::Update(const PlayerInput& input, float deltaTime, PxController* controller)
{
    stateTimer += deltaTime;

    switch (currentState) {
    case PLAYER_STATE::IDLE: 
        UpdateIdleState(input, controller); break;
    case PLAYER_STATE::WALK: 
        UpdateWalkState(input, controller); break;
    case PLAYER_STATE::RUN_SLOW: 
        UpdateRunSlowState(input, controller); break;
    case PLAYER_STATE::RUN_FAST: 
        UpdateRunFastState(input, controller); break;
    case PLAYER_STATE::ROLL: 
        UpdateRollState(input, deltaTime, controller); break;
    case PLAYER_STATE::AIM: 
        UpdateAimState(input, controller); break;
    case PLAYER_STATE::FIRE: 
        UpdateFireState(input, deltaTime, controller); break;
    }
}

void ServerPlayerStateMachine::UpdateIdleState(const PlayerInput& input, PxController* controller)
{
    if (input.aim) TransitionTo(PLAYER_STATE::AIM);
    else if (input.roll) TransitionTo(PLAYER_STATE::ROLL);
    else if (input.up || input.down || input.left || input.right) TransitionTo(PLAYER_STATE::WALK);
    velocity = 0.0f;
}

void ServerPlayerStateMachine::UpdateWalkState(const PlayerInput& input, PxController* controller)
{
    UpdateMovement(input, 1.0f, controller);
    if (velocity > 150.0f) TransitionTo(PLAYER_STATE::RUN_SLOW);
    else if (velocity <= 0.0f) TransitionTo(PLAYER_STATE::IDLE);
}

void ServerPlayerStateMachine::UpdateRunSlowState(const PlayerInput& input, PxController* controller)
{
    UpdateMovement(input, 2.0f, controller);
    if (velocity > 350.0f) TransitionTo(PLAYER_STATE::RUN_FAST);
    else if (velocity <= 150.0f) TransitionTo(PLAYER_STATE::WALK);
}

void ServerPlayerStateMachine::UpdateRunFastState(const PlayerInput& input, PxController* controller)
{
    UpdateMovement(input, 3.0f, controller);
    if (velocity <= 350.0f) TransitionTo(PLAYER_STATE::RUN_SLOW);
}

void ServerPlayerStateMachine::UpdateRollState(const PlayerInput& input, float deltaTime, PxController* controller)
{
    if (stateTimer > 0.5f) TransitionTo(PLAYER_STATE::IDLE);
}

void ServerPlayerStateMachine::UpdateAimState(const PlayerInput& input, PxController* controller)
{
    if (!input.aim) TransitionTo(PLAYER_STATE::IDLE);
    if (input.fire) TransitionTo(PLAYER_STATE::FIRE);
}

void ServerPlayerStateMachine::UpdateFireState(const PlayerInput& input, float deltaTime, PxController* controller)
{
    if (stateTimer > 0.2f) {
        if (input.aim) TransitionTo(PLAYER_STATE::AIM);
        else TransitionTo(PLAYER_STATE::IDLE);
    }
}

void ServerPlayerStateMachine::UpdateMovement(const PlayerInput& input, float speedMultiplier, PxController* controller)
{
    PxVec3 inputDir(0, 0, 0);
    if (input.up) inputDir.z += 1;
    if (input.down) inputDir.z -= 1;
    if (input.left) inputDir.x -= 1;
    if (input.right) inputDir.x += 1;

    if (inputDir.magnitudeSquared() > 0) {
        inputDir.normalize();
        direction = inputDir;
        velocity = PxMin(velocity + 1000.0f * 0.016f, 500.0f * speedMultiplier);
    }
    else {
        velocity = PxMax(velocity - 1000.0f * 0.016f, 0.0f);
    }

    PxVec3 displacement = direction * velocity * 0.016f;
    controller->move(PxVec3(displacement.x, 0, displacement.z), 0.0001f, 0.016f, PxControllerFilters());
    PxVec3 pos { controller->getPosition().x, controller->getPosition().y, controller->getPosition().z };
    position = pos;
}

void ServerPlayerStateMachine::TransitionTo(PLAYER_STATE newState)
{
    previousState = currentState;
    currentState = newState;
    stateTimer = 0.0f;
}
