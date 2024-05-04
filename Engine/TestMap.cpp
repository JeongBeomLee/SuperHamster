#include "pch.h"
#include "TestMap.h"
#include "Input.h"
#include "Animator.h"
#include "Transform.h"
#include "Timer.h"
#include "Engine.h"

const float moveForce = 10000.0f;
const PxVec3 up(0.0f, 1.0f, 0.0f);

void TestMap::Update()
{
	//if (INPUT->GetButtonDown(KEY_TYPE::KEY_1)) {
	//	Vec3 position = GetTransform()->GetLocalPosition();
	//	Vec3 scale = GetTransform()->GetLocalScale();

	//	position.y -= 39.f;

	//	GetTransform()->SetLocalPosition(position);
	//	GetTransform()->SetLocalScale(scale);
	//}

	//if (INPUT->GetButtonDown(KEY_TYPE::KEY_2)) {
	//	Vec3 position = GetTransform()->GetLocalPosition();

	//	position.y += 5.f;

	//	GetTransform()->SetLocalPosition(position);
	//}
}