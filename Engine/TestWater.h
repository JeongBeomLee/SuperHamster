#pragma once

#include "Material.h"
#include "MonoBehaviour.h"

class TestWater : public MonoBehaviour
{
public:
	TestWater();
	virtual ~TestWater() = default;
	virtual void Update() override;

private:
	shared_ptr<Material> _material;
	float _time = 0.0f;
};



