#pragma once
#include "MonoBehaviour.h"

class PlayerScript : public MonoBehaviour
{
public:
	PlayerScript();
	void SetVelocity(float velocity) { m_Velocity = velocity; }
	void SetState(PLAYER_STATE state) { m_CurrentState = state; }
	virtual void Update() override;

private:
	PLAYER_STATE m_CurrentState;
	PLAYER_STATE m_PrevState;
	char m_PrevDirection;
	float m_Velocity;
};

class PlayerScript2 : public MonoBehaviour
{
public:
	PlayerScript2();
	void SetVelocity(float velocity) { m_Velocity = velocity; }
	void SetState(PLAYER_STATE state) { m_CurrentState = state; }
	virtual void Update() override;

private:
	PLAYER_STATE m_CurrentState;
	PLAYER_STATE m_PrevState;
	char m_PrevDirection;
	float m_Velocity;
};

