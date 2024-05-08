#include "pch.h"
#include "PlayerScript.h"
#include "Input.h"
#include "Animator.h"
#include "Transform.h"
#include "PhysXComponent.h"
#include "Timer.h"
#include "Engine.h"
#include "SceneManager.h"
#include "Scene.h"
#include "GameObject.h"

PlayerScript::PlayerScript()
	: m_Acceleration(0.0f)
	, m_Velocity(0.0f)
	, m_RotationSpeed(7.5f)
	, m_Weight(100.0f)
	, m_CurrentState(PLAYER_STATE::CLIMB)
{
}

void PlayerScript::Update()
{
	PxControllerManager* manager = gEngine->GetControllerManager();
	PxController* playerController = manager->getController(0);
	PxVec3 disp(0.0f);

	// 카메라의 회전 정보 가져오기
	Vec3 cameraRotation = GET_SINGLE(SceneManager)->GetActiveScene()->GetGameObjectByName(L"Main_Camera")->GetTransform()->GetLocalRotation();

	// 카메라 회전을 고려하여 이동 방향 계산
	float cosX = cos(cameraRotation.x);
	float sinX = sin(cameraRotation.x);
	float cosY = cos(cameraRotation.y);
	float sinY = sin(cameraRotation.y);

	if (INPUT->GetButton(KEY_TYPE::UP))
	{
		disp.x += sinY * cosX;
		disp.y += sinX;
		disp.z += cosY * cosX;
	}

	if (INPUT->GetButton(KEY_TYPE::DOWN))
	{
		disp.x -= sinY * cosX;
		disp.y -= sinX;
		disp.z -= cosY * cosX;
	}

	if (INPUT->GetButton(KEY_TYPE::RIGHT))
	{
		disp.x += cosY;
		disp.z -= sinY;
	}

	if (INPUT->GetButton(KEY_TYPE::LEFT))
	{
		disp.x -= cosY;
		disp.z += sinY;
	}

	if (INPUT->GetButton(KEY_TYPE::A) && m_CurrentState != PLAYER_STATE::AIM && m_CurrentState != PLAYER_STATE::FIRE && m_CurrentState != PLAYER_STATE::ROLL) {
		m_CurrentState = PLAYER_STATE::AIM;
		m_Velocity = 0.0f;
		m_Acceleration = 0.0f;
	}

	if (INPUT->GetButtonUp(KEY_TYPE::A)) {
		m_CurrentState = PLAYER_STATE::IDLE;
	}

	if ((m_CurrentState == PLAYER_STATE::AIM && INPUT->GetButtonDown(KEY_TYPE::S)) || (m_CurrentState == PLAYER_STATE::FIRE && INPUT->GetButtonDown(KEY_TYPE::S))) {
		m_CurrentState = PLAYER_STATE::FIRE;

		// FIRE 애니메이션 재생
		shared_ptr<Animator> animator = GetAnimator();
		if (animator) {
			animator->Play(PLAYER_STATE::FIRE);
		}
	}

	// PLAYER_STATE::FIRE 상태일 때 애니메이션이 끝나면 상태 변경
	if (m_CurrentState == PLAYER_STATE::FIRE) {
		shared_ptr<Animator> animator = GetAnimator();
		if (animator && animator->IsAnimationFinished(PLAYER_STATE::FIRE)) {
			if (INPUT->GetButton(KEY_TYPE::A)) {
				m_CurrentState = PLAYER_STATE::AIM;
			}
			else {
				m_CurrentState = PLAYER_STATE::IDLE;
			}
		}
	}

	if (INPUT->GetButtonDown(KEY_TYPE::SPACE) && m_CurrentState != PLAYER_STATE::AIM && m_CurrentState != PLAYER_STATE::FIRE && m_CurrentState != PLAYER_STATE::ROLL) {
		m_CurrentState = PLAYER_STATE::ROLL;

		// ROLL 애니메이션 재생
		shared_ptr<Animator> animator = GetAnimator();
		if (animator) {
			animator->Play(PLAYER_STATE::ROLL);
		}
	}

	// PLAYER_STATE::ROLL 상태일 때 애니메이션이 끝나면 상태 변경
	if (m_CurrentState == PLAYER_STATE::ROLL) {
		shared_ptr<Animator> animator = GetAnimator();
		if (true == animator->IsAnimationFinished(PLAYER_STATE::ROLL)) {
			m_CurrentState = PLAYER_STATE::IDLE;
		}
		else {
			// 가속도 계산
			m_Acceleration = 1000.0f;

			// 속도 업데이트
			m_Velocity += m_Acceleration * DELTA_TIME;
		}
	}

	if (false == disp.isZero()) {
		if (disp.magnitudeSquared() > 0.0f) {
			disp.normalize();

			if (m_CurrentState != PLAYER_STATE::AIM && m_CurrentState != PLAYER_STATE::FIRE && m_CurrentState != PLAYER_STATE::ROLL) {
				// 가속도 계산
				m_Acceleration = disp.magnitude() * 1000.0f;

				// 속도 업데이트
				m_Velocity += m_Acceleration * DELTA_TIME;
				m_Velocity = std::clamp(m_Velocity, 0.0f, 500.0f);

				if (m_Velocity <= 0.0f)
					m_CurrentState = PLAYER_STATE::IDLE;
				else if (m_Velocity > 0.0f && m_Velocity <= 150.0f)
					m_CurrentState = PLAYER_STATE::WALK;
				else if (m_Velocity > 150.0f && m_Velocity <= 350.0f)
					m_CurrentState = PLAYER_STATE::RUN_SLOW;
				else if (m_Velocity > 350.0f)
					m_CurrentState = PLAYER_STATE::RUN_FAST;
			}

			PxReal targetAngle = PxAtan2(-disp.x, -disp.z);

			// 현재 회전 각도와 목표 회전 각도 계산
			Vec3 currentRotation = GetTransform()->GetLocalRotation();
			PxReal currentAngle = currentRotation.y;

			// 최소 회전 방향 계산
			PxReal deltaAngle = targetAngle - currentAngle;
			if (deltaAngle > PxPi)
				deltaAngle -= PxTwoPi;
			else if (deltaAngle < -PxPi)
				deltaAngle += PxTwoPi;

			// 보간 비율 계산
			PxReal interpolationFactor = m_RotationSpeed * DELTA_TIME;

			// 보간을 이용하여 부드러운 회전 적용
			PxReal interpolatedAngle = currentAngle + deltaAngle * interpolationFactor;

			// 보간된 각도를 -180도에서 180도 범위로 조정
			if (interpolatedAngle > PxPi)
				interpolatedAngle -= PxTwoPi;
			else if (interpolatedAngle < -PxPi)
				interpolatedAngle += PxTwoPi;

			Vec3 Rotation = GetTransform()->GetLocalRotation();
			Rotation.y = interpolatedAngle;
			GetTransform()->SetLocalRotation(Rotation);
		}
	}
	else {
		// 이동하지 않을 때 속도와 가속도 초기화
		m_Velocity = 0.0f;
		m_Acceleration = 0.0f;
		if (m_CurrentState != PLAYER_STATE::AIM && m_CurrentState != PLAYER_STATE::FIRE && m_CurrentState != PLAYER_STATE::ROLL) {
			m_CurrentState = PLAYER_STATE::IDLE;
		}
	}

	// 이동 속도 설정
	disp *= m_Velocity * DELTA_TIME;

	// 중력 적용
	disp.y -= 9.8f * DELTA_TIME * m_Weight;

	// 캐릭터 컨트롤러 이동
	PxControllerFilters filters;
	playerController->move(disp, 0.001f, DELTA_TIME, filters);

	// 캐릭터 이동
	Vec3 Position = GetTransform()->GetLocalPosition();
	Position.x = playerController->getPosition().x;
	Position.y = playerController->getPosition().y - 50.f;
	Position.z = playerController->getPosition().z;

	// 이전 상태와 현재 상태가 다르면 애니메이션 변경
	if (m_PrevState != m_CurrentState) {
		shared_ptr<Animator> animator = GetAnimator();
		if (animator) {
			animator->Play(m_CurrentState);
		}
		m_PrevState = m_CurrentState;
	}

	cameraPos->x = Position.x;
	cameraPos->y = Position.y;
	cameraPos->z = Position.z;

	cameraRot->x = GetTransform()->GetLocalRotation().x;
	cameraRot->y = GetTransform()->GetLocalRotation().y;
	cameraRot->z = GetTransform()->GetLocalRotation().z;
	GetTransform()->SetLocalPosition(Position);
}