#include "pch.h"
#include "TestAnimation.h"
#include "Input.h"
#include "Animator.h"
#include "Transform.h"
#include "PhysXComponent.h"
#include "Timer.h"
#include "Engine.h"

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

	if (INPUT->GetButtonDown(KEY_TYPE::KEY_3)) {
		Vec3 Rotation = GetTransform()->GetLocalRotation();

		Rotation.x -= 0.1f;

		GetTransform()->SetLocalRotation(Rotation);
	}

	// W
	/*PxRigidDynamic* playerActor = static_pointer_cast<PhysXComponent>(GetPhysXComponent())->GetPhysicsActor()->is<PxRigidDynamic>();
	if (INPUT->GetButton(KEY_TYPE::W)) {
		PxVec3 forward = playerActor->getGlobalPose().q.rotate(PxVec3(0.0f, 0.0f, 1.0f));
		playerActor->addForce(forward * moveForce);
	}*/


	{
		PxControllerManager* manager = gEngine->GetControllerManager();
		PxController* playerController = manager->getController(0);
		PxVec3 disp(0.0f);

		if (INPUT->GetButton(KEY_TYPE::UP)) 
			disp.z += 1.0f;
		if (INPUT->GetButton(KEY_TYPE::DOWN)) 
			disp.z -= 1.0f;
		if (INPUT->GetButton(KEY_TYPE::LEFT))
			disp.x -= 1.0f;
		if (INPUT->GetButton(KEY_TYPE::RIGHT))
			disp.x += 1.0f;

		if (disp.magnitudeSquared() > 0.0f) {
			disp.normalize();

			// 캐릭터 컨트롤러 회전
			PxVec3 forward = PxVec3(0.0f, 0.0f, 1.0f);
			PxVec3 right = PxVec3(1.0f, 0.0f, 0.0f);
			PxReal angle = PxAtan2(disp.x, -disp.z);
			PxQuat rotation(angle, PxVec3(0.0f, 1.0f, 0.0f));

			PxExtendedVec3 currentPosition = playerController->getPosition();
			PxVec3 currentPositionVec3(float(currentPosition.x), float(currentPosition.y), float(currentPosition.z));
			PxTransform transform(currentPositionVec3, rotation);
			PxExtendedVec3 newPosition = PxExtendedVec3(transform.p.x, transform.p.y, transform.p.z);
			playerController->setPosition(newPosition);

			// 캐릭터 회전
			Vec3 Rotation = GetTransform()->GetLocalRotation();
			Rotation.y = -angle;
			GetTransform()->SetLocalRotation(Rotation);
		}

		// 이동 속도 설정
		disp *= 550.0f * DELTA_TIME;

		// 중력 적용
		disp.y -= 20.8f * DELTA_TIME;

		// 캐릭터 컨트롤러 이동
		PxControllerFilters filters;
		playerController->move(disp, 0.001f, DELTA_TIME, filters);

		// 캐릭터 이동
		Vec3 Position = GetTransform()->GetLocalPosition();
		Position.x = playerController->getPosition().x;
		Position.y = playerController->getPosition().y;
		Position.z = playerController->getPosition().z;
		GetTransform()->SetLocalPosition(Position);

		cameraPos->x = GetTransform()->GetLocalPosition().x;
		cameraPos->y = GetTransform()->GetLocalPosition().y;
		cameraPos->z = GetTransform()->GetLocalPosition().z;

		cameraRot->x = GetTransform()->GetLocalRotation().x;
		cameraRot->y = GetTransform()->GetLocalRotation().y;
		cameraRot->z = GetTransform()->GetLocalRotation().z;
	}
}