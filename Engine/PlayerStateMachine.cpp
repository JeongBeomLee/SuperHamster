#include "pch.h"
#include "PlayerStateMachine.h"
#include "Animator.h"

PlayerStateMachine::PlayerStateMachine(shared_ptr<Animator> animator)
    : currentState(PLAYER_STATE::IDLE), previousState(PLAYER_STATE::IDLE),
    animator(animator), stateTimer(0.0f)
{
}

void PlayerStateMachine::Update(const InputState& input, float deltaTime)
{
    stateTimer += deltaTime;

    switch (currentState) {
    case PLAYER_STATE::IDLE:
        UpdateIdleState(input);
        break;
    case PLAYER_STATE::WALK:
        UpdateWalkState(input);
        break;
    case PLAYER_STATE::RUN_SLOW:
        UpdateRunSlowState(input);
        break;
    case PLAYER_STATE::RUN_FAST:
        UpdateRunFastState(input);
        break;
    case PLAYER_STATE::ROLL:
        UpdateRollState(input, deltaTime);
        break;
    case PLAYER_STATE::AIM:
        UpdateAimState(input);
        break;
    case PLAYER_STATE::FIRE:
        UpdateFireState(input, deltaTime);
        break;
    }
}

void PlayerStateMachine::TransitionTo(PLAYER_STATE newState)
{
    if (CanTransitionTo(newState)) {
        previousState = currentState;
        currentState = newState;
        stateTimer = 0.0f;
        animator->Play(static_cast<uint32>(currentState));
    }
}

void PlayerStateMachine::UpdateIdleState(const InputState& input)
{
    if (input.aim)
        TransitionTo(PLAYER_STATE::AIM);
    else if (input.roll)
        TransitionTo(PLAYER_STATE::ROLL);
    else if (input.up || input.down || input.left || input.right)
        TransitionTo(PLAYER_STATE::WALK);
}



bool PlayerStateMachine::CanTransitionTo(PLAYER_STATE newState) const
{
    // 상태 전이 규칙 정의
    switch (currentState) {
    case PLAYER_STATE::ROLL:
        return stateTimer > 0.5f; // Roll 동작이 0.5초 이상 지속된 경우에만 전환 가능
    case PLAYER_STATE::FIRE:
        return stateTimer > 0.2f; // Fire 동작이 0.2초 이상 지속된 경우에만 전환 가능
    default:
        return true;
    }
}