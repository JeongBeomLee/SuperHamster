#pragma once
#include "MonoBehaviour.h"

class PlayerScript : public MonoBehaviour
{
public:
	PlayerScript();
	virtual void Update() override;

private:
	float m_Velocity;
	float m_RollVelocity;
	float m_RotationSpeed;
	float m_Weight;
	float m_Acceleration;
	PLAYER_STATE m_CurrentState;
	PLAYER_STATE m_PrevState;
};

