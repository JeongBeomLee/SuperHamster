#pragma once
#include "MonoBehaviour.h"

class TestCameraScript : public MonoBehaviour
{
public:
	TestCameraScript();
	virtual ~TestCameraScript();

	virtual void LateUpdate() override;

private:
	float _speed = 500.f;
	float m_RotationX;
	float m_RotationY;
	float m_Distance;
	float m_Height;
};

