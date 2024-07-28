#pragma once
#include "Material.h"
#include "MonoBehaviour.h"

class WaterScript : public MonoBehaviour
{
public:
	WaterScript();
	virtual ~WaterScript() = default;
	virtual void Update() override;

private:
	shared_ptr<Material> _material;
	float _time = 0.0f;
};



