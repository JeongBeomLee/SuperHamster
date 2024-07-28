#pragma once
#include "MonoBehaviour.h"

enum class PLAYER_GUN {
	DEFAULT,
	LASER,
	MAGNETIC,

	END
};

class PlayerScript : public MonoBehaviour
{
public:
	PlayerScript();
	void SetVelocity(float velocity) { m_Velocity = velocity; }
	void SetState(PLAYER_STATE state) { m_CurrentState = state; }
	void UpdateGun(PLAYER_GUN gun);

	virtual void Update() override;

private:
	PLAYER_STATE m_CurrentState;
	PLAYER_STATE m_PrevState;
	PLAYER_GUN m_Gun = PLAYER_GUN::DEFAULT;
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
	PLAYER_GUN m_Gun = PLAYER_GUN::DEFAULT;
	char m_PrevDirection;
	float m_Velocity;
};

