#include "pch.h"
#include "PlayerScript.h"
#include "Input.h"
#include "Animator.h"
#include "Transform.h"
#include "Timer.h"
#include "Engine.h"
#include "SceneManager.h"
#include "Scene.h"
#include "GameObject.h"
#include "PlayerStateMachine.h"

PlayerScript::PlayerScript()
    : stateMachine(make_unique<PlayerStateMachine>(GetAnimator())),
    velocity(0.0f)
{
}

//void PlayerScript::Update()
//{
//    BYTE inputDirection = 0;
//
//    if (INPUT->GetButton(KEY_TYPE::UP))
//        inputDirection |= 1;
//    if (INPUT->GetButton(KEY_TYPE::DOWN))
//        inputDirection |= 2;
//    if (INPUT->GetButton(KEY_TYPE::RIGHT))
//        inputDirection |= 4;
//    if (INPUT->GetButton(KEY_TYPE::LEFT))
//        inputDirection |= 8;
//    if (INPUT->GetButton(KEY_TYPE::A))
//        inputDirection |= 16;
//    if (INPUT->GetButtonDown(KEY_TYPE::S))
//        inputDirection |= 32;
//    if (INPUT->GetButtonDown(KEY_TYPE::SPACE))
//        inputDirection |= 64;
//
//    CS_MOVE_PACKET movePacket;
//    movePacket.size = sizeof(movePacket);
//    movePacket.type = CS_MOVE;
//    movePacket.direction = inputDirection;
//    movePacket.prevDirection = m_PrevDirection;
//
//    // 서버로 이동 패킷 전송
//    send_packet(&movePacket);
//
//    m_PrevDirection = inputDirection;
//
//    // 상태에 따른 애니메이션 변경
//    if (m_PrevState != m_CurrentState) {
//        shared_ptr<Animator> animator = GetAnimator();
//        if (animator) {
//            animator->Play(m_CurrentState);
//        }
//        m_PrevState = m_CurrentState;
//    }
//}

void PlayerScript::Update()
{
    UpdateInputState();
    stateMachine->Update(inputState, DELTA_TIME);
    SendMovePacket();
}

void PlayerScript::UpdateInputState()
{
    inputState.up    = INPUT->GetButton(KEY_TYPE::UP);
    inputState.down  = INPUT->GetButton(KEY_TYPE::DOWN);
    inputState.left  = INPUT->GetButton(KEY_TYPE::LEFT);
    inputState.right = INPUT->GetButton(KEY_TYPE::RIGHT);
    inputState.aim   = INPUT->GetButton(KEY_TYPE::A);
    inputState.fire  = INPUT->GetButtonDown(KEY_TYPE::S);
    inputState.roll  = INPUT->GetButtonDown(KEY_TYPE::SPACE);
}

void PlayerScript::SendMovePacket()
{
    CS_MOVE_PACKET movePacket;
    movePacket.size = sizeof(movePacket);
    movePacket.type = CS_MOVE;
    movePacket.direction = 0;
    if (inputState.up)    movePacket.direction |= 1;
    if (inputState.down)  movePacket.direction |= 2;
    if (inputState.right) movePacket.direction |= 4;
    if (inputState.left)  movePacket.direction |= 8;
    if (inputState.aim)   movePacket.direction |= 16;
    if (inputState.fire)  movePacket.direction |= 32;
    if (inputState.roll)  movePacket.direction |= 64;

    send_packet(&movePacket);
}