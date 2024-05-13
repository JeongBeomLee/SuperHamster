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

PlayerScript::PlayerScript()
	: m_PrevDirection(0)
    , m_PrevState(PLAYER_STATE::CLIMB)
	, m_CurrentState(PLAYER_STATE::IDLE)
    , m_Velocity(0.0f)
{
}

void PlayerScript::Update()
{
    //Vec3 cameraRotation = GET_SINGLE(SceneManager)->GetActiveScene()->GetGameObjectByName(L"Main_Camera")->GetTransform()->GetLocalRotation();
    //float cosX = cos(cameraRotation.x);
    //float sinX = sin(cameraRotation.x);
    //float cosY = cos(cameraRotation.y);
    //float sinY = sin(cameraRotation.y);

    //BYTE inputDirection = 0;

    //if (INPUT->GetButton(KEY_TYPE::UP))
    //    inputDirection |= 1;
    //if (INPUT->GetButton(KEY_TYPE::DOWN))
    //    inputDirection |= 2;
    //if (INPUT->GetButton(KEY_TYPE::RIGHT))
    //    inputDirection |= 4;
    //if (INPUT->GetButton(KEY_TYPE::LEFT))
    //    inputDirection |= 8;

    //CS_MOVE_PACKET movePacket;
    //movePacket.size = sizeof(movePacket);
    //movePacket.type = CS_MOVE;
    //movePacket.direction = inputDirection;
    //movePacket.prevDirection = m_PrevDirection;

    //send_packet(&movePacket);

    //m_PrevDirection = inputDirection;

    //// 애니메이션 state 계산
    //if (INPUT->GetButton(KEY_TYPE::A) && m_CurrentState != PLAYER_STATE::AIM && m_CurrentState != PLAYER_STATE::FIRE && m_CurrentState != PLAYER_STATE::ROLL) {
    //    m_CurrentState = PLAYER_STATE::AIM;
    //    m_Velocity = 0.0f;
    //}

    //if (INPUT->GetButtonUp(KEY_TYPE::A)) {
    //    m_CurrentState = PLAYER_STATE::IDLE;
    //}

    //if ((m_CurrentState == PLAYER_STATE::AIM && INPUT->GetButtonDown(KEY_TYPE::S)) || (m_CurrentState == PLAYER_STATE::FIRE && INPUT->GetButtonDown(KEY_TYPE::S))) {
    //    m_CurrentState = PLAYER_STATE::FIRE;
    //}

    //if (m_CurrentState == PLAYER_STATE::FIRE) {
    //    if (INPUT->GetButton(KEY_TYPE::A)) {
    //        m_CurrentState = PLAYER_STATE::AIM;
    //    }
    //    else {
    //        m_CurrentState = PLAYER_STATE::IDLE;
    //    }
    //}

    //if (INPUT->GetButtonDown(KEY_TYPE::SPACE) && m_CurrentState != PLAYER_STATE::AIM && m_CurrentState != PLAYER_STATE::FIRE && m_CurrentState != PLAYER_STATE::ROLL) {
    //    m_CurrentState = PLAYER_STATE::ROLL;
    //}

    //if (m_CurrentState == PLAYER_STATE::ROLL) {
    //    if (m_Velocity <= 0.0f || GetAnimator()->IsAnimationFinished(PLAYER_STATE::ROLL))
    //        m_CurrentState = PLAYER_STATE::IDLE;
    //}

    //if (m_CurrentState != PLAYER_STATE::AIM && m_CurrentState != PLAYER_STATE::FIRE && m_CurrentState != PLAYER_STATE::ROLL) {
    //    if (m_Velocity <= 0.0f)
    //        m_CurrentState = PLAYER_STATE::IDLE;
    //    else if (m_Velocity > 0.0f && m_Velocity <= 150.0f)
    //        m_CurrentState = PLAYER_STATE::WALK;
    //    else if (m_Velocity > 150.0f && m_Velocity <= 350.0f)
    //        m_CurrentState = PLAYER_STATE::RUN_SLOW;
    //    else if (m_Velocity > 350.0f)
    //        m_CurrentState = PLAYER_STATE::RUN_FAST;
    //}

    //// 상태에 따른 애니메이션 변경
    //if (m_PrevState != m_CurrentState) {
    //    shared_ptr<Animator> animator = GetAnimator();
    //    if (animator) {
    //        animator->Play(m_CurrentState);
    //    }
    //    m_PrevState = m_CurrentState;
    //}

    BYTE inputDirection = 0;

    if (INPUT->GetButton(KEY_TYPE::UP))
        inputDirection |= 1;
    if (INPUT->GetButton(KEY_TYPE::DOWN))
        inputDirection |= 2;
    if (INPUT->GetButton(KEY_TYPE::RIGHT))
        inputDirection |= 4;
    if (INPUT->GetButton(KEY_TYPE::LEFT))
        inputDirection |= 8;
    if (INPUT->GetButton(KEY_TYPE::A))
        inputDirection |= 16;
    if (INPUT->GetButtonDown(KEY_TYPE::S))
        inputDirection |= 32;
    if (INPUT->GetButtonDown(KEY_TYPE::SPACE))
        inputDirection |= 64;

    CS_MOVE_PACKET movePacket;
    movePacket.size = sizeof(movePacket);
    movePacket.type = CS_MOVE;
    movePacket.direction = inputDirection;
    movePacket.prevDirection = m_PrevDirection;

    // 서버로 이동 패킷 전송
    send_packet(&movePacket);

    m_PrevDirection = inputDirection;

    // 상태에 따른 애니메이션 변경
    if (m_PrevState != m_CurrentState) {
        shared_ptr<Animator> animator = GetAnimator();
        if (animator) {
            animator->Play(m_CurrentState);
        }
        m_PrevState = m_CurrentState;
    }
}


PlayerScript2::PlayerScript2()
    : m_PrevDirection(0)
    , m_PrevState(PLAYER_STATE::CLIMB)
    , m_CurrentState(PLAYER_STATE::IDLE)
    , m_Velocity(0.0f)
{
}

void PlayerScript2::Update()
{
    // 상태에 따른 애니메이션 변경
    if (m_PrevState != m_CurrentState) {
        shared_ptr<Animator> animator = GetAnimator();
        if (animator) {
            animator->Play(m_CurrentState);
        }
        m_PrevState = m_CurrentState;
    }
}