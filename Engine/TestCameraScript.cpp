#include "pch.h"
#include "TestCameraScript.h"
#include "Transform.h"
#include "Camera.h"
#include "GameObject.h"
#include "Input.h"
#include "Timer.h"
#include "SceneManager.h"
#include "Scene.h"

TestCameraScript::TestCameraScript() : m_RotationX(45.0f), m_RotationY(-35.0f), m_Distance(1200.0f), m_Height(75.0f)
{
}

TestCameraScript::~TestCameraScript()
{
}

void TestCameraScript::LateUpdate()
{
	/*shared_ptr<GameObject> player = GET_SINGLE(SceneManager)->GetActiveScene()->GetGameObjectByName(L"Hamster");
	Vec3 playerPos = player->GetTransform()->GetLocalPosition();*/

	shared_ptr<GameObject> player = GET_SINGLE(SceneManager)->GetActiveScene()->GetGameObjectByName(L"Hamster");

	Vec3 playerPos = player->GetTransform()->GetLocalPosition();

	// 카메라의 회전 설정
	float rotationX = XMConvertToRadians(m_RotationX);
	float rotationY = XMConvertToRadians(m_RotationY);
	Vec3 cameraRotation(rotationX, rotationY, 0.0f);
	GetTransform()->SetLocalRotation(cameraRotation);

	// 카메라의 위치 설정
	Vec3 cameraDirection = GetTransform()->GetLook();
	Vec3 cameraPos = playerPos - cameraDirection * m_Distance + Vec3(0.0f, m_Height, 0.0f);
	GetTransform()->SetLocalPosition(cameraPos);

	Vec3 pos = GetTransform()->GetLocalPosition();

	if (INPUT->GetButton(KEY_TYPE::W))
		m_RotationX += 1.f;

	if (INPUT->GetButton(KEY_TYPE::S))
		m_RotationX -= 1.f;

	if (INPUT->GetButton(KEY_TYPE::A))
		m_RotationY += 1.f;

	if (INPUT->GetButton(KEY_TYPE::D))
		m_RotationY -= 1.f;

	if (INPUT->GetButton(KEY_TYPE::SPACE))
		m_Distance += 1.f;

	if (INPUT->GetButton(KEY_TYPE::LCONTROL))
		m_Distance -= 1.f;

	if (INPUT->GetButton(KEY_TYPE::Q))
		m_Height += 1.f;

	if (INPUT->GetButton(KEY_TYPE::E))
		m_Height -= 1.f;

	if (INPUT->GetButton(KEY_TYPE::Z))
	{
		Vec3 rotation = GetTransform()->GetLocalRotation();
		rotation.y += DELTA_TIME * 0.5f;
		GetTransform()->SetLocalRotation(rotation);
	}

	if (INPUT->GetButton(KEY_TYPE::C))
	{
		Vec3 rotation = GetTransform()->GetLocalRotation();
		rotation.y -= DELTA_TIME * 0.5f;
		GetTransform()->SetLocalRotation(rotation);
	}

	if (INPUT->GetButtonDown(KEY_TYPE::RBUTTON))
	{
		const POINT& pos = INPUT->GetMousePos();
		GET_SINGLE(SceneManager)->Pick(pos.x, pos.y);
	}

	/*cameraPos->x = pos.x;
	cameraPos->y = pos.y;
	cameraPos->z = pos.z;

	cameraRot->x = GetTransform()->GetLocalRotation().x;
	cameraRot->y = GetTransform()->GetLocalRotation().y;
	cameraRot->z = GetTransform()->GetLocalRotation().z;*/

	//GetTransform()->SetLocalPosition(pos);
}