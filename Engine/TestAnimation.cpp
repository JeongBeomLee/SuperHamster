#include "pch.h"
#include "TestAnimation.h"
#include "Input.h"
#include "Animator.h"
#include "Transform.h"
#include "PhysXComponent.h"
#include "Timer.h"
#include "Engine.h"
#include "SceneManager.h"
#include "Scene.h"
#include "GameObject.h"

const float moveForce = 10000.0f;
const PxVec3 up(0.0f, 1.0f, 0.0f);

void TestAnimation::Update()
{
	if (INPUT->GetButtonDown(KEY_TYPE::KEY_1)) {
		shared_ptr<Animator> animator = GetAnimator();
		if(!animator)
			return;

		int32 count = animator->GetAnimCount();
		int32 currentIndex = animator->GetCurrentClipIndex();
		
		int32 index = (currentIndex + 1) % count;
		
		animator->Play(index);
	}

	if (INPUT->GetButtonDown(KEY_TYPE::KEY_2)) {
		shared_ptr<Animator> animator = GetAnimator();
		if (!animator)
			return;

		int32 count = animator->GetAnimCount();
		int32 currentIndex = animator->GetCurrentClipIndex();

		int32 index = (currentIndex - 1 + count) % count;

		animator->Play(index);
	}

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

		if (false == disp.isZero()) {
			if (disp.magnitudeSquared() > 0.0f) {
				disp.normalize();

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
				PxReal interpolationFactor = 7.5f * DELTA_TIME;

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
		
		// 이동 속도 설정
		disp *= 550.0f * DELTA_TIME;

		// 중력 적용
		disp.y -= 20.8f * DELTA_TIME * 78.4f;

		// 캐릭터 컨트롤러 이동
		PxControllerFilters filters;
		playerController->move(disp, 0.001f, DELTA_TIME, filters);

		// 캐릭터 이동
		Vec3 Position = GetTransform()->GetLocalPosition();
		Position.x = playerController->getPosition().x;
		Position.y = playerController->getPosition().y - 50.f;
		Position.z = playerController->getPosition().z;

		cameraPos->x = Position.x;
		cameraPos->y = Position.y;
		cameraPos->z = Position.z;

		cameraRot->x = GetTransform()->GetLocalRotation().x;
		cameraRot->y = GetTransform()->GetLocalRotation().y;
		cameraRot->z = GetTransform()->GetLocalRotation().z;
		GetTransform()->SetLocalPosition(Position);
	}
}